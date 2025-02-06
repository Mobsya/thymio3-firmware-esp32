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
//! \author  Vincent Gonet, Stefano Morgani
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
  int16_t Motors[2];   //!< Correction factor of the left motor [0] and right motor [1]
  int16_t OffsetGyro;  //!< Offset factor of the gyroscope
  int16_t White[3];    //!< Offset for color sensor white red, green, blue
  int16_t Black[3];    //!< Offset for color sensor black red, green, blue
  int16_t RC5Address;  //!< RC5 default address
  float MotFwBw;       //!< Forward to backward motors correction
  int16_t ZeroOffGyro[3]; //!< axes gyro offsets
  int16_t GyroRotFactor; //!< gyro rotation factor
  int16_t GroundBlack[2]; //!< ground left black, ground right black
  int16_t GroundWhite[2]; //!< ground left white, ground right white
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

//! \brief     Set the left and right motors correction
//! \pre       First initialize the settings
//! \param     values - motors corrrections (left, right)
//! \return    None
extern void Settings_SetMotorsSettings(int16_t* values);

//! \brief     Set the volume (do not save in flash)
//! \pre       First initialize the settings
//! \param     volume
//! \return    None
extern void Settings_SetVolumeSettings(int16_t volume);

//! \brief     Set the gyro offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetOffsetGyroSettings(int16_t offset);

//! \brief     Set the white red offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetWhiteRedSettings(int16_t offset);

//! \brief     Set the white green offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetWhiteGreenSettings(int16_t offset);

//! \brief     Set the white blue offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetWhiteBlueSettings(int16_t offset);

//! \brief     Set the white r,g,b offsets (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_SetWhiteSettings(int16_t* values);

//! \brief     Set the black red offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetBlackRedSettings(int16_t offset);

//! \brief     Set the black green offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetBlackGreenSettings(int16_t offset);

//! \brief     Set the black blue offset
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetBlackBlueSettings(int16_t offset);

//! \brief     Set the black r,g,b offsets (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_SetBlackSettings(int16_t* values);

//! \brief     Set the RC5 address (do not save in flash)
//! \pre       First initialize the settings
//! \param     addr
//! \return    None
extern void Settings_SetRC5AddressSettings(int16_t addr);

//! \brief     Set the forward to backward motors correction (do not save in flash)
//! \pre       First initialize the settings
//! \param     addr
//! \return    None
extern void Settings_SetMotFwBwSettings(float factor);

//! \brief     Set the gyro x axis offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetOffsetGyroXSettings(int16_t offset);

//! \brief     Set the gyro y axis offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetOffsetGyroYSettings(int16_t offset);

//! \brief     Set the gyro z axis offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offset
//! \return    None
extern void Settings_SetOffsetGyroZSettings(int16_t offset);

//! \brief     Set the gyro axes offsets (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_SetZeroOffGyroSettings(int16_t* values);

//! \brief     Set the gyro rotation factor (do not save in flash)
//! \pre       First initialize the settings
//! \param     factor
//! \return    None
extern void Settings_SetGyroRotFactorSettings(int16_t factor);

//! \brief     Set the ground black offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_SetGroundBlackSettings(int16_t* values);

//! \brief     Set the ground white offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_SetGroundWhiteSettings(int16_t* values);

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

//! \brief     Get motors corrections
//! \pre       First initialize the settings
//! \param     motors correction (left, right)
//! \return    None
extern void Settings_GetMotorsSettings(int16_t* values);

//! \brief     Get the volume stored in flash
//! \pre       First initialize the settings
//! \param     None
//! \return    Volume
extern int16_t Settings_GetVolumeSettings(void);

//! \brief     Get the gyro offset
//! \pre       First initialize the settings
//! \param     None
//! \return    offset
extern int16_t Settings_GetOffsetGyroSettings(void);

//! \brief     Get the white red offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetWhiteRedSettings(void);

//! \brief     Get the white green offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetWhiteGreenSettings(void);

//! \brief     Get the white blue offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetWhiteBlueSettings(void);

//! \brief     Get the white r,g,b offsets
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_GetWhiteSettings(int16_t* values);

//! \brief     Get the black red offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetBlackRedSettings(void);

//! \brief     Get the black green offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetBlackGreenSettings(void);

//! \brief     Get the black blue offset
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_GetBlackBlueSettings(void);

//! \brief     Get the black r,g,b offsets
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_GetBlackSettings(int16_t* values);

//! \brief     Get the RC5 address
//! \pre       First initialize the settings
//! \param     None
//! \return    RC5 address
extern int16_t Settings_GetRC5AddressSettings(void);

//! \brief     Get the forward to backward motors correction
//! \pre       First initialize the settings
//! \param     None
//! \return    Forward to backward motors correction
extern float Settings_GetMotFwBwSettings(void);

//! \brief     Get the gyro x axis offset
//! \pre       First initialize the settings
//! \param     None
//! \return    offset
extern int16_t Settings_GetOffsetGyroXSettings(void);

//! \brief     Get the gyro y axis offset
//! \pre       First initialize the settings
//! \param     None
//! \return    offset
extern int16_t Settings_GetOffsetGyroYSettings(void);

//! \brief     Get the gyro z axis offset
//! \pre       First initialize the settings
//! \param     None
//! \return    offset
extern int16_t Settings_GetOffsetGyroZSettings(void);

//! \brief     Get the gyro axes offsets
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_GetZeroOffGyroSettings(int16_t* values);

//! \brief     Get the gyro rotation factor (do not save in flash)
//! \pre       First initialize the settings
//! \param     factor
//! \return    None
extern int16_t Settings_GetGyroRotFactorSettings(void);

//! \brief     Get the ground black offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_GetGroundBlackSettings(int16_t* values);

//! \brief     Get the ground white offset (do not save in flash)
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_GetGroundWhiteSettings(int16_t* values);

//! \brief     Load the motors settings value, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadMotorsFile(void);

//! \brief     Load the offset gyroscope settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadOffsetGyroFile(void);

//! \brief     Load the volume settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadVolumeFile(void);

//! \brief     Load the white (red, green, blue) settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadWhiteFile(void);

//! \brief     Load the black (red, green, blue) settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadBlackFile(void);

//! \brief     Load the remote address settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadRC5AddressFile(void);

//! \brief     Load the forward to backward motors correction settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadMotFwBwFile(void);

//! \brief     Load the gyro axes offsets settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadZeroOffGyroFile(void);

//! \brief     Load the gyro rotation factor settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadGyroRotFactorFile(void);

//! \brief     Load the grounds black offsets settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadGroundBlackFile(void);

//! \brief     Load the grounds white offsets settings file, if it doesn't exist create it with default values.
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_LoadGroundWhiteFile(void);

//! \brief     Write the motors corrections to the settings file
//! \pre       First initialize the settings
//! \param     values - motors corrections (left, right)
//! \return    None
extern void Settings_WriteMotors(int16_t* values);

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

//! \brief     Write the white (red, green, blue) offsets settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_WriteWhite(int16_t* values);

//! \brief     Write the black (red, green, blue) offsets settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_WriteBlack(int16_t* values);

//! \brief     Write the remote address to the settings file
//! \pre       First initialize the settings
//! \param     address - Remote address
//! \return    None
extern void Settings_WriteRC5Address(int16_t address);

//! \brief     Write the forward to backward motors correction settings file
//! \pre       First initialize the settings
//! \param     factor - forward to backward motors correction
//! \return    None
extern void Settings_WriteMotFwBwFactor(float factor);

//! \brief     Write the gyro axes offsets settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_WriteZeroOffGyro(int16_t* values);

//! \brief     Write the gyro rotation factor settings file
//! \pre       First initialize the settings
//! \param     factor - rotation factor
//! \return    None
extern void Settings_WriteGyroRotFactor(int16_t factor);

//! \brief     Write the grounds black offsets settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_WriteGroundBlack(int16_t* offsets);

//! \brief     Write the grounds white offsets settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_WriteGroundWhite(int16_t* offsets);

//! \brief     Read the motors corrections from the settings file
//! \pre       First initialize the settings
//! \param     values - motors corrections (left, right)
//! \return    None
extern void Settings_ReadMotors(int16_t* values);

//! \brief     Read the offset gyroscope value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadOffsetGyro(void);

//! \brief     Read the volume value from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadVolume(void);

//! \brief     Read the white (red, green, blue) offsets from the settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_ReadWhite(int16_t* values);

//! \brief     Read the black (red, green, blue) offsets from the settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_ReadBlack(int16_t* values);

//! \brief     Read the remote address from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadRC5Address(void);

//! \brief     Read the forward to backward motors correction from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    forward to backward motors correction
extern float Settings_ReadMotFwBwFactor(void);

//! \brief     Read the gyro axes offsets from the settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_ReadZeroOffGyro(int16_t* values);

//! \brief     Read the gyro rotation factor from the settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern int16_t Settings_ReadGyroRotFactor();

//! \brief     Read the ground black offsets from the settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_ReadGroundBlack(int16_t* values);

//! \brief     Read the ground white offsets from the settings file
//! \pre       First initialize the settings
//! \param     offsets
//! \return    None
extern void Settings_ReadGroundWhite(int16_t* values);

//! \brief     Erase the motors settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseMotorsFile(void);

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

//! \brief     Erase the white settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseWhiteFile(void);

//! \brief     Erase the black settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseBlackFile(void);

//! \brief     Erase the remote address settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseRC5AddressFile(void);

//! \brief     Erase the forward to backward motors correction settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseMotFwBwFile(void);

//! \brief     Erase the gyro axes offsets settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseZeroOffGyroFile(void);

//! \brief     Erase the gyro rotation factor settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseGyroRotFactorFile(void);

//! \brief     Erase the ground black offsets settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseGroundBlackFile(void);

//! \brief     Erase the ground white offsets settings file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
extern void Settings_EraseGroundWhiteFile(void);

#endif // SETTINGS_H_
