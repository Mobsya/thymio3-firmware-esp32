//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer_hw.h
//! \brief   This module provides the useful functions to use the hardware timers
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

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct PrivateTimer;
typedef struct PrivateTimer T_TimerHw;  //!< Definition of T_TimerHw type

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
extern void TimerHw_Init(void);

//! \brief     Create the timer
//! \pre       First initialize the timer
//! \param     duration_us - Duration in [us]
//! \param     callback - Callback function called when duration is reached
//! \return    Timer created
extern T_TimerHw* TimerHw_Create(uint32_t duration_us, void (*callback)(void*));

//! \brief     Start the timer once
//! \pre       First initialize the timer
//! \param     timer - Timer to start once
//! \param     duration_us - Duration in [us]
//! \return    None
extern void TimerHw_StartTimerOnce(T_TimerHw* timer, uint32_t duration_us);

//! \brief     Start the timer periodically
//! \pre       First initialize the timer
//! \param     timer - Timer to start periodically
//! \param     duration_us - Duration in [us]
//! \return    None
extern void TimerHw_StartTimerPeriodically(T_TimerHw* timer, uint32_t duration_us);

//! \brief     Stop the timer
//! \pre       First initialize the timer
//! \param     timer - Timer to stop
//! \return    None
extern void TimerHw_StopTimer(T_TimerHw* timer);

#endif // TIMER_HW_H_
