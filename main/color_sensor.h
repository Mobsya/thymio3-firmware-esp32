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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef COLOR_SENSOR_H_
#define COLOR_SENSOR_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "error.h"
#include "bh1745nuc.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Color_Red,
  E_Color_Yellow,
  E_Color_Green,
  E_Color_Blue,
  E_Color_Purple,
  E_Color_White,
  E_Color_Black,
  E_Color_Unknown
} T_Color;

typedef struct
{
  int16_t Hue;
  int16_t Saturation;
  int16_t Value;
} T_HSV;
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
//! \param     calibrationStatus - Calibration status to update
//! \return    None
extern bool ColorSensor_Calibrate(uint8_t choice, uint8_t* calibrationStatus);

//! \brief     Check the color sensor
//! \pre       First initialize the color sensor
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Color_InvalidID
extern T_Error ColorSensor_CheckManufacturerId(void);

//! \brief     Get the HSV values
//! \pre       First initialize the color sensor
//! \param     None
//! \return    HSV
extern T_HSV ColorSensor_GetHsv(void);

//! \brief     Get the RGB raw values
//! \pre       First initialize the color sensor
//! \param     None
//! \return    T_RawColor
extern T_RawColor ColorSensor_GetRaw(void);

//! \brief     Get the white calibration values (calibration done in a white surface).
//! \pre       First initialize the color sensor
//! \param     None
//! \return    T_RawColor
extern T_RawColor ColorSensor_GetWhiteCalibration(void);

//! \brief     Get the black calibration values (calibration done in a black surface).
//! \pre       First initialize the color sensor
//! \param     None
//! \return    T_RawColor
extern T_RawColor ColorSensor_GetBlackCalibration(void);

//! \brief     Calibrate the white
//! \pre       First initialize the color sensor
//! \return    None
void ColorSensor_CalibrateWhite(void);

//! \brief     Calibrate the black
//! \pre       First initialize the color sensor
//! \return    None
void ColorSensor_CalibrateBlack(void);

#endif // COLOR_SENSOR_H_
