//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_hw.c
//! \brief   This module provides the useful functions to use the hardware timers
//!
//! \author  Vincent Gonet
//!
//! \version $Id: timer_hw.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>

#include "esp_types.h"

#include "soc/timer_group_struct.h"

#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/queue.h"

//#include "xtensa/xtruntime.h"

#include "timer_hw.h"

#include "board.h"
#include "gpio.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TIMER_NUM             TIMER_1

#define TIMER_DIVIDER         16  //!<  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define MAX_TIMERS_ALLOWED    10u  //!< Maximum number of timers allowed

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details Parameters of the timer
struct PrivateTimer
{
  volatile uint32_t Elapsed_ms;   //!< Elapsed time [ms]
  uint32_t          Interval_ms;  //!< Duration of the timer [ms]
  bool              InUseFlag;    //!< Flag that indicates if the timer is used
  bool              StartedFlag;  //!< Flag that indicates if the timer is started
};

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

static bool TimerSchedulerFlag = false;  //!< Scheduler timer flag (100 [us] scheduling)

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static volatile uint32_t SystemTimestamp = 0UL;  //!< System timestamp incremented every 100 [us]

static struct PrivateTimer TableTimers[MAX_TIMERS_ALLOWED];  //!< Table containing the timers created

static const T_GpioPinConfig PinConfig = {SOUND_OUT_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable};

//xQueueHandle timer_queue;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

void IRAM_ATTR timer_group0_isr(void *para);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TimerHw_Init(void)
{
  Gpio_ConfigurePin(&PinConfig);

  double timer_interval_sec = 0.00002;  //= 20us
  //double timer_interval_sec = 1.0;

  /* Select and initialize basic parameters of the timer */
  timer_config_t config;
  config.divider = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.intr_type = TIMER_INTR_LEVEL;
  config.auto_reload = true;

  timer_init(TIMER_GROUP_0, TIMER_NUM, &config);

  /* Timer's counter will initially start from value below.
	 Also, if auto_reload is set, this value will be automatically reload on alarm */
  timer_set_counter_value(TIMER_GROUP_0, TIMER_NUM, 0x00000000ULL);

  /* Configure the alarm value and the interrupt on alarm. */
  timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM, timer_interval_sec * TIMER_SCALE);
  //timer_set_alarm_value(TIMER_GROUP_0, TIMER_NUM, timer_interval_sec);
  timer_enable_intr(TIMER_GROUP_0, TIMER_NUM);
  timer_isr_register(TIMER_GROUP_0, TIMER_NUM, timer_group0_isr,
  (void *) TIMER_NUM, ESP_INTR_FLAG_IRAM, NULL);

  timer_start(TIMER_GROUP_0, TIMER_NUM);

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  // Initialize the table timers
  for (uint8_t i = 0U; i < MAX_TIMERS_ALLOWED; i++)
  {
    TableTimers[i].InUseFlag   = false;
    TableTimers[i].Elapsed_ms  = 0UL;
    TableTimers[i].Interval_ms = 0UL;
    TableTimers[i].StartedFlag = false;
  }

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________
#if 0
void TimerHw_Task(void)
{
  //ESP_ERROR_CHECK(esp_timer_start_once(periodic_timer, 60));
}
#endif
#if 0
void TimerHw_Task(void* pvParameter)
{
  //timer_queue = xQueueCreate(10, sizeof(timer_event_t));

  while (1)
  {
    //timer_event_t evt;
    //xQueueReceive(timer_queue, &evt, portMAX_DELAY);
  }
}
#endif
//_____________________________________________________________________________

uint32_t TimerHw_GetSystemTimestamp(void)
{
  uint32_t result;

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  result = SystemTimestamp;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();

  return result;
}

//_____________________________________________________________________________

T_TimerHw* TimerHw_Create(uint32_t interval_ms)
{
  T_TimerHw* timer = NULL;
  uint8_t i;

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  for (i = 0U; i < MAX_TIMERS_ALLOWED; i++)
  {
    if (!TableTimers[i].InUseFlag)
    {
      timer = &TableTimers[i];
      timer->InUseFlag = true;

      timer->Elapsed_ms  = 0UL;
      timer->Interval_ms = interval_ms;
      timer->StartedFlag = false;
      break;
    }
  }

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();

  //M_CheckPointer(timer)

  return timer;
}

//_____________________________________________________________________________

void TimerHw_Start(T_TimerHw* timer)
{
  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  timer->Elapsed_ms  = 0UL;
  timer->StartedFlag = true;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________

void TimerHw_Stop(T_TimerHw* timer)
{
  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  timer->Elapsed_ms  = 0UL;
  timer->StartedFlag = false;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________

void TimerHw_Restart(T_TimerHw* timer)
{
  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  timer->Elapsed_ms  = 0UL;
  timer->StartedFlag = true;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________

void TimerHw_SetInterval(T_TimerHw* timer, uint32_t interval_ms)
{
  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  timer->Elapsed_ms  = 0UL;
  timer->Interval_ms = interval_ms;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________

bool TimerHw_IsStarted(const T_TimerHw* timer)
{
  bool result;

  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  result = timer->StartedFlag;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();

  return result;
}

//_____________________________________________________________________________

bool TimerHw_HasExpired(const T_TimerHw* timer)
{
  bool result;

  //M_CheckPointer(timer)

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  result = (timer->Elapsed_ms >= timer->Interval_ms);

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();

  return result;
}

//_____________________________________________________________________________

bool TimerHw_IsSchedulerFlagSet(void)
{
  bool result;

  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  result = TimerSchedulerFlag;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();

  return result;
}

//_____________________________________________________________________________

void TimerHw_ResetSchedulerFlag(void)
{
  //M_CriticalSectionEnter();
  //uint32_t volatile register ilevel = XTOS_DISABLE_ALL_INTERRUPTS;
  //portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
  //portENTER_CRITICAL(&myMutex);

  TimerSchedulerFlag = false;

  //portEXIT_CRITICAL(&myMutex);
  //XTOS_RESTORE_INTLEVEL(ilevel);
  //M_CriticalSectionExit();
}

//_____________________________________________________________________________

void IRAM_ATTR timer_group0_isr(void *para)
{
  int timer_idx = (int) para;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG0.int_st_timers.val;

  TIMERG0.hw_timer[timer_idx].update = 1;

  //uint64_t timer_counter_value =
    //((uint64_t) TIMERG0.hw_timer[timer_idx].cnt_high) << 32
    //| TIMERG0.hw_timer[timer_idx].cnt_low;

  // Prepare basic event data
  //that will be then sent back to the main program task */
  //timer_event_t evt;
  //evt.timer_group = 0;
  //evt.timer_idx = timer_idx;
  //evt.timer_counter_value = timer_counter_value;

  Gpio_TogglePinLevel(SOUND_OUT_PIN);
  //Leds_Task();

  // Clear the interrupt
  if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_NUM)
  {
//#if 0
    for (uint8_t i = 0u; i < MAX_TIMERS_ALLOWED; i++)
    {
      if (TableTimers[i].StartedFlag)
      {
        TableTimers[i].Elapsed_ms++;
      }
    }
//#endif

    //Gpio_TogglePinLevel(TEST_PIN);
    //evt.type = TEST_WITH_RELOAD;
    TIMERG0.int_clr_timers.t1 = 1;
  }
  //else
  //{
    //evt.type = -1; // not supported even type
  //}

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

  /* Now just send the event data back to the main program task */
  //xQueueSendFromISR(timer_queue, &evt, NULL);
}
