//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm6ds3tr.h
//! \brief   This module provides the useful functions to use the LSM6DS3TR device
//!          (3D accelerometer and 3D gyroscope)
//!
//! \author  Vincent Gonet, Michael Bonani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef LSM6DS3TR_H_
#define LSM6DS3TR_H_

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

//! \brief     Initialize the accelerometer of the LSM6DS3TR device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3TR_InitAccelerometer(void);

//! \brief     Get the acceleration in [???]
//! \pre       First initialize the LSM6DS3TR device
//! \param     acceleration - Acceleration
//! \return    None
//! \image     html ReadAcceleration.svg
extern void LSM6DS3TR_ReadAcceleration(T_Axis* acceleration);

//! \brief     Get the acceleration tap source
//! \pre       First initialize the LSM6DS3TR device
//! \param     source - Source of the tap
//! \return    None
extern void LSM6DS3TR_ReadTapSource(uint8_t* source);

//! \brief     Initialize the gyroscope of the LSM6DS3TR device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM6DS3TR_InitGyroscope(int16_t offset);

//! \brief     Get the angular velocity in [???]
//! \pre       First initialize the LSM6DS3TR device
//! \param     angularPosition - Angular position
//! \return    None
//! \image     html ReadAngle.svg
extern void LSM6DS3TR_ReadAngularVelocity(T_Axis* angularVelocity);

//! \brief     Read the buffered data from the gyroscope
//! \pre       First initialize the LSM6DS3TR device
//! \param     None
//! \return    Number of samples read
extern uint16_t LSM6DS3TR_ReadBufferedAngularPosition(void);

extern void LSM6DS3TR_SetOffset(int32_t offset);

//! \brief     Check the manufacturer ID
//! \pre       None
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Acc_InvalidID
extern T_Error LSM6DS3TR_CheckManufacturerId(void);

#endif // LSM6DS3TR_H_
