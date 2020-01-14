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

#include "aseba_esp32.h"
#include "file_system.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DEFAULT_LEFT_MOTOR      256
#define DEFAULT_RIGHT_MOTOR     256
#define DEFAULT_OFFSET_GYRO       0

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

static T_Settings Settings;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "settings";

static const char* FileLeftMotor  = "/spiffs/left_motor.dat";
static const char* FileRightMotor = "/spiffs/right_motor.dat";
static const char* FileOffsetGyro = "/spiffs/offset_gyro.dat";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void WriteFactoryLeftMotor(void);

static void WriteFactoryRightMotor(void);

static void WriteFactoryOffsetGyro(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Settings_Init(void)
{
  Settings.LeftMotor  = DEFAULT_LEFT_MOTOR;
  Settings.RightMotor = DEFAULT_RIGHT_MOTOR;
  Settings.OffsetGyro = DEFAULT_OFFSET_GYRO;

  ESP_LOGI(Tag, "Settings are initialized");
}

//_____________________________________________________________________________

void Settings_UpdateSettings(void)
{
  Settings.LeftMotor  = vmVariables.settings[0];
  Settings.RightMotor = vmVariables.settings[1];
  //Settings.OffsetGyro = vmVariables.settings[2];
}

//_____________________________________________________________________________
#if 0
void Settings_SetSettings(int16_t value, uint16_t position)
{
  if (position == 0u)
  {
    Settings.LeftMotor = value;
  }
  else if (position == 1u)
  {
    Settings.RightMotor = value;
  }
  else if (position == 2u)
  {
    Settings.OffsetGyro = value;
  }
  else
  {
    // Do nothing
  }
}
#endif
//_____________________________________________________________________________
#if 0
void Settings_SetSettings(int16_t* buffer, uint16_t position)
{
  Settings.LeftMotor  = buffer[position];
  Settings.RightMotor = buffer[position + 1u];
  //Settings.OffsetGyro = buffer[position + 2u];
}
#endif
//_____________________________________________________________________________

int16_t Settings_GetLeftMotorSettings(void)
{
  return Settings.LeftMotor;
}

//_____________________________________________________________________________

int16_t Settings_GetRightMotorSettings(void)
{
  return Settings.RightMotor;
}

//_____________________________________________________________________________

void Settings_CreateLeftMotorFile(void)
{
  if (FileSystem_CreateFile(FileLeftMotor))
  {
    WriteFactoryLeftMotor();
  }
}

//_____________________________________________________________________________

void Settings_CreateRightMotorFile(void)
{
  if (FileSystem_CreateFile(FileRightMotor))
  {
    WriteFactoryRightMotor();
  }
}

//_____________________________________________________________________________

void Settings_CreateOffsetGyroFile(void)
{
  if (FileSystem_CreateFile(FileOffsetGyro))
  {
    WriteFactoryOffsetGyro();
  }
}

//_____________________________________________________________________________

void Settings_WriteLeftMotor(int16_t leftMotor)
{
  int16_t input = leftMotor;

  FileSystem_Write(FileLeftMotor, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteRightMotor(int16_t rightMotor)
{
  int16_t input = rightMotor;

  FileSystem_Write(FileRightMotor, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteOffsetGyro(int16_t offsetGyro)
{
  int16_t input = offsetGyro;

  FileSystem_Write(FileOffsetGyro, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

int16_t Settings_ReadOffsetGyro(void)
{
  int16_t output = 0;

  FileSystem_Read(FileOffsetGyro, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

void Settings_EraseLeftMotor(void)
{
  FileSystem_EraseFile(FileLeftMotor);
}

//_____________________________________________________________________________

void Settings_EraseRightMotor(void)
{
  FileSystem_EraseFile(FileRightMotor);
}

//_____________________________________________________________________________

void Settings_EraseOffsetGyro(void)
{
  FileSystem_EraseFile(FileOffsetGyro);
}

//_____________________________________________________________________________

static void WriteFactoryLeftMotor(void)
{
  int16_t input = DEFAULT_LEFT_MOTOR;

  FileSystem_Write(FileLeftMotor, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryRightMotor(void)
{
  int16_t input = DEFAULT_RIGHT_MOTOR;

  FileSystem_Write(FileRightMotor, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryOffsetGyro(void)
{
  int16_t input = DEFAULT_OFFSET_GYRO;

  FileSystem_Write(FileOffsetGyro, &input, sizeof(int16_t));
}
