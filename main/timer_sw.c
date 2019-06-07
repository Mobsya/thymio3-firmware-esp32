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
//! \version $Id: timer_sw.c 18076 2017-04-20 12:28:12Z v.gonet $
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

#define MAX_TIMERS_ALLOWED    15u  //!< Maximum number of timers allowed

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
  for (uint8_t i = 0U; i < MAX_TIMERS_ALLOWED; i++)
  {
    TableTimers[i].IsUsed      = false;
    TableTimers[i].IsRunning   = false;
    TableTimers[i].Duration_us = 0ul;
  }

  ESP_LOGI(Tag, "Software Timer is initialized");
}

//_____________________________________________________________________________

T_TimerSw* TimerSw_Create(uint32_t duration_us, void (*callback)(void*))
{
  T_TimerSw* timer = NULL;
  uint8_t i;

  for (i = 0U; i < MAX_TIMERS_ALLOWED; i++)
  {
    if (!TableTimers[i].IsUsed)
    {
      timer = &TableTimers[i];
      timer->IsUsed          = true;
      timer->IsRunning       = false;
      timer->Duration_us     = duration_us;
      timer->Config.callback = callback;

      ESP_ERROR_CHECK(esp_timer_create(&timer->Config, &timer->Handler));
      // The timer has been created but is not running yet
      break;
    }
  }

  ESP_LOGI(Tag, "Software Timer %d is created", i);

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
