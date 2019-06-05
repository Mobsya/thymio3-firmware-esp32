//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_sw.h
//! \brief   This module provides the useful functions to use the SW timers
//!
//! \author  Vincent Gonet
//!
//! \version $Id: timer_sw.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef TIMER_SW_H_
#define TIMER_SW_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct PrivateTimer;
typedef struct PrivateTimer T_TimerSw;  //!< Definition of T_TimerSw type

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initializes the Timer module
//! \pre       None
//! \param     None
//! \return    None
extern void TimerSw_Init(void);

//! \brief     Create the timer
//! \pre       First initialize the timer
//! \param     duration_us - Duration in [us]
//! \param     callback - Callback function called when duration is reached
//! \return    Timer created
extern T_TimerSw* TimerSw_Create(uint32_t duration_us, void (*callback)(void*));

//! \brief     Start the timer once
//! \pre       First initialize the timer
//! \param     timer - Timer to start once
//! \param     duration_us - Duration in [us]
//! \return    None
extern void TimerSw_StartTimerOnce(T_TimerSw* timer, uint32_t duration_us);

//! \brief     Start the timer periodically
//! \pre       First initialize the timer
//! \param     timer - Timer to start periodically
//! \param     duration_us - Duration in [us]
//! \return    None
extern void TimerSw_StartTimerPeriodically(T_TimerSw* timer, uint32_t duration_us);

//! \brief     Stop the timer
//! \pre       First initialize the timer
//! \param     timer - Timer to stop
//! \return    None
extern void TimerSw_StopTimer(T_TimerSw* timer);

#endif // TIMER_SW_H_
