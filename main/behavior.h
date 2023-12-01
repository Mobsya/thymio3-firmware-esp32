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
//! \author  Vincent Gonet, Stefano Morgani
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
#define B_LEDS_PROX     (1 << 2) // Implemented on STM
#define B_LEDS_LEGO_GYRO (1 << 3)
#define B_LEDS_BATTERY  (1 << 4) // Implemented on STM
#define B_LEDS_CIRCLE   (1 << 5) // Not implemented?
#define B_LEDS_ACC      (1 << 6)
#define B_LEDS_RGB      (1 << 7) // Not implemented?
#define B_LED_MIC       (1 << 8) // Implemented on STM, 2 bits [9 8]
#define B_LED_MIC_STATE (1 << 9) // [0 0] => manual setting LED OFF, [1 0] => manual setting LED ON, [x 1] => LED ON when volume > threshold
#define B_LED_RC5       (1 << 10)
#define B_MODE          (1 << 11)
#define B_SETTING       (1 << 12)
#define B_LEDS_LEGO_KITT (1 << 13)
#define B_LEDS_TEST 	(1 << 14)

#define B_ALWAYS    (B_LEDS_BATTERY | B_LEDS_BUTTON | B_SOUND_BUTTON | B_LEDS_CIRCLE | B_LEDS_RGB)  // TODO (B_LEDS_BATTERY | B_LEDS_RC5 | B_SOUND_BUTTON | B_LEDS_BUTTON)
// B_LEDS_RGB not used
// B_LEDS_CIRCLE not used
// B_LEDS_BATTERY not used

#define RUNNING_SETTINGS_MENU 0
#define RUNNING_SETTINGS_BEHAVIOR 1

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

extern void Behavior_PlaySoundButtons(uint8_t button);

extern void Behavior_PlaySoundAlarm(uint8_t type);

#endif // BEHAVIOR_H_
