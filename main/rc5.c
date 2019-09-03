//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    rc5.c
//! \brief   This module provides the useful functions to use the RC5 receiver
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"

#include "driver/rmt.h"

#include "rc5.h"

#include "aseba_esp32.h"
#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

/*
14 bits
3 bit preamble (2x start, 1x toggle)
5 bit address + 6 bit command length
Carrier frequency of 36kHz
Bi-phase coding (aka Manchester coding)
Constant bit time of 1.778ms (64 cycles of 36 kHz)
RC5X command 7 bits
*/

#define RMT_RX_CHANNEL             0  //!< RMT channel for receiver

#define RMT_CLK_DIV               80  //!< RMT counter clock divider
#define RMT_TICK_10_US    (80000000 / RMT_CLK_DIV / 100000)  //!< RMT counter value for 10 us.(Source clock is APB clock)

#define RMT_ITEM32_TIMEOUT_us   9500  //!< RMT receiver timeout value(us)

#define RMT_ITEM_DURATION(d)    ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */

#define RMT_RX_ACTIVE_LEVEL        0  //!< If we connect with a IR receiver, the data is active low

#define RC5_BITS_NUM              14  //!< 2 start + 1 toggle + 5 address + 6 data

#define RC5_MIN_SHORT_DURATION   500
#define RC5_MAX_SHORT_DURATION  1000

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "rc5";

static rmt_config_t rmt_rx;

static bool FrameIsValid = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunRXTask(void* arg);

static inline void ParseItems(rmt_item16_t* item);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void RC5_Init(void)
{
  rmt_rx.channel                       = RMT_RX_CHANNEL;
  rmt_rx.gpio_num                      = IR_RECEIVER_PIN;
  rmt_rx.clk_div                       = RMT_CLK_DIV;
  rmt_rx.mem_block_num                 = 1;
  rmt_rx.rmt_mode                      = RMT_MODE_RX;
  rmt_rx.rx_config.filter_en           = true;
  rmt_rx.rx_config.filter_ticks_thresh = 255;
  rmt_rx.rx_config.idle_threshold      = RMT_ITEM32_TIMEOUT_us / 10 * (RMT_TICK_10_US);

  rmt_config(&rmt_rx);

  ESP_LOGI(Tag, "IR receiver is initialized");
}

//_____________________________________________________________________________

void RC5_Start(void)
{
  xTaskCreatePinnedToCore(
    RunRXTask,  // Function to implement the task
    "rc5",      // Name of the task
    2048,       // Stack size in words
    NULL,       // Task input parameter
    3,          // Priority of the task
    NULL,       // Task handle
    0);         // Core where the task should run
}

//_____________________________________________________________________________

bool RC5_IsFrameValid(void)
{
  return FrameIsValid;
}

//_____________________________________________________________________________

void RC5_ClearFrameValidity(void)
{
  FrameIsValid = false;
}

//_____________________________________________________________________________

static void RunRXTask(void* arg)
{
  while (1)
  {
    //vTaskDelay(10);
//    ESP_LOGI(Tag, "Start IR receiver Task");
    //IRReceiver_Init();
    rmt_driver_install(rmt_rx.channel, 1000, 0);

    //get RMT RX ringbuffer
    RingbufHandle_t buffer = NULL;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &buffer);

    // rmt_rx_start(channel, rx_idx_rst) - Set true to reset memory index for receiver
    rmt_rx_start(RMT_RX_CHANNEL, true);

    while (buffer)
    {
      size_t rx_size = 0;
      rmt_item16_t* item = (rmt_item16_t*) xRingbufferReceive(buffer, &rx_size, 1000);

      if (item)
      {
        //ESP_LOGI(Tag, "rx_size = %d", rx_size);
        //ESP_LOGI(Tag, "Received waveform - buffer size: %d (%d items)", rx_size, rx_size / 2);

        //rmt_dump_items(item, rx_size / 2);

        ParseItems(item);

        vRingbufferReturnItem(buffer, (void*) item);
      }
      else
      {
        break;
      }
    }

    rmt_driver_uninstall(RMT_RX_CHANNEL);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

static inline void ParseItems(rmt_item16_t* item)
{
  // Bit  13    -> First Start bit (S1)
  // Bit  12    -> Second Start bit (S2)
  // Bit  11    -> Toggle bit
  // Bits 10..6 -> Address bits
  // Bits  5..0 -> Command bits

  int16_t value = 0x2000;  // binary = 10000000000000 -> MSB is S1
  int16_t toggle = 0;      // 1 bit
  int16_t address = 0;     // 5 bits
  int16_t command = 0;     // 6 bits
  int16_t mask = 0x2000;

  static int16_t oldToggle = 0;

  if ((item->level == RMT_RX_ACTIVE_LEVEL) && (item->duration > RC5_MIN_SHORT_DURATION))  // Is S1 valid ?
  {
    for (int16_t pos = 0; pos < (RC5_BITS_NUM - 1); pos++) // Only 13 bits are decoded because S1 is already done
    {
      //ESP_LOGE(Tag, "item = %d", i);

      if (item->level == RMT_RX_ACTIVE_LEVEL)
      {
        if ((item->duration > RC5_MIN_SHORT_DURATION) && (item->duration < RC5_MAX_SHORT_DURATION))
        {
          //ESP_LOGE(Tag, "Bit = 1");
          value |= (mask >> (pos + 1));
          item += 2;
        }
        else
        {
           //ESP_LOGE(Tag, "Bit = 0");
           item++;
        }
      }
      else
      {
        if ((item->duration > RC5_MIN_SHORT_DURATION) && (item->duration < RC5_MAX_SHORT_DURATION))
        {
          //ESP_LOGE(Tag, "Bit = 0");
          item += 2;
        }
        else
        {
          //ESP_LOGE(Tag, "Bit = 1");
          value |= (mask >> (pos + 1));
          item++;
        }
      }
    }

	FrameIsValid = true;

    toggle = (value >> 11) & 0x01;

    if (toggle != oldToggle)
    {
      address = (value >> 6) & 0x1F;

      // S2 (inverted) | Command
      command = (~(value >> 6) & 0x40) | (value & 0x3F);

      vmVariables.rc5_address = address;
      vmVariables.rc5_command = command;
      //SET_EVENT(EVENT_RC5);

      //ESP_LOGI(Tag, "IR CODE: 0x%04x", value);
      //ESP_LOGI(Tag, "Address: 0x%04x", address);
      //ESP_LOGI(Tag, "Data: 0x%04x", data);

      //ESP_LOGI(Tag, "Toggle: %d", toggle);
      //ESP_LOGI(Tag, "Address: %d", address);
      //ESP_LOGI(Tag, "Command: %d", command);

      oldToggle = toggle;
    }
  }
}
