//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
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
#define B_LEDS_LEGO     (1 << 3)
#define B_LEDS_BATTERY  (1 << 4)
#define B_LEDS_CIRCLE   (1 << 5)
#define B_LEDS_ACC      (1 << 6)
#define B_LEDS_RGB      (1 << 7)
#define B_LED_MIC       (1 << 8)
#define B_LED_RC5       (1 << 9)
#define B_MODE          (1 << 10)
#define B_SETTING       (1 << 11)

#define B_ALWAYS    (B_LEDS_BATTERY | B_LEDS_BUTTON | B_SOUND_BUTTON | B_LEDS_CIRCLE | B_LEDS_RGB | B_LED_RC5)  // TODO (B_LEDS_BATTERY | B_LEDS_RC5 | B_SOUND_BUTTON | B_LEDS_BUTTON)

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

//! \brief     Stop the behavior task
//! \pre       None
//! \param     None
//! \return    None
extern void Behavior_Stop(void);

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

//! \brief     Get the status of behaviors
//! \pre       None
//! \param     None
//! \return    The status (bit = 0 -> behavior is disabled, bit = 1 -> behavior is enabled)
extern uint16_t Behavior_GetStatus(void);

#endif // BEHAVIOR_H_
