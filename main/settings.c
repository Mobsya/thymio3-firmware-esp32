//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    settings.c
//! \brief   This module provides the useful functions to use the settings
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "settings.h"

#include "file_system.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int16_t LeftMotor;   //!< Correction factor of the left motor
  int16_t RightMotor;  //!< Correction factor of the right motor
  int16_t OffsetGyro;  //!< Offset factor of the gyroscope
} T_Settings;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

static T_Settings Input;
static T_Settings Output;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "settings";

static const char* Filename = "/spiffs/settings.dat";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void WriteFactorySettings(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Settings_Init(void)
{
  Input.LeftMotor  = DEFAULT_LEFT_MOTOR;
  Input.RightMotor = DEFAULT_RIGHT_MOTOR;
  Input.OffsetGyro = DEFAULT_OFFSET_GYRO;

  ESP_LOGI(Tag, "Settings are initialized");
}

//_____________________________________________________________________________

void Settings_CreateFile(void)
{
  if (FileSystem_CreateFile(Filename))
  {
    WriteFactorySettings();
  }
}

//_____________________________________________________________________________

void Settings_Write(int16_t leftMotor, int16_t rightMotor, int16_t offsetGyro)
{
  int16_t size = sizeof(T_Settings);

  Input.LeftMotor  = leftMotor;
  Input.RightMotor = rightMotor;
  Input.OffsetGyro = offsetGyro;

  FileSystem_Write(Filename, &Input, size);
}

//_____________________________________________________________________________

int16_t Settings_ReadOffsetGyro(void)
{
  int16_t size = sizeof(T_Settings);

  FileSystem_Read(Filename, &Output, size);

  return Output.OffsetGyro;
}

//_____________________________________________________________________________

void Settings_Erase(void)
{
  FileSystem_EraseFile(Filename);
}

//_____________________________________________________________________________

static void WriteFactorySettings(void)
{
  int16_t size = sizeof(T_Settings);

  Input.LeftMotor  = DEFAULT_LEFT_MOTOR;
  Input.RightMotor = DEFAULT_RIGHT_MOTOR;
  Input.OffsetGyro = DEFAULT_OFFSET_GYRO;

  FileSystem_Write(Filename, &Input, size);
}
