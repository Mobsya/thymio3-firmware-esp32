//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    accelerometer.h
//! \brief   This module provides the useful functions to use the accelerometer
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

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

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the accelerometer
//! \pre       None
//! \param     None
//! \return    None
extern void Accelerometer_Init(void);

//! \brief     Read the acceleration
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern void Accelerometer_ReadAcceleration(void);

//! \brief     Get the acceleration on Y-axis
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    Acceleration on the Y-axis
extern int16_t Accelerometer_GetAccelerationY(void);

//! \brief     Read the acceleration tap source
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern void Accelerometer_ReadTapSource(void);

//! \brief     Get the acceleration tap source
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern uint8_t Accelerometer_GetTapSource(void);

//! \brief     Check the accelerometer
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Acc_InvalidID
extern T_Error Accelerometer_CheckManufacturerId(void);

#endif // ACCELEROMETER_H_
