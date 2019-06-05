//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_hw.h
//! \brief   This module provides the useful functions to use the HW timers
//!
//! \author  Vincent Gonet
//!
//! \version $Id: timer_hw.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef TIMER_HW_H_
#define TIMER_HW_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "driver/timer.h"

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

//! \brief     Initialize the timer
//! \pre       None
//! \param     None
//! \return    None
extern void TimerHw_Init(int16_t timerNum, int timerIndex, bool autoReload, double interval_sec);

//! \brief     Initialize the timer
//! \pre       None
//! \param     None
//! \return    None
extern void TimerHw_Init1(int16_t timerGroup, int timerIndex, bool autoReload, double interval, void (*fn)(void*));
//extern void Timer_Init1(int16_t timerNum, int timerIndex, bool autoReload, double interval);

//! \brief     Start the timer
//! \pre       First initialize the timer
//! \param     None
//! \return    None
extern void TimerHw_Start(int16_t timerNum, int16_t timerIndex);

extern void TimerHw_Stop(int16_t timerNum, int16_t timerIndex);

#endif // TIMER_HW_H_
