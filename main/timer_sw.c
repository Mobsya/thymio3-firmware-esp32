//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_sw.c
//! \brief   This module provides the useful functions to use the SW timers
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

#include "esp_log.h"
#include "esp_timer.h"

#include "timer_sw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MAX_TIMERS_ALLOWED    10u  //!< Maximum number of timers allowed

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details Parameters of the timer
struct PrivateTimer
{
  bool                    IsUsed;       //!< Flag that indicates if the timer is used
  bool                    IsRunning;    //!< Flag that indicates if the timer is running
  uint32_t                Duration_us;  //!< Duration of the timer [us]
  esp_timer_create_args_t Config;       //!< Configuration of the timer
  esp_timer_handle_t      Handler;      //!< Handler of the timer
};

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "timer_sw";

static struct PrivateTimer TableTimers[MAX_TIMERS_ALLOWED];  //!< Table containing the timers created

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TimerSw_Init(void)
{
  // Initialize the table timers
  for (uint8_t index = 0u; index < MAX_TIMERS_ALLOWED; index++)
  {
    TableTimers[index].IsUsed      = false;
    TableTimers[index].IsRunning   = false;
    TableTimers[index].Duration_us = 0uL;
  }

  ESP_LOGI(Tag, "Software Timer is initialized");
}

//_____________________________________________________________________________

T_TimerSw* TimerSw_Create(uint32_t duration_us, const TimerSWFunctionPtr callback)
{
  T_TimerSw* timer = NULL;
  uint8_t index = 0u;

  for (index = 0u; index < MAX_TIMERS_ALLOWED; index++)
  {
    if (!TableTimers[index].IsUsed)
    {
      timer = &TableTimers[index];
      timer->IsUsed          = true;
      timer->IsRunning       = false;
      timer->Duration_us     = duration_us;
      timer->Config.callback = callback;

      ESP_ERROR_CHECK(esp_timer_create(&timer->Config, &timer->Handler));
      // The timer has been created but is not running yet
      break;
    }
  }

  ESP_LOGI(Tag, "Software Timer %d is created", index);

  return timer;
}

//_____________________________________________________________________________

void TimerSw_StartTimerOnce(T_TimerSw* timer, uint32_t duration_us)
{
  ESP_ERROR_CHECK(esp_timer_start_once(timer->Handler, duration_us));

  timer->IsRunning = true;
}

//_____________________________________________________________________________

void TimerSw_StartTimerPeriodically(T_TimerSw* timer, uint32_t duration_us)
{
  ESP_ERROR_CHECK(esp_timer_start_periodic(timer->Handler, duration_us));

  timer->IsRunning = true;
}

//_____________________________________________________________________________

void TimerSw_StopTimer(T_TimerSw* timer)
{
  if (timer->IsRunning)
  {
    ESP_ERROR_CHECK(esp_timer_stop(timer->Handler));

    timer->IsRunning = false;
  }
}
