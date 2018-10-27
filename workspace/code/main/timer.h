//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer.h
//! \brief   This module provides the useful functions to use the timer
//!
//! \author  Vincent Gonet
//!
//! \version $Id: timer.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef TIMER_H_
#define TIMER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#if 0
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the xxx
//! \pre       None
//! \param     None
//! \return    None
extern void Timer_Init(int timer_idx, bool auto_reload, double timer_interval_sec);

//! \brief     Run the xxx task
//! \pre       First initialize the xxx
//! \param     None
//! \return    None
extern void Timer_Task(void);

extern void Timer_CreateSemaphore(void);

//extern TaskHandle_t Timer_GetSemaphore(void);
extern SemaphoreHandle_t Timer_GetSemaphore(void);

extern void Timer_CheckSemaphore(void);
#endif
#endif // TIMER_H_
