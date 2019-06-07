//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_hw.c
//! \brief   This module provides the useful functions to use the HW timers
//!
//! \author  Vincent Gonet
//!
//! \version $Id: timer_hw.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "esp_log.h"

#include "timer_hw.h"

#include "board.h"
#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TIMER_DIVIDER 16 //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "timer_hw";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TimerHw_Init(int16_t timerGroup, int timerIndex, bool autoReload, double interval, void (*fn)(void*))
{
  // Select and initialize basic parameters of the timer
  timer_config_t config;

  config.divider     = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en  = TIMER_PAUSE;
  config.alarm_en    = TIMER_ALARM_EN;
  config.intr_type   = TIMER_INTR_LEVEL;
  config.auto_reload = autoReload;

  if ((timerGroup < TIMER_GROUP_MAX) && (timerIndex < TIMER_MAX))
  {
    timer_init(timerGroup, timerIndex, &config);

    // Timer's counter will initially start from value below
    // Also, if auto_reload is set, this value will be automatically reload on alarm
    timer_set_counter_value(timerGroup, timerIndex, 0x00000000ULL);

    // Configure the alarm value and the interrupt on alarm
    timer_set_alarm_value(timerGroup, timerIndex, interval);
    timer_enable_intr(timerGroup, timerIndex);


    timer_isr_register(timerGroup, timerIndex, fn,
                      (void*) timerIndex, ESP_INTR_FLAG_IRAM, NULL);
   
    ESP_LOGI(Tag, "Group %d Timer %d is initialized", timerGroup, timerIndex);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid parameters, Group = %d, Index = %d", timerGroup, timerIndex);
  }   
}

//_____________________________________________________________________________

void TimerHw_Start(int16_t timerNum, int16_t timerIndex)
{
  timer_start(timerNum, timerIndex);
}

//_____________________________________________________________________________

void TimerHw_Stop(int16_t timerNum, int16_t timerIndex)
{
  timer_pause(timerNum, timerIndex);
}
