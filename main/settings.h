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

extern void Settings_UpdateSettings(void);

//extern void Settings_SetSettings(int16_t* buffer, uint16_t position);

extern int16_t Settings_GetLeftMotorSettings(void);

extern int16_t Settings_GetRightMotorSettings(void);

//! \brief     Create the settings file for the left motor
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateLeftMotorFile(void);

//! \brief     Create the settings file for the right motor
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateRightMotorFile(void);

//! \brief     Create the settings file for the offset gyro
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateOffsetGyroFile(void);

//! \brief     Write the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_WriteLeftMotor(int16_t leftMotor);

extern void Settings_WriteRightMotor(int16_t rightMotor);

extern void Settings_WriteOffsetGyro(int16_t offsetGyro);

//! \brief     Create the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateFile(void);

extern void Settings_Write(int16_t leftMotor, int16_t rightMotor, int16_t offsetGyro);

extern int16_t Settings_ReadOffsetGyro(void);

extern void Settings_EraseLeftMotor(void);

extern void Settings_EraseRightMotor(void);

extern void Settings_EraseOffsetGyro(void);

#endif // SETTINGS_H_
