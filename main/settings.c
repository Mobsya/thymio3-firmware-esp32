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
#define DEFAULT_VOLUME           80
#define DEFAULT_WHITE_RED      1140
#define DEFAULT_WHITE_GREEN    1350
#define DEFAULT_WHITE_BLUE     1110
#define DEFAULT_BLACK_RED       710
#define DEFAULT_BLACK_GREEN     940
#define DEFAULT_BLACK_BLUE      700

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

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
static const char* FileVolume     = "/spiffs/volume.dat";
static const char* FileWhiteRed   = "/spiffs/white_red.dat";
static const char* FileWhiteGreen = "/spiffs/white_green.dat";
static const char* FileWhiteBlue  = "/spiffs/white_blue.dat";
static const char* FileBlackRed   = "/spiffs/black_red.dat";
static const char* FileBlackGreen = "/spiffs/black_green.dat";
static const char* FileBlackBlue  = "/spiffs/black_blue.dat";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Write the default left motor correction to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryLeftMotor(void);

//! \brief     Write the default right motor correction to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryRightMotor(void);

//! \brief     Write the default gyroscope offset value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryOffsetGyro(void);

//! \brief     Write the default volume value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryVolume(void);

//! \brief     Write the default white (red) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryWhiteRed(void);

//! \brief     Write the default white (green) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryWhiteGreen(void);

//! \brief     Write the default white (blue) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryWhiteBlue(void);

//! \brief     Write the default black (red) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryBlackRed(void);

//! \brief     Write the default black (green) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryBlackGreen(void);

//! \brief     Write the default black (blue) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryBlackBlue(void);

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

void Settings_SetLeftMotorSettings(int16_t leftMotor)
{
  Settings.LeftMotor = leftMotor;
}

//_____________________________________________________________________________

void Settings_SetRightMotorSettings(int16_t rightMotor)
{
  Settings.RightMotor = rightMotor;
}

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

void Settings_CreateVolumeFile(void)
{
  if (FileSystem_CreateFile(FileVolume))
  {
    WriteFactoryVolume();
  }
}

//_____________________________________________________________________________

void Settings_CreateWhiteRedFile(void)
{
  if (FileSystem_CreateFile(FileWhiteRed))
  {
    WriteFactoryWhiteRed();
  }
}

//_____________________________________________________________________________

void Settings_CreateWhiteGreenFile(void)
{
  if (FileSystem_CreateFile(FileWhiteGreen))
  {
    WriteFactoryWhiteGreen();
  }
}

//_____________________________________________________________________________

void Settings_CreateWhiteBlueFile(void)
{
  if (FileSystem_CreateFile(FileWhiteBlue))
  {
    WriteFactoryWhiteBlue();
  }
}

//_____________________________________________________________________________

void Settings_CreateBlackRedFile(void)
{
  if (FileSystem_CreateFile(FileBlackRed))
  {
    WriteFactoryBlackRed();
  }
}

//_____________________________________________________________________________

void Settings_CreateBlackGreenFile(void)
{
  if (FileSystem_CreateFile(FileBlackGreen))
  {
    WriteFactoryBlackGreen();
  }
}

//_____________________________________________________________________________

void Settings_CreateBlackBlueFile(void)
{
  if (FileSystem_CreateFile(FileBlackBlue))
  {
    WriteFactoryBlackBlue();
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

void Settings_WriteVolume(int16_t volume)
{
  int16_t input = volume;

  FileSystem_Write(FileVolume, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteWhiteRed(int16_t whiteRed)
{
  int16_t input = whiteRed;

  FileSystem_Write(FileWhiteRed, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteWhiteGreen(int16_t whiteGreen)
{
  int16_t input = whiteGreen;

  FileSystem_Write(FileWhiteGreen, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteWhiteBlue(int16_t whiteBlue)
{
  int16_t input = whiteBlue;

  FileSystem_Write(FileWhiteBlue, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteBlackRed(int16_t blackRed)
{
  int16_t input = blackRed;

  FileSystem_Write(FileBlackRed, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteBlackGreen(int16_t blackGreen)
{
  int16_t input = blackGreen;

  FileSystem_Write(FileBlackGreen, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteBlackBlue(int16_t blackBlue)
{
  int16_t input = blackBlue;

  FileSystem_Write(FileBlackBlue, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

int16_t Settings_ReadLeftMotor(void)
{
  int16_t output = 0;

  FileSystem_Read(FileLeftMotor, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadRightMotor(void)
{
  int16_t output = 0;

  FileSystem_Read(FileRightMotor, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadOffsetGyro(void)
{
  int16_t output = 0;

  FileSystem_Read(FileOffsetGyro, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadVolume(void)
{
  int16_t output = 0;

  FileSystem_Read(FileVolume, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadWhiteRed(void)
{
  int16_t output = 0;

  FileSystem_Read(FileWhiteRed, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadWhiteGreen(void)
{
  int16_t output = 0;

  FileSystem_Read(FileWhiteGreen, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadWhiteBlue(void)
{
  int16_t output = 0;

  FileSystem_Read(FileWhiteBlue, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadBlackRed(void)
{
  int16_t output = 0;

  FileSystem_Read(FileBlackRed, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadBlackGreen(void)
{
  int16_t output = 0;

  FileSystem_Read(FileBlackGreen, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

int16_t Settings_ReadBlackBlue(void)
{
  int16_t output = 0;

  FileSystem_Read(FileBlackBlue, &output, sizeof(int16_t));

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

void Settings_EraseVolume(void)
{
  FileSystem_EraseFile(FileVolume);
}

//_____________________________________________________________________________

void Settings_EraseWhiteRed(void)
{
  FileSystem_EraseFile(FileWhiteRed);
}

//_____________________________________________________________________________

void Settings_EraseWhiteGreen(void)
{
  FileSystem_EraseFile(FileWhiteGreen);
}

//_____________________________________________________________________________

void Settings_EraseWhiteBlue(void)
{
  FileSystem_EraseFile(FileWhiteBlue);
}

//_____________________________________________________________________________

void Settings_EraseBlackRed(void)
{
  FileSystem_EraseFile(FileBlackRed);
}

//_____________________________________________________________________________

void Settings_EraseBlackGreen(void)
{
  FileSystem_EraseFile(FileBlackGreen);
}

//_____________________________________________________________________________

void Settings_EraseBlackBlue(void)
{
  FileSystem_EraseFile(FileBlackBlue);
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

//_____________________________________________________________________________

static void WriteFactoryVolume(void)
{
  int16_t input = DEFAULT_VOLUME;

  FileSystem_Write(FileVolume, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryWhiteRed(void)
{
  int16_t input = DEFAULT_WHITE_RED;

  FileSystem_Write(FileWhiteRed, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryWhiteGreen(void)
{
  int16_t input = DEFAULT_WHITE_GREEN;

  FileSystem_Write(FileWhiteGreen, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryWhiteBlue(void)
{
  int16_t input = DEFAULT_WHITE_BLUE;

  FileSystem_Write(FileWhiteBlue, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryBlackRed(void)
{
  int16_t input = DEFAULT_BLACK_RED;

  FileSystem_Write(FileBlackRed, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryBlackGreen(void)
{
  int16_t input = DEFAULT_BLACK_GREEN;

  FileSystem_Write(FileBlackGreen, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryBlackBlue(void)
{
  int16_t input = DEFAULT_BLACK_BLUE;

  FileSystem_Write(FileBlackBlue, &input, sizeof(int16_t));
}
