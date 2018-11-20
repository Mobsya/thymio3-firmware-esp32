//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: accelerometer.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

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

//! \brief     Initialize the accelerometer
//! \pre       None
//! \param     None
//! \return    None
extern void Accelerometer_Init(void);

//! \brief     Get the acceleration
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern void Accelerometer_GetAcceleration(void);

//! \brief     Get the acceleration tap source
//! \pre       First initialize the accelerometer
//! \param     None
//! \return    None
extern void Accelerometer_GetTapSource(void);

#endif // ACCELEROMETER_H_
