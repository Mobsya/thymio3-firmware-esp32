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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
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

//! \details Declaration of the timer HW pointer function
typedef void (*TimerHWFunctionPtr)(void*);

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the HW timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \param     autoReload - Allow auto reload
//! \param     interval - Duration of the timer
//! \param     callback - Pointer to function called when interval is reached
//! \return    None
extern void TimerHw_Init(int16_t timerGroup, int timerIndex, bool autoReload, uint64_t interval,
                         const TimerHWFunctionPtr callback);

//! \brief     Start the HW timer
//! \pre       First initialize the HW timer module
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \return    None
extern void TimerHw_Start(int16_t timerNum, int16_t timerIndex);

//! \brief     Stop the HW timer
//! \pre       First initialize the HW timer module
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \return    None
extern void TimerHw_Stop(int16_t timerNum, int16_t timerIndex);

//! \brief     Set an alarm value for the HW timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \param     interval - Duration of the timer in microseconds
//! \return    None
extern void TimerHw_Set_Alarm(int16_t timerNum, int16_t timerIndex, uint64_t interval_us);

//! \brief     Set an alarm value for the HW timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \param     interval - Duration of the timer in timer ticks
//! \return    None
extern void TimerHw_Set_Alarm_Ticks(int16_t timerNum, int16_t timerIndex, uint64_t interval_ticks);

//! \brief     Deinitialize the HW timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \return    None
extern void TimerHw_Deinit(int16_t timerNum, int16_t timerIndex);

//! \brief     Get the counter value of hardware timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \return    None
extern uint64_t TimerHw_Get_Counter(int16_t timerNum, int16_t timerIndex);

//! \brief     Reset the counter value of hardware timer
//! \pre       None
//! \param     timerGroup - Timer group (0 or 1)
//! \param     timerIndex - Timer index (0 or 1)
//! \return    None
extern void TimerHw_Reset_Counter(int16_t timerNum, int16_t timerIndex);

#endif // TIMER_HW_H_
