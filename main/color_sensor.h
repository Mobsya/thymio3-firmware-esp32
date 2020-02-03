//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    color_sensor.h
//! \brief   This module provides the useful functions to use the color sensor
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef COLOR_SENSOR_H_
#define COLOR_SENSOR_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Color_Red,
  E_Color_Orange,
  E_Color_Yellow,
  E_Color_Green,
  E_Color_Cyan,
  E_Color_Blue,
  E_Color_Purple,
  E_Color_White,
  E_Color_Unknown
} T_Color;

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
extern void ColorSensor_Init(void);

//! \brief     Read the color
//! \pre       First initialize the color sensor
//! \param     None
//! \return    None
extern void ColorSensor_ReadColor(void);

//! \brief     Get the color
//! \pre       First initialize the color sensor
//! \param     None
//! \return    Color
extern T_Color ColorSensor_GetColor(void);

//! \brief     Calibrate the colors
//! \pre       First initialize the color sensor
//! \param     choice - 0 = white, 1 = black
//! \return    None
extern void ColorSensor_Calibrate(uint8_t choice);

//! \brief     Check the color sensor
//! \pre       First initialize the color sensor
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Color_InvalidID
extern T_Error ColorSensor_CheckManufacturerId(void);

#endif // COLOR_SENSOR_H_
