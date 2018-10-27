//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: lsm6ds3us.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef LSM6DS3US_H_
#define LSM6DS3US_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

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

//! \brief     Check the manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3US_CheckManufacturerId(void);

//! \brief     Get the acceleration in [???]
//! \pre       First initialize the LSM6DS3US device
//! \param     None
//! \return    None
extern void LSM6DS3US_GetAcceleration(T_Axis* acceleration);

//! \brief     Initialize the gyroscope of the LSM6DS3US device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3US_InitGyroscope(void);

//! \brief     Get the angular position in [???]
//! \pre       First initialize the LSM6DS3US device
//! \param     None
//! \return    None
extern void LSM6DS3US_GetAngularPosition(T_Axis* angularPosition);

#endif // LSM6DS3US_H_
