//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    common.h
//! \brief   This module provides the common mode functions
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef COMMON_H_
#define COMMON_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MIN_LIMIT_SPEED        -600
#define MAX_LIMIT_SPEED         600

#define when(cond) if(({static unsigned char prev; \
                        unsigned char c = !!(cond); \
                        unsigned char result = c && !prev; \
                        prev = c; \
                        result;}))

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

//! \brief     Get the body color pulse
//! \pre       None
//! \param     None
//! \return    The body color pulse
extern uint8_t Common_GetBodyColorPulse(void);

//! \brief     Set the target speed
//! \pre       None
//! \param     left - Left target speed
//! \param     right - Right target speed
//! \return    None
extern void Common_SetTargetSpeed(int16_t left, int16_t right);

//! \brief     Limit the speed applied
//! \pre       None
//! \param     min - Minimum speed allowed
//! \param     max - Maximum speed allowed
//! \return    None
extern void Common_LimitSpeed(int16_t min, int16_t max);

//! \brief     Handle the positive speed applied
//! \pre       None
//! \param     speed - Speed
//! \return    None
extern void Common_HandlePositiveSpeed(int16_t speed);

//! \brief     Handle the negative speed applied
//! \pre       None
//! \param     speed - Speed
//! \return    None
extern void Common_HandleNegativeSpeed(int16_t speed);

//! \brief     Set the speed by using the capacitive buttons
//! \pre       None
//! \param     speed - Speed
//! \param     increment - Increment added (or subtracted) to speed according to the button pressed
//! \param     max - Maximum speed allowed
//! \param     min - Minimum speed allowed
//! \return    None
extern void Common_SetSpeedUsingButtons(int16_t* speed, int16_t increment, int16_t max, int16_t min);

//! \brief     Handle the edge table detection
//! \pre       None
//! \param     red - Red brightness of the body LEDs
//! \param     green - Green brightness of the body LEDs
//! \param     blue - Blue brightness of the body LEDs
//! \return    None
extern void Common_HandleTableEdgeDetection(uint8_t red, uint8_t green, uint8_t blue);

#endif // COMMON_H_
