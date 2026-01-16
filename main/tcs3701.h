//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ts3701.h
//! \brief   This module provides the useful functions to use the color sensor TCS3701
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef TCS3701_H_
#define TCS3701_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#include "color_sensor.h"
#include <stdint.h>

#include "error.h"


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

//! \brief     Initialize the color sensor
//! \pre       None
//! \param     None
//! \return    None
extern void TCS3701_Init(void);

//! \brief     Check the device ID
//! \pre       First initialize the color sensor
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Color_InvalidID
extern T_Error TCS3701_CheckDeviceId(void);

//! \brief     Read the RGBC raw color
//! \pre       First initialize the color sensor
//! \param     raw - Raw color RGBC in [lux]
//! \return    Nones
extern void TCS3701_ReadColor(T_RawColor* raw);

#endif // TCS3701_H_
