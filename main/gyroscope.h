//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    gyroscope.h
//! \brief   This module provides the useful functions to use the gyroscope
//!
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef GYROSCOPE_H_
#define GYROSCOPE_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#define LSM6DS3US 0
#define LSM6DS3TR 1
#define LSM6DS0 2

#include "imu_common.h"
#include "lsm6ds3us.h"
#include "lsm6ds3tr.h"
#include "lsm6ds0.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define GYRO_BUFFER_SIZE 128

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

//! \brief     Initialize the gyroscope
//! \pre       None
//! \param     None
//! \return    None
extern void Gyroscope_Init(void);

//! \brief     Read the angular velocity
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
extern void Gyroscope_ReadAngularVelocity(void);

//! \brief     Read the angle
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
extern void Gyroscope_ReadAngle(void);

//! \brief     Get the angular velocity on Z-axis
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    Angular velocity on the Z-axis
extern int16_t Gyroscope_GetAngularVelocityZ(void);

//! \brief     Get the angle on Z-axis
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    Angle on the Z-axis
extern int16_t Gyroscope_GetAngleZ(void);

//! \brief     Get the angle in [degree] on Z-axis
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    Angle on the Z-axis
extern int16_t Gyroscope_GetAngleZ_deg(void);

//! \brief     Reset the angle
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
extern void Gyroscope_ResetAngle(void);

//! \brief     Reset the calibration
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
extern void Gyroscope_ResetCalibration(void);

//! \brief     Calibrate gyroscope
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    True if calibration performed correctly
extern bool Gyroscope_Calibrate(void);

//! \brief     Get current calibration values.
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    True if calibration performed correctly
extern void Gyroscope_GetCalibration(int16_t* values);

//! \brief     Set the offset
//! \pre       First initialize the gyroscope
//! \param     offset - Offset
//! \return    None
extern void Gyroscope_SetOffset(int32_t offset);

//! \brief     Get last angular velocities read
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    Raw angular velocities values
T_Axis Gyroscope_GetAngularVelocity(void);

//! \brief     Enable continuous gyro calibration
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
void Gyroscope_EnableContinuousCalib(void);

//! \brief     Disable continuous gyro calibration
//! \pre       First initialize the gyroscope
//! \param     None
//! \return    None
void Gyroscope_DisableContinuousCalib(void);

#endif // GYROSCOPE_H_
