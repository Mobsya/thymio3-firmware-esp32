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
#include <freertos/semphr.h>

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

//xSemaphoreHandle SoundSemaphore;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static volatile uint32_t SystemTimestamp = 0UL;  //!< System timestamp incremented every 100 [us]

static struct PrivateTimer TableTimers[MAX_TIMERS_ALLOWED];  //!< Table containing the timers created

static const T_GpioPinConfig PinConfig = {IR_PULSE_FRONT_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable};

//xQueueHandle timer_queue;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//static void periodic_timer_callback(void* arg);

esp_timer_handle_t Timer125us;
esp_timer_handle_t Timer500us;
esp_timer_handle_t Timer200ms;

esp_timer_handle_t oneshot_timer;

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TimerHw_Init(void)
{
  Gpio_ConfigurePin(&PinConfig);

  // Configuration of Timer125us
  const esp_timer_create_args_t timer125us_args =
  {
    .callback = &TimerHw_Callback125us,
    .name = "timer125us"
  };
  
  ESP_ERROR_CHECK(esp_timer_create(&timer125us_args, &Timer125us));
  // The timer has been created but is not running yet

  ESP_ERROR_CHECK(esp_timer_start_periodic(Timer125us, 125));
#if 0
  // Configuration of Timer500us
  const esp_timer_create_args_t timer500us_args =
  {
    .callback = &TimerHw_Callback500us,
    .name = "timer500us"
  };

  ESP_ERROR_CHECK(esp_timer_create(&timer500us_args, &Timer500us));
  // The timer has been created but is not running yet

  ESP_ERROR_CHECK(esp_timer_start_periodic(Timer500us, 700));

  // Configuration of Timer200ms
  const esp_timer_create_args_t timer200ms_args =
  {
    .callback = &TimerHw_Callback200ms,
    .name = "timer200ms"
  };

  ESP_ERROR_CHECK(esp_timer_create(&timer200ms_args, &Timer200ms));
  // The timer has been created but is not running yet

  ESP_ERROR_CHECK(esp_timer_start_periodic(Timer200ms, 200000));
#endif

  const esp_timer_create_args_t oneshot_timer_args =
  {
    .callback = &oneshot_timer_callback,
    /* argument specified here will be passed to timer callback function */
    .arg = (void*) Timer125us,
    .name = "one-shot"
  };

  ESP_ERROR_CHECK(esp_timer_create(&oneshot_timer_args, &oneshot_timer));

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

void TimerHw_StartTimer(uint16_t interval, uint32_t duration)
{
  ESP_ERROR_CHECK(esp_timer_start_periodic(Timer125us, interval));
  //ESP_ERROR_CHECK(esp_timer_start_once(oneshot_timer, duration));
}

//_____________________________________________________________________________
#if 0
void TimerHw_Task(void)
{
  //ESP_ERROR_CHECK(esp_timer_start_once(periodic_timer, 60));
  while (1)
  {
	if(SoundSemaphore != NULL )
	{
      if (xSemaphoreTake(SoundSemaphore, 0))
      {
        Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);
      }
	}

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
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
