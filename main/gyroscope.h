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
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef GYROSCOPE_H_
#define GYROSCOPE_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

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

#endif // GYROSCOPE_H_
