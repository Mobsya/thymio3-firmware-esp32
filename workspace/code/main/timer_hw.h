//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//#define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
#define TIMER_INTERVAL0_SEC   (0.001) // sample test interval for the first timer
//#define TIMER_INTERVAL1_SEC   (5.78)   // sample test interval for the second timer
#define TIMER_INTERVAL1_SEC   (0.001)   // sample test interval for the second timer
#define TEST_WITHOUT_RELOAD   0        // testing will be done without auto reload
#define TEST_WITH_RELOAD      1        // testing will be done with auto reload

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct PrivateTimer;
typedef struct PrivateTimer T_TimerHw;  //!< Definition of T_Timer type

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initializes the Timer module.
//! \pre       None
//! \param     None
//! \return    None
extern void TimerHw_Init(void);

extern void TimerHw_StartTimer(uint16_t interval, uint32_t duration);

extern void TimerHw_StartFrontTimer60us(void);

extern void TimerHw_StartBackTimer60us(void);

extern void TimerHw_StartAudioTimer20us(void);

#if 0 // TODO Remove if not used
extern void TimerHw_StartRightTimer375us(void);

extern void TimerHw_StartLeftTimer375us(void);
#endif

extern void TimerHw_Task(void);
//extern void TimerHw_Task(void* pvParameter);

//! \brief     Get the system timestamp (number of 100us since boot)
//! \pre       First initialize the timer.
//! \param     None
//! \return    The system timestamp in [100us].
extern uint32_t TimerHw_GetSystemTimestamp(void);

//! \brief     Create the timer.
//! \pre       First initialize the timer.
//! \param     interval_ms is the timer expiration interval in [ms].
//! \return    The timer created.
extern T_TimerHw* TimerHw_Create(uint32_t interval_ms);

//! \brief     Start the timer.
//! \pre       First initialize the timer.
//! \param     timer is the timer to start.
//! \return    None
extern void TimerHw_Start(T_TimerHw* timer);

//! \brief     Stop the timer.
//! \pre       First initialize the timer.
//! \param     timer is the timer to stop.
//! \return    None
extern void TimerHw_Stop(T_TimerHw* timer);

//! \brief     Restart the timer.
//! \pre       First initialize the timer.
//! \param     timer is the timer to restart.
//! \return    None
extern void TimerHw_Restart(T_TimerHw* timer);

//! \brief     Set the timer interval. If the timer is started, this will restart the timer.
//! \pre       First initialize the timer.
//! \param     timer is the timer to restart.
//! \param     interval_ms is the timer expiration interval in [ms].
//! \return    None
extern void TimerHw_SetInterval(T_TimerHw* timer, uint32_t interval_ms);

//! \brief     Is the timer started ?
//! \pre       First initialize the timer.
//! \param     timer is the timer to check.
//! \return    True if timer started, false otherwise.
extern bool TimerHw_IsStarted(const T_TimerHw* timer);

//! \brief     Check if the timer has expired.
//! \pre       First initialize the timer.
//! \param     timer is the timer to check.
//! \return    True if the timer has expired, false otherwise.
extern bool TimerHw_HasExpired(const T_TimerHw* timer);

//! \brief     Is scheduler timer flag set?
//! \pre       First initialize the timer.
//! \param     None
//! \return    True if set, false otherwise
extern bool TimerHw_IsSchedulerFlagSet(void);

//! \brief     Reset the scheduler timer flag
//! \pre       First initialize the timer.
//! \param     None
//! \return    None
extern void TimerHw_ResetSchedulerFlag(void);

extern void TimerHw_CallbackFront60us(void* arg);

extern void TimerHw_CallbackBack60us(void* arg);

extern void TimerHw_CallbackAudio20us(void* arg);

#endif // TIMER_HW_H_
