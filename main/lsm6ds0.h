//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm6ds0.h
//! \brief   This module provides the useful functions to use the LSM6DS0 device
//!          (3D accelerometer and 3D gyroscope)
//!
//! \author  Vincent Gonet, Michael Bonani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef LSM6DS0_H_
#define LSM6DS0_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "error.h"
#include "gyroscope.h"

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

//! \brief     Initialize the accelerometer of the LSM6DS0 device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS0_InitAccelerometer(void);

//! \brief     Get the acceleration in [???]
//! \pre       First initialize the LSM6DS0 device
//! \param     acceleration - Acceleration
//! \return    None
extern void LSM6DS0_ReadAcceleration(T_Axis* acceleration);

//! \brief     Get the acceleration tap source
//! \pre       First initialize the LSM6DS0 device
//! \param     source - Source of the tap
//! \return    None
extern void LSM6DS0_ReadTapSource(uint8_t* source);

//! \brief     Initialize the gyroscope of the LSM6DS0 device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS0_InitGyroscope(int16_t offset);

//! \brief     Get the angular velocity in [???]
//! \pre       First initialize the LSM6DS0 device
//! \param     angularPosition - Angular position
//! \return    None
//! \image     html ReadAngle.svg
extern void LSM6DS0_ReadAngularVelocity(T_Axis* angularVelocity);

//! \brief     Read the buffered data from the gyroscope
//! \pre       First initialize the LSM6DS0 device
//! \param     None
//! \return    Number of samples read
extern uint16_t LSM6DS0_ReadBufferedAngularPosition(void);

extern void LSM6DS0_SetOffset(int32_t offset);

//! \brief     Check the manufacturer ID
//! \pre       None
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Acc_InvalidID
extern T_Error LSM6DS0_CheckManufacturerId(void);

#endif // LSM6DS0_H_
