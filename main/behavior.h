//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    behavior.h
//! \brief   This module provides the useful functions to handle the behavior
//!
//! \author  Vincent Gonet
//!
//! \version $Id: behavior.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef BEHAVIOR_H_
#define BEHAVIOR_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define B_SOUND_BUTTON  (1 << 0)
#define B_LEDS_BUTTON   (1 << 1)
#define B_LEDS_PROX     (1 << 2)
#define B_LEDS_BATTERY  (1 << 5)
#define B_LEDS_ACC      (1 << 8)
#define B_MODE          (1 << 10)
#define B_SETTING       (1 << 12)

#define B_ALWAYS    (B_LEDS_BATTERY | B_LEDS_BUTTON | B_SOUND_BUTTON)  // TODO (B_LEDS_BATTERY | B_LEDS_RC5 | B_LEDS_SD | B_SOUND_BUTTON | B_LEDS_BUTTON)

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

//! \brief     Initialize the behavior
//! \pre       None
//! \param     None
//! \return    None
extern void Behavior_Init(void);

//! \brief     Start the behavior task
//! \pre       None
//! \param     None
//! \return    None
extern void Behavior_Start(void);

//! \brief     Enable the behavior
//! \pre       None
//! \param     b - Behavior to start
//! \return    None
extern void Behavior_Enable(uint16_t b);

//! \brief     Disable the behavior
//! \pre       None
//! \param     b - Behavior to stop
//! \return    None
extern void Behavior_Disable(uint16_t b);

#endif // BEHAVIOR_H_
