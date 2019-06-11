//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm6ds3us.h
//! \brief   This module provides the useful functions to use the LSM6DS3US device
//!          (3D accelerometer and 3D gyroscope)
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef LSM6DS3US_H_
#define LSM6DS3US_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int16_t X;
  int16_t Y;
  int16_t Z;
} T_Axis;  //!< Axis

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the accelerometer of the LSM6DS3US device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3US_InitAccelerometer(void);

//! \brief     Get the acceleration in [???]
//! \pre       First initialize the LSM6DS3US device
//! \param     acceleration - Acceleration
//! \return    None
extern void LSM6DS3US_GetAcceleration(T_Axis* acceleration);

//! \brief     Get the acceleration tap source
//! \pre       First initialize the LSM6DS3US device
//! \param     source - Source of the tap
//! \return    None
extern void LSM6DS3US_GetTapSource(uint8_t* source);

//! \brief     Initialize the gyroscope of the LSM6DS3US device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3US_InitGyroscope(void);

//! \brief     Get the angular position in [???]
//! \pre       First initialize the LSM6DS3US device
//! \param     angularPosition - Angular position
//! \return    None
extern void LSM6DS3US_GetAngularPosition(T_Axis* angularPosition);

//! \brief     Check the manufacturer ID
//! \pre       None
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Acc_InvalidID
extern T_Error LSM6DS3US_CheckManufacturerId(void);

#endif // LSM6DS3US_H_
