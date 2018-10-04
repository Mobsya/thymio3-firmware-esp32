//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm303c.h
//! \brief   This module provides the useful functions to use the LSM303C device
//!          (3D accelerometer and 3D magnetometer)
//!
//! \author  Vincent Gonet
//!
//! \version $Id: lsm303c.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef LSM303C_H_
#define LSM303C_H_

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
  uint16_t X;
  uint16_t Y;
  uint16_t Z;
} T_Acc_Axis;  //!< Axis

typedef struct
{
  uint16_t X;
  uint16_t Y;
  uint16_t Z;
} T_Mag_Axis;  //!< Axis

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the accelerometer of the LSM303C device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM303C_InitAccelerometer(void);

//! \brief     Check the accelerometer manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
extern void LSM303C_CheckAccManufacturerId(void);

//! \brief     Get the acceleration in [???]
//! \pre       First initialize the LSM303C device
//! \param     None
//! \return    None
extern T_Acc_Axis* LSM303C_GetAcceleration(void);

//! \brief     Initialize the magnetometer of the LSM303C device
//! \pre       None
//! \param     None
//! \return    None
extern void LSM303C_InitMagnetometer(void);

//! \brief     Check the magnetometer manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
extern void LSM303C_CheckMagManufacturerId(void);

//! \brief     Get the magnetic field in [???]
//! \pre       First initialize the LSM303C device
//! \param     None
//! \return    None
extern T_Mag_Axis* LSM303C_GetMagneticField(void);

#endif // LSM303C_H_
