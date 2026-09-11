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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#define LSM6DS3US 0
#define LSM6DS3TR 1
#define LSM6DS0 2
#define ACC_NOT_AVAILABLE 3

#include "error.h"
#include "imu_common.h"
#include "lsm6ds3us.h"
#include "lsm6ds3tr.h"
#include "lsm6ds0.h"

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

//! \brief     Read the raw acceleration directly from the sensor, without applying the hardware
//!            version axes correction and without updating the Aseba variables
//! \pre       First initialize the accelerometer. The caller must handle the I2C mutex.
//! \param     acceleration - Raw acceleration values read from the sensor
//! \return    True if the acceleration has been read, false if the accelerometer is not available
extern bool Accelerometer_ReadRawAcceleration(T_Axis* acceleration);

//! \brief     Get last acceleration read
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    Raw acceleration values
extern T_Axis Accelerometer_GetAcceleration(void);

//! \brief     Get the acceleration on Y-axis
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    Acceleration on the Y-axis
extern int16_t Accelerometer_GetAccelerationY(void);

//! \brief     Get the acceleration on Z-axis
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    Acceleration on the Z-axis
extern int16_t Accelerometer_GetAccelerationZ(void);

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

//! \brief     Is a tap detected (from interrupt)
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    True if a tap has been detected by the interrupt, false otherwise
extern bool Accelerometer_IsTapDetected(void);

//! \brief     Clear the acceleration tap status
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern void Accelerometer_ClearTapStatus(void);

//! \brief     Is a free fall detected (from interrupt)
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    True if a free fall has been detected by the interrupt, false otherwise
extern bool Accelerometer_IsFreeFallDetected(void);

//! \brief     Check the accelerometer
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Acc_InvalidID
extern T_Error Accelerometer_CheckManufacturerId(void);

#endif // ACCELEROMETER_H_
