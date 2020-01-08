//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    settings.h
//! \brief   This module provides the useful functions to use the settings
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SETTINGS_H_
#define SETTINGS_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#define DEFAULT_LEFT_MOTOR      256
#define DEFAULT_RIGHT_MOTOR     256
#define DEFAULT_OFFSET_GYRO       0

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

//! \brief     Initialize the settings
//! \pre       None
//! \param     None
//! \return    None
extern void Settings_Init(void);

//! \brief     Create the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateFile(void);

extern void Settings_Write(int16_t leftMotor, int16_t rightMotor, int16_t offsetGyro);

extern int16_t Settings_ReadOffsetGyro(void);

extern void Settings_Erase(void);

#endif // SETTINGS_H_
