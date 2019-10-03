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

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/rmt.h"

#include "rc5.h"

#include "aseba_esp32.h"
#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define RMT_RX_CHANNEL             0  //!< RMT channel for receiver

#define RMT_CLK_DIV               80  //!< RMT counter clock divider
#define RMT_TICK_10_US    (80000000 / RMT_CLK_DIV / 100000)  //!< RMT counter value for 10 us.(Source clock is APB clock)

#define RMT_ITEM32_TIMEOUT_us   9500  //!< RMT receiver timeout value(us)

//#define RMT_ITEM_DURATION(d)    ((d & 0x7fff)*10/RMT_TICK_10_US)  /*!< Parse duration time from memory register value */

#define RMT_RX_ACTIVE_LEVEL        0  //!< If we connect with a IR receiver, the data is active low

#define RC5_BITS_NUM              14  //!< 2 start + 1 toggle + 5 address + 6 data

#define RC5_MIN_SHORT_DURATION   500
#define RC5_MAX_SHORT_DURATION  1000

#define VALID_ADDRESS              0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  union
  {
    struct
    {
      uint16_t duration : 15;
      uint16_t level : 1;
    };
    uint16_t val;
  };
} rmt_item16_t;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "rc5";

static TaskHandle_t RC5Task = NULL;

static bool TaskIsStarted = false;

static rmt_config_t rmt_rx;

static bool FrameIsValid = false;

static int16_t Toggle = 0;   // 1 bit
static int16_t OldToggle = 0;

static int16_t Address = 0;  // 5 bits
static int16_t Command = 0;  // 6 bits + 1 start bit (S2)

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunRXTask(void* arg);

static inline bool ParseItems(rmt_item16_t* item);

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

  TaskIsStarted = false;

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
    &RC5Task,   // Task handle
    0);         // Core where the task should run

  TaskIsStarted = true;
}

//_____________________________________________________________________________

void RC5_Stop(void)
{
  if (TaskIsStarted)
  {
    ESP_LOGW(Tag, "RC5 task is stopped");

    rmt_driver_uninstall(rmt_rx.channel);
    TaskIsStarted = false;
    vTaskDelete(RC5Task);
  }
}

//_____________________________________________________________________________

int16_t RC5_GetCommand(void)
{
  return Command;
}

//_____________________________________________________________________________

bool RC5_IsNewMessageReceived(int16_t* last)
{
  bool result = false;

  if (*last != Toggle)
  {
    result = true;
  }

  *last = Toggle;

  return result;
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
  ESP_LOGI(Tag, "Start IR receiver Task");

  while (1)
  {
    rmt_driver_install(rmt_rx.channel, 1000, 0);

    //get RMT RX ring buffer
    RingbufHandle_t buffer = NULL;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &buffer);

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

        if (ParseItems(item))
        {
          if (Address == VALID_ADDRESS)
          {
            FrameIsValid = true;

            if (Toggle != OldToggle)
            {
              vmVariables.rc5_address = Address;
              vmVariables.rc5_command = Command;
              //SET_EVENT(EVENT_RC5);

              OldToggle = Toggle;
            }
          }
        }

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

static inline bool ParseItems(rmt_item16_t* item)
{
  int16_t value = 0x2000;  // binary = 10000000000000 -> MSB is S1
  int16_t mask = 0x2000;

  bool frameIsReceived = false;

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

    frameIsReceived = true;

    // Bit  13    -> First Start bit (S1)
    // Bit  12    -> Second Start bit (S2)
    // Bit  11    -> Toggle bit
    // Bits 10..6 -> Address bits
    // Bits  5..0 -> Command bits
    Toggle = (value >> 11) & 0x01;
    Address = (value >> 6) & 0x1F;
    // S2 (inverted) | Command
    Command = (~(value >> 6) & 0x40) | (value & 0x3F);

    //ESP_LOGI(Tag, "IR CODE: 0x%04x", value);
    //ESP_LOGI(Tag, "Address: 0x%04x", address);
    //ESP_LOGI(Tag, "Data: 0x%04x", data);
    //ESP_LOGI(Tag, "Toggle: %d", toggle);
    //ESP_LOGI(Tag, "Address: %d", address);
    //ESP_LOGI(Tag, "Command: %d", command);
  }

  return frameIsReceived;
}
