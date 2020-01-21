//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    common.h
//! \brief   This module provides the useful functions to use the explorer mode
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

#define MIN_SPEED        -600
#define MAX_SPEED         600

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

extern uint8_t Common_GetBodyColorPulse(void);

extern void Common_LimitSpeed(int16_t min, int16_t max);

//! \brief     Handle the edge table detection
//! \pre       None
//! \param     red - Red brightness of the body LEDs
//! \param     green - Green brightness of the body LEDs
//! \param     blue - Blue brightness of the body LEDs
//! \return    None
extern void Common_HandleTableEdgeDetection(uint8_t red, uint8_t green, uint8_t blue);

#endif // COMMON_H_
