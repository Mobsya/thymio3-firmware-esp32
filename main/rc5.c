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

//#define RC5_BIT_US                1680
//#define RC5_BIT_HALF_US            840

//#define RC5_DATA_ITEM_NUM       14  /*!< RC5 code item number: 2 start + 1 toggle + 5 addr + 6 data + (rmt end) */
//#define RC5_BIT_MARGIN          90

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunRXTask(void* arg);

//static int rc5_parse_items(rmt_item16_t* item, int item_num, uint32_t* cmd_data);

static inline void ParseItems(rmt_item16_t* item);

//static bool rc5_detect_bit(rmt_item16_t* item);

//static bool rc5_header_if(rmt_item16_t* item);

//static bool rc5_bit_one_if(rmt_item16_t* item);

//static bool rc5_bit_zero_if(rmt_item16_t* item);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//static inline void rmt_dump_items(rmt_item16_t* item, int item_num);

//inline bool rmt_check_in_range(int duration_ticks, int target_us, int margin_us);

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void RC5_Init(void)
{
  //rmt_config_t rmt_rx;
  rmt_rx.channel                       = RMT_RX_CHANNEL;
  rmt_rx.gpio_num                      = IR_RECEIVER_PIN;
  rmt_rx.clk_div                       = RMT_CLK_DIV;
  rmt_rx.mem_block_num                 = 1;
  rmt_rx.rmt_mode                      = RMT_MODE_RX;
  rmt_rx.rx_config.filter_en           = true;
  rmt_rx.rx_config.filter_ticks_thresh = 100;
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

static void RunRXTask(void* arg)
{
  while (1)
  {
    //vTaskDelay(10);
//    ESP_LOGI(Tag, "Start IR receiver Task");
    //IRReceiver_Init();
    rmt_driver_install(rmt_rx.channel, 1000, 0);

    //get RMT RX ringbuffer
    RingbufHandle_t rb = NULL;
    rmt_get_ringbuf_handle(RMT_RX_CHANNEL, &rb);

    // rmt_rx_start(channel, rx_idx_rst) - Set true to reset memory index for receiver
    rmt_rx_start(RMT_RX_CHANNEL, true);

    while (rb)
    {
      size_t rx_size = 0;
      rmt_item16_t* item = (rmt_item16_t*) xRingbufferReceive(rb, &rx_size, 1000);

      if (item)
      {
        //ESP_LOGI(Tag, "rx_size = %d", rx_size);
        //ESP_LOGI(Tag, "Received waveform - buffer size: %d (%d items)", rx_size, rx_size / 2);

        //rmt_dump_items(item, rx_size / 2);

        ParseItems(item);

        //uint32_t rmt_data = 0;
        //int res = rc5_parse_items(item, rx_size / 2, &rmt_data);
        //int res = rc5_parse_items(item, 20, &rmt_data);
        //ESP_LOGI(Tag, "IR CODE: 0x%08x", rmt_data);

        vRingbufferReturnItem(rb, (void*) item);
      }
      else
      {
//        ESP_LOGI(Tag, "ELSE (ITEM)");
        break;
      }
    }

//    ESP_LOGI(Tag, "END");
    rmt_driver_uninstall(RMT_RX_CHANNEL);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________
#if 0
static inline void rmt_dump_items(rmt_item16_t* item, int item_num)
{
  ESP_LOGI(Tag, "Dump %d items", item_num);

  for (int i = 0; i < item_num; i++)
  {
    ESP_LOGI(Tag, "Item %d: (%d) %dms", i, item->level, item->duration);
    item++;
  }
}
#endif
//_____________________________________________________________________________
#if 0
static int rc5_parse_items(rmt_item16_t* item, int item_num, uint32_t* cmd_data)
{
    // decoding is different from sending as low-high is normally 1
    // however, when receiving, rmt will start with a high value
    if(item_num < RC5_DATA_ITEM_NUM) {
        //ESP_LOGE(Tag, "item_num = %d", item_num);
        ESP_LOGE(Tag, "ITEM NUMBER ERROR");
        return -1;
    }

    int i = 0;

    if(!rc5_header_if(item++)) {
        ESP_LOGE(Tag, "HEADER ERROR");
        return -1;
    } else {
        i++;
    }

    uint32_t decoded = 0;

    // parse from left to right 11 bits (0x400)
    uint32_t mask = 0x01;
    mask <<= 11 - 1;

    for (int j = 0; j < 11; j++) {

        if (rc5_bit_one_if(item)) {
            decoded |= (mask >> j);
        } else if (rc5_bit_zero_if(item)) {
            decoded |= 0 & (mask >> j);
        } else {
            ESP_LOGE(Tag, "BIT ERROR");
            return -1;
        }
        item++;
        i++;
    }

    *cmd_data = decoded;

    return i;
}
#endif
//_____________________________________________________________________________

static inline void ParseItems(rmt_item16_t* item)
{
  int16_t value = 0x2000;  // binary = 10000000000000 -> MSB is the first start bit
  int16_t toggle = 0;   // 1 bit
  int16_t address = 0;  // 5 bits
  int16_t command = 0;  // 6 bits
  int16_t mask = 0x2000;

  for (int16_t pos = 0; pos < (RC5_BITS_NUM - 1); pos++) // Only 13 bits are decoded because the first start bit is already done
  {
    //ESP_LOGE(Tag, "item = %d", i);

    if (item->level == RMT_RX_ACTIVE_LEVEL)
    {
      if (item->duration < RC5_MAX_SHORT_DURATION)
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
      if (item->duration < RC5_MAX_SHORT_DURATION)
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

  toggle = (value >> 11) & 0x01;

  address = (value >> 6) & 0x1F;

  // 2nd start bit (inverted) | command
  command = (~(value >> 6) & 0x40) | (value & 0x3F);

  vmVariables.rc5_address = address;
  vmVariables.rc5_command = command;

  ESP_LOGI(Tag, "IR CODE: 0x%04x", value);
  //ESP_LOGI(Tag, "Address: 0x%04x", address);
  //ESP_LOGI(Tag, "Data: 0x%04x", data);

  ESP_LOGI(Tag, "Address: %d", address);
  ESP_LOGI(Tag, "Data: %d", command);
}

//_____________________________________________________________________________
#if 0
static bool rc5_detect_bit(rmt_item16_t* item)
{
    if(item->level == RMT_RX_ACTIVE_LEVEL)
    {
        return true;
    }
    return false;
}
#endif
//_____________________________________________________________________________
#if 0
static bool rc5_header_if(rmt_item16_t* item)
{
  //if ((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
  //   && rmt_check_in_range(item->duration0, RC5_BIT_HALF_US, RC5_BIT_MARGIN)
  //   && rmt_check_in_range(item->duration1, RC5_BIT_HALF_US, RC5_BIT_MARGIN))

  if ((item->level == RMT_RX_ACTIVE_LEVEL) &&
      rmt_check_in_range(item->duration, RC5_BIT_HALF_US, RC5_BIT_MARGIN))
  {
    //ESP_LOGE(Tag, "HEADER OK");
    return true;
  }

  //ESP_LOGE(Tag, "HEADER FALSE");
  return false;
}
#endif
//_____________________________________________________________________________
#if 0
static bool rc5_bit_one_if(rmt_item16_t* item)
{
  //ESP_LOGI(Tag, "CHECK BIT ONE");

  //if ((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL)
  //   && rmt_check_in_range(item->duration0, RC5_BIT_HALF_US, RC5_BIT_MARGIN)
  //   && rmt_check_in_range(item->duration1, RC5_BIT_HALF_US, RC5_BIT_MARGIN))

  if ((item->level == RMT_RX_ACTIVE_LEVEL) &&
      rmt_check_in_range(item->duration, RC5_BIT_HALF_US, RC5_BIT_MARGIN))
  {
    //ESP_LOGE(Tag, "BIT ONE OK");
    return true;
  }

  //ESP_LOGE(Tag, "BIT ONE FALSE");
  return false;
}
#endif
//_____________________________________________________________________________
#if 0
static bool rc5_bit_zero_if(rmt_item16_t* item)
{
  //ESP_LOGI(Tag, "CHECK BIT ZERO");

  //if ((item->level0 == RMT_RX_ACTIVE_LEVEL && item->level1 != RMT_RX_ACTIVE_LEVEL) &&
  //    rmt_check_in_range(item->duration0, RC5_BIT_HALF_US, RC5_BIT_MARGIN) &&
  //    rmt_check_in_range(item->duration1, RC5_BIT_HALF_US, RC5_BIT_MARGIN))

  if ((item->level == RMT_RX_ACTIVE_LEVEL) &&
      rmt_check_in_range(item->duration, RC5_BIT_HALF_US, RC5_BIT_MARGIN))
  {
    //ESP_LOGE(Tag, "BIT ZERO OK");
    return true;
  }

  //ESP_LOGE(Tag, "BIT ZERO FALSE");
  return false;
}
#endif
//_____________________________________________________________________________
#if 0
inline bool rmt_check_in_range(int duration_ticks, int target_us, int margin_us)
{
  //ESP_LOGI(Tag, "duration_ticks = %d", duration_ticks);

  if ((RMT_ITEM_DURATION(duration_ticks) < (target_us + margin_us)) &&
      (RMT_ITEM_DURATION(duration_ticks) > (target_us - margin_us)))
  {
    //ESP_LOGI(Tag, "RANGE OK");
    return true;
  }
  else
  {
    //ESP_LOGE(Tag, "RANGE FALSE");
    return false;
  }
}
#endif
