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

typedef struct
{
  int16_t Volume;      //!< Audio Volume
  int16_t LeftMotor;   //!< Correction factor of the left motor
  int16_t RightMotor;  //!< Correction factor of the right motor
  int16_t OffsetGyro;  //!< Offset factor of the gyroscope
} T_Settings;

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

//! \brief     Update the settings from Aseba
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_UpdateSettings(void);

//! \brief     Set the left motor correction
//! \pre       First initialize the settings
//! \param     leftMotor - Left motor correction
//! \return    None
extern void Settings_SetLeftMotorSettings(int16_t leftMotor);

//! \brief     Set the right motor correction
//! \pre       First initialize the settings
//! \param     rightMotor - Right motor correction
//! \return    None
extern void Settings_SetRightMotorSettings(int16_t rightMotor);

//! \brief     Get the left motor correction
//! \pre       First initialize the settings
//! \param     None
//! \return    Left motor correction
extern int16_t Settings_GetLeftMotorSettings(void);

//! \brief     Get the right motor correction
//! \pre       First initialize the settings
//! \param     None
//! \return    Right motor correction
extern int16_t Settings_GetRightMotorSettings(void);

//! \brief     Create the left motor settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateLeftMotorFile(void);

//! \brief     Create the right motor settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateRightMotorFile(void);

//! \brief     Create the offset gyroscope settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateOffsetGyroFile(void);

//! \brief     Create the volume settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateVolumeFile(void);

//! \brief     Create the white (red) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateWhiteRedFile(void);

//! \brief     Create the white (green) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateWhiteGreenFile(void);

//! \brief     Create the white (blue) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateWhiteBlueFile(void);

//! \brief     Create the black (red) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateBlackRedFile(void);

//! \brief     Create the black (green) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateBlackGreenFile(void);

//! \brief     Create the black (blue) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_CreateBlackBlueFile(void);

//! \brief     Write the left motor correction to the settings file
//! \pre       First initialize the settings
//! \param     leftMotor - Left motor correction
//! \return    None
extern void Settings_WriteLeftMotor(int16_t leftMotor);

//! \brief     Write the right motor correction to the settings file
//! \pre       First initialize the settings
//! \param     rightMotor - Right motor correction
//! \return    None
extern void Settings_WriteRightMotor(int16_t rightMotor);

//! \brief     Write the offset gyroscope value to the settings file
//! \pre       First initialize the settings
//! \param     offsetGyro - Offset gyroscope value
//! \return    None
extern void Settings_WriteOffsetGyro(int16_t offsetGyro);

//! \brief     Write the volume value to the settings file
//! \pre       First initialize the settings
//! \param     volume - Volume value
//! \return    None
extern void Settings_WriteVolume(int16_t volume);

//! \brief     Write the white (red) value to the settings file
//! \pre       First initialize the settings
//! \param     whiteRed - White (red) value
//! \return    None
extern void Settings_WriteWhiteRed(int16_t whiteRed);

//! \brief     Write the white (green) value to the settings file
//! \pre       First initialize the settings
//! \param     whiteGreen - White (green) value
//! \return    None
extern void Settings_WriteWhiteGreen(int16_t whiteGreen);

//! \brief     Write the white (blue) value to the settings file
//! \pre       First initialize the settings
//! \param     whiteBlue - White (blue) value
//! \return    None
extern void Settings_WriteWhiteBlue(int16_t whiteBlue);

//! \brief     Write the black (red) value to the settings file
//! \pre       First initialize the settings
//! \param     blackRed - Black (red) value
//! \return    None
extern void Settings_WriteBlackRed(int16_t blackRed);

//! \brief     Write the black (green) value to the settings file
//! \pre       First initialize the settings
//! \param     blackGreen - Black (green) value
//! \return    None
extern void Settings_WriteBlackGreen(int16_t blackGreen);

//! \brief     Write the black (blue) value to the settings file
//! \pre       First initialize the settings
//! \param     blackBlue - Black (blue) value
//! \return    None
extern void Settings_WriteBlackBlue(int16_t blackBlue);

//! \brief     Write the left motor correction from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadLeftMotor(void);

//! \brief     Write the right motor correction from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadRightMotor(void);

//! \brief     Write the offset gyroscope value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadOffsetGyro(void);

//! \brief     Write the volume value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadVolume(void);

//! \brief     Write the white (red) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadWhiteRed(void);

//! \brief     Write the white (green) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadWhiteGreen(void);

//! \brief     Write the white (blue) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadWhiteBlue(void);

//! \brief     Write the black (red) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadBlackRed(void);

//! \brief     Write the black (green) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadBlackGreen(void);

//! \brief     Write the black (blue) value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadBlackBlue(void);

//! \brief     Erase the left motor settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseLeftMotorFile(void);

//! \brief     Erase the right motor settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseRightMotorFile(void);

//! \brief     Erase the offset gyroscope settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseOffsetGyroFile(void);

//! \brief     Erase the volume settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseVolumeFile(void);

//! \brief     Erase the white (blue) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseWhiteRedFile(void);

//! \brief     Erase the white (green) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseWhiteGreenFile(void);

//! \brief     Erase the white (blue) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseWhiteBlueFile(void);

//! \brief     Erase the black (red) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseBlackRedFile(void);

//! \brief     Erase the black (green) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseBlackGreenFile(void);

//! \brief     Erase the black (blue) settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseBlackBlueFile(void);

#endif // SETTINGS_H_
