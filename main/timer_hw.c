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

static void IRAM_ATTR ISR_TimerGroup0(void* para);

static void IRAM_ATTR ISR_TimerGroup1(void* para);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TimerHw_Init(int16_t timerNum, int timerIndex, bool autoReload, double interval_sec)
{
  // Select and initialize basic parameters of the timer
  timer_config_t config;

  config.divider     = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en  = TIMER_PAUSE;
  config.alarm_en    = TIMER_ALARM_EN;
  config.intr_type   = TIMER_INTR_LEVEL;
  config.auto_reload = autoReload;

  timer_init(timerNum, timerIndex, &config);

  // Timer's counter will initially start from value below
  // Also, if auto_reload is set, this value will be automatically reload on alarm
  timer_set_counter_value(timerNum, timerIndex, 0x00000000ULL);

  // Configure the alarm value and the interrupt on alarm
  timer_set_alarm_value(timerNum, timerIndex, interval_sec * TIMER_SCALE);
  timer_enable_intr(timerNum, timerIndex);

  if (timerNum == TIMER_GROUP_0)
  {
    timer_isr_register(timerNum, timerIndex, ISR_TimerGroup0,
                       (void*) timerIndex, ESP_INTR_FLAG_IRAM, NULL);
  }
#if 0
  else
  {
    timer_isr_register(timerNum, timerIndex, ISR_TimerGroup1,
                       (void*) timerIndex, ESP_INTR_FLAG_IRAM, NULL);
  }
#endif

  ESP_LOGI(Tag, "Group %d Timer %d is initialized", timerNum, timerIndex);


  ESP_LOGI(Tag, "Scale = %d, Base = %d", TIMER_SCALE, TIMER_BASE_CLK);
}

//_____________________________________________________________________________

void TimerHw_Init1(int16_t timerGroup, int timerIndex, bool autoReload, double interval, void (*fn)(void*))
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
#if 0
void Timer_Init1(int16_t timerNum, int timerIndex, bool autoReload, double interval)
{
  // Select and initialize basic parameters of the timer
  timer_config_t config;

  config.divider     = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en  = TIMER_PAUSE;
  config.alarm_en    = TIMER_ALARM_EN;
  config.intr_type   = TIMER_INTR_LEVEL;
  config.auto_reload = autoReload;

  //if ((timerNum < TIMER_GROUP_MAX) && (timerIndex < TIMER_MAX))
  {
    timer_init(timerNum, timerIndex, &config);

    // Timer's counter will initially start from value below
    // Also, if auto_reload is set, this value will be automatically reload on alarm
    timer_set_counter_value(timerNum, timerIndex, 0x00000000ULL);

    // Configure the alarm value and the interrupt on alarm
    timer_set_alarm_value(timerNum, timerIndex, interval);
    timer_enable_intr(timerNum, timerIndex);

    if (timerNum == TIMER_GROUP_0)
    {
      timer_isr_register(timerNum, timerIndex, ISR_TimerGroup0,
                         (void*) timerIndex, ESP_INTR_FLAG_IRAM, NULL);
    }
    else
    {
      timer_isr_register(timerNum, timerIndex, ISR_TimerGroup1,
                         (void*) timerIndex, ESP_INTR_FLAG_IRAM, NULL);
    }

    ESP_LOGI(Tag, "Group %d Timer %d is initialized", timerNum, timerIndex);
  }
}
#endif
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

//_____________________________________________________________________________

static void IRAM_ATTR ISR_TimerGroup0(void* para)
{
  int timer_idx = (int) para;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  TIMERG0.hw_timer[timer_idx].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if ((intr_status & BIT(timer_idx)) && (timer_idx == TIMER_0))
  {
    TIMERG0.int_clr_timers.t0 = 1;
  }
  else if ((intr_status & BIT(timer_idx)) && (timer_idx == TIMER_1))
  {
    TIMERG0.int_clr_timers.t1 = 1;
  }
  else
  {
    // Do nothing
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;
}

//_____________________________________________________________________________
//#if 0  // TODO Add it if more timers are used. Otherwise remove it
static void IRAM_ATTR ISR_TimerGroup1(void* para)
{
  int timer_idx = (int) para;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[timer_idx].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if ((intr_status & BIT(timer_idx)) && (timer_idx == TIMER_0))
  {
    //Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_Low);
    //Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);

    //TIMERG1.int_clr_timers.t0 = 1;
    //timer_pause(1, 0);
  }
  else if ((intr_status & BIT(timer_idx)) && (timer_idx == TIMER_1))
  {
    TIMERG1.int_clr_timers.t1 = 1;
  }
  else
  {
    // Do nothing
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;
}
//#endif
