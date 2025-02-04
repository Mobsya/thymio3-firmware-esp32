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

#include "string.h"

#include "settings.h"
#include "aseba_esp32.h"
#include "codec.h"
#include "file_system.h"
#include "gyroscope.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DEFAULT_LEFT_MOTOR      256
#define DEFAULT_RIGHT_MOTOR     256
#define DEFAULT_OFFSET_GYRO       0
#define DEFAULT_VOLUME           80
#define DEFAULT_WHITE_RED      11036
#define DEFAULT_WHITE_GREEN    14470
#define DEFAULT_WHITE_BLUE     13334
#define DEFAULT_BLACK_RED       4214
#define DEFAULT_BLACK_GREEN     6055
#define DEFAULT_BLACK_BLUE      5312
#define DEFAULT_RC5_ADDRESS       0
#define DEFAULT_MOT_FW_TO_BW 1.0
#define DEFAULT_OFFSET_GYRO_X 0
#define DEFAULT_OFFSET_GYRO_Y 0
#define DEFAULT_OFFSET_GYRO_Z 0

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
static const char* FileWhite      = "/spiffs/white.dat";
static const char* FileBlack      = "/spiffs/black.dat";
static const char* FileRC5Address = "/spiffs/rc5_address.dat";
static const char* FileMotFwBw    = "/spiffs/mot_fw_bw.dat";
static const char* FileZeroOffGyro = "/spiffs/zero_off_gyro.dat";

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

//! \brief     Write the default white (red, green, blue) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryWhite(void);

//! \brief     Write the default black (red, green, blue) value to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryBlack(void);

//! \brief     Write the default remote address to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryRC5Address(void);

//! \brief     Write the default forward to backward motors correction to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryMotFwBwFactor(void);

//! \brief     Write the default gyro axes offsets to the file
//! \pre       First initialize the settings
//! \param     None
//! \return    None
static void WriteFactoryZeroOffGyro(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Settings_Init(void)
{
  Settings_LoadLeftMotorFile();
  ESP_LOGI(Tag, "Left motor: %d", Settings.LeftMotor);
  Settings_LoadRightMotorFile();
  ESP_LOGI(Tag, "Right motor: %d", Settings.RightMotor);
  Settings_LoadOffsetGyroFile();
  ESP_LOGI(Tag, "Gyro offset: %d", Settings.OffsetGyro);
  Settings_LoadVolumeFile();
  ESP_LOGI(Tag, "Volume: %d", Settings.Volume);
  Settings_LoadWhiteFile();
  ESP_LOGI(Tag, "White: %d, %d, %d", Settings.White[0], Settings.White[1], Settings.White[2]);
  Settings_LoadBlackFile();
  ESP_LOGI(Tag, "Black: %d, %d, %d", Settings.Black[0], Settings.Black[1], Settings.Black[2]);
  //Settings_LoadRC5AddressFile();
  //ESP_LOGI(Tag, "RC5 addr: %d", Settings.RC5Address);
  Settings_LoadMotFwBwFile();
  ESP_LOGI(Tag, "Mot fw bw: %f", Settings.MotFwBw);
  Settings_LoadZeroOffGyroFile();
  ESP_LOGI(Tag, "Gyro offset x,y,z: %d,%d,%d", Settings.ZeroOffGyro[0], Settings.ZeroOffGyro[1], Settings.ZeroOffGyro[2]);  
  // Enable gyro continuous auto calibration only if there is no calibration saved.
  if((Settings.ZeroOffGyro[0]==DEFAULT_OFFSET_GYRO_X) && (Settings.ZeroOffGyro[1]==DEFAULT_OFFSET_GYRO_Y) && (Settings.ZeroOffGyro[2]==DEFAULT_OFFSET_GYRO_Z))
  {
    Gyroscope_EnableContinuousCalib();
  } 
  else
  {
    Gyroscope_SetCalibration(Settings.ZeroOffGyro);
  }

  ESP_LOGI(Tag, "Settings are initialized");
}

//_____________________________________________________________________________

void Settings_UpdateSettings(void)
{
  static int16_t oldSoundVolume = 0;

  Settings.LeftMotor  = vmVariables.settings[0];
  Settings.RightMotor = vmVariables.settings[1];
  //Settings.OffsetGyro = vmVariables.settings[2];

  if (vmVariables.sound_volume != oldSoundVolume)
  {
    if (vmVariables.sound_volume > 100)
    {
      Settings.Volume = 100;
    }
    else if (vmVariables.sound_volume < 40)
    {
      Settings.Volume = 40;
    }
    else
    {
      Settings.Volume = vmVariables.sound_volume;
    }

    ESP_LOGE(Tag, "Volume: %d", Settings.Volume);

    Codec_SetVolume(Settings.Volume);

    oldSoundVolume = vmVariables.sound_volume;
  }
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

void Settings_SetVolumeSettings(int16_t volume)
{
  Settings.Volume = volume;
}

//_____________________________________________________________________________

void Settings_SetOffsetGyroSettings(int16_t offset)
{
  Settings.OffsetGyro = offset;
}

//_____________________________________________________________________________

void Settings_SetWhiteRedSettings(int16_t offset)
{
  Settings.White[0] = offset;
}

//_____________________________________________________________________________

void Settings_SetWhiteGreenSettings(int16_t offset)
{
  Settings.White[1] = offset;
}

//_____________________________________________________________________________

void Settings_SetWhiteBlueSettings(int16_t offset)
{
  Settings.White[2] = offset;
}

//_____________________________________________________________________________

void Settings_SetWhiteSettings(int16_t* values)
{
  memcpy(Settings.White, values, 6);
}

//_____________________________________________________________________________

void Settings_SetBlackRedSettings(int16_t offset)
{
  Settings.Black[0] = offset;
}

//_____________________________________________________________________________

void Settings_SetBlackGreenSettings(int16_t offset)
{
  Settings.Black[1] = offset;
}

//_____________________________________________________________________________

void Settings_SetBlackBlueSettings(int16_t offset)
{
  Settings.Black[2] = offset;
}

//_____________________________________________________________________________

void Settings_SetBlackSettings(int16_t* values)
{
  memcpy(Settings.Black, values, 6);
}

//_____________________________________________________________________________

void Settings_SetRC5AddressSettings(int16_t addr)
{
  Settings.RC5Address = addr;
}

//_____________________________________________________________________________

void Settings_SetMotFwBwSettings(float factor)
{
  Settings.MotFwBw = factor;
}

//_____________________________________________________________________________

void Settings_SetOffsetGyroXSettings(int16_t offset)
{
  Settings.ZeroOffGyro[0] = offset;
}

//_____________________________________________________________________________

void Settings_SetOffsetGyroYSettings(int16_t offset)
{
  Settings.ZeroOffGyro[1] = offset;
}

//_____________________________________________________________________________

void Settings_SetOffsetGyroZSettings(int16_t offset)
{
  Settings.ZeroOffGyro[2] = offset;
}

//_____________________________________________________________________________

void Settings_SetZeroOffGyroSettings(int16_t* values)
{
  memcpy(Settings.ZeroOffGyro, values, 6);
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

int16_t Settings_GetVolumeSettings(void)
{
  return Settings.Volume;
}

//_____________________________________________________________________________

int16_t Settings_GetOffsetGyroSettings(void)
{
  return Settings.OffsetGyro;
}

//_____________________________________________________________________________

int16_t Settings_GetWhiteRedSettings(void)
{
  return Settings.White[0];
}

//_____________________________________________________________________________

int16_t Settings_GetWhiteGreenSettings(void)
{
  return Settings.White[1];
}

//_____________________________________________________________________________

int16_t Settings_GetWhiteBlueSettings(void)
{
  return Settings.White[2];
}

//_____________________________________________________________________________

void Settings_GetWhiteSettings(int16_t* values)
{
  memcpy(values, Settings.White, 6);
}

//_____________________________________________________________________________

int16_t Settings_GetBlackRedSettings(void)
{
  return Settings.Black[0];
}

//_____________________________________________________________________________

int16_t Settings_GetBlackGreenSettings(void)
{
  return Settings.Black[1];
}

//_____________________________________________________________________________

int16_t Settings_GetBlackBlueSettings(void)
{
  return Settings.Black[2];
}

//_____________________________________________________________________________

void Settings_GetBlackSettings(int16_t* values)
{
  memcpy(values, Settings.Black, 6);
}

//_____________________________________________________________________________

int16_t Settings_GetRC5AddressSettings(void)
{
  return Settings.RC5Address;
}

//_____________________________________________________________________________

float Settings_GetMotFwBwSettings(void)
{
  return Settings.MotFwBw;
}

//_____________________________________________________________________________

int16_t Settings_GetOffsetGyroXSettings(void)
{
  return Settings.ZeroOffGyro[0];
}


//_____________________________________________________________________________

int16_t Settings_GetOffsetGyroYSettings(void)
{
  return Settings.ZeroOffGyro[1];
}

//_____________________________________________________________________________

int16_t Settings_GetOffsetGyroZSettings(void)
{
  return Settings.ZeroOffGyro[2];
}

//_____________________________________________________________________________

void Settings_GetZeroOffGyroSettings(int16_t* values)
{
  memcpy(values, Settings.ZeroOffGyro, 6);
}

//_____________________________________________________________________________

void Settings_LoadLeftMotorFile(void)
{
  if (FileSystem_CreateFile(FileLeftMotor))
  {
    WriteFactoryLeftMotor();
    Settings.LeftMotor = DEFAULT_LEFT_MOTOR;
  } 
  else 
  {
    Settings.LeftMotor = Settings_ReadLeftMotor();
  }
}

//_____________________________________________________________________________

void Settings_LoadRightMotorFile(void)
{
  if (FileSystem_CreateFile(FileRightMotor))
  {
    WriteFactoryRightMotor();
    Settings.RightMotor = DEFAULT_RIGHT_MOTOR;
  } 
  else
  {
    Settings.RightMotor = Settings_ReadRightMotor();
  }
}

//_____________________________________________________________________________

void Settings_LoadOffsetGyroFile(void)
{
  if (FileSystem_CreateFile(FileOffsetGyro))
  {
    WriteFactoryOffsetGyro();
    Settings.OffsetGyro = DEFAULT_OFFSET_GYRO;
  } 
  else
  {
    Settings.OffsetGyro = Settings_ReadOffsetGyro();
  }
}

//_____________________________________________________________________________

void Settings_LoadVolumeFile(void)
{
  if (FileSystem_CreateFile(FileVolume))
  {
    WriteFactoryVolume();
    Settings.Volume = DEFAULT_VOLUME;
  }
  else
  {
    Settings.Volume = Settings_ReadVolume();
  }
}

//_____________________________________________________________________________

void Settings_LoadWhiteFile(void)
{
  if (FileSystem_CreateFile(FileWhite))
  {
    WriteFactoryWhite();
    Settings.White[0] = DEFAULT_WHITE_RED;
    Settings.White[1] = DEFAULT_WHITE_GREEN;
    Settings.White[2] = DEFAULT_WHITE_BLUE;
  }
  else
  {
    Settings_ReadWhite(Settings.White);
  }
}

//_____________________________________________________________________________

void Settings_LoadBlackFile(void)
{
  if (FileSystem_CreateFile(FileBlack))
  {
    WriteFactoryBlack();
    Settings.Black[0] = DEFAULT_BLACK_RED;
    Settings.Black[1] = DEFAULT_BLACK_GREEN;
    Settings.Black[2] = DEFAULT_BLACK_BLUE;
  }
  else
  {
    Settings_ReadBlack(Settings.Black);
  }
}

//_____________________________________________________________________________

void Settings_LoadRC5AddressFile(void)
{
  if (FileSystem_CreateFile(FileRC5Address))
  {
    WriteFactoryRC5Address();
    Settings.RC5Address = DEFAULT_RC5_ADDRESS;
  }
  else
  {
    Settings.RC5Address = Settings_ReadRC5Address();
  }
}

//_____________________________________________________________________________

void Settings_LoadMotFwBwFile(void)
{
  if (FileSystem_CreateFile(FileMotFwBw))
  {
    WriteFactoryMotFwBwFactor();
    Settings.MotFwBw = DEFAULT_MOT_FW_TO_BW;
  }
  else
  {
    Settings.MotFwBw = Settings_ReadMotFwBwFactor();
  }
}

//_____________________________________________________________________________

void Settings_LoadZeroOffGyroFile(void)
{
  if (FileSystem_CreateFile(FileZeroOffGyro))
  {
    WriteFactoryZeroOffGyro();
    Settings.ZeroOffGyro[0] = DEFAULT_OFFSET_GYRO_X;
    Settings.ZeroOffGyro[1] = DEFAULT_OFFSET_GYRO_Y;
    Settings.ZeroOffGyro[2] = DEFAULT_OFFSET_GYRO_Z;
  }
  else
  {
    Settings_ReadZeroOffGyro(Settings.ZeroOffGyro);
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

void Settings_WriteWhite(int16_t* values)
{
  FileSystem_Write(FileWhite, values, 6);
}

//_____________________________________________________________________________

void Settings_WriteBlack(int16_t* values)
{
  FileSystem_Write(FileBlack, values, 6);
}

//_____________________________________________________________________________

void Settings_WriteRC5Address(int16_t address)
{
  int16_t input = address;

  FileSystem_Write(FileRC5Address, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

void Settings_WriteMotFwBwFactor(float factor)
{
  float input = factor;

  FileSystem_Write(FileMotFwBw, &input, sizeof(float));
}

//_____________________________________________________________________________

void Settings_WriteZeroOffGyro(int16_t* values)
{
  FileSystem_Write(FileZeroOffGyro, values, 6);
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

  Settings.Volume = output;

  return output;
}

//_____________________________________________________________________________

void Settings_ReadWhite(int16_t* values)
{
  FileSystem_Read(FileWhite, values, 6);
}

//_____________________________________________________________________________

void Settings_ReadBlack(int16_t* values)
{
  FileSystem_Read(FileBlack, values, 6);
}

//_____________________________________________________________________________

int16_t Settings_ReadRC5Address(void)
{
  int16_t output = 0;

  FileSystem_Read(FileRC5Address, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

float Settings_ReadMotFwBwFactor(void)
{
  float output = 0;

  FileSystem_Read(FileMotFwBw, &output, sizeof(float));

  return output;
}

//_____________________________________________________________________________

void Settings_ReadZeroOffGyro(int16_t* values)
{
  FileSystem_Read(FileZeroOffGyro, values, 6);
}

//_____________________________________________________________________________

void Settings_EraseLeftMotorFile(void)
{
  FileSystem_EraseFile(FileLeftMotor);
}

//_____________________________________________________________________________

void Settings_EraseRightMotorFile(void)
{
  FileSystem_EraseFile(FileRightMotor);
}

//_____________________________________________________________________________

void Settings_EraseOffsetGyroFile(void)
{
  FileSystem_EraseFile(FileOffsetGyro);
}

//_____________________________________________________________________________

void Settings_EraseVolumeFile(void)
{
  FileSystem_EraseFile(FileVolume);
}

//_____________________________________________________________________________

void Settings_EraseWhiteFile(void)
{
  FileSystem_EraseFile(FileWhite);
}

//_____________________________________________________________________________

void Settings_EraseBlackFile(void)
{
  FileSystem_EraseFile(FileBlack);
}

//_____________________________________________________________________________

void Settings_EraseRC5AddressFile(void)
{
  FileSystem_EraseFile(FileRC5Address);
}

//_____________________________________________________________________________

void Settings_EraseMotFwBwFile(void)
{
  FileSystem_EraseFile(FileMotFwBw);
}

//_____________________________________________________________________________

void Settings_EraseZeroOffGyroFile(void)
{
  FileSystem_EraseFile(FileZeroOffGyro);
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

static void WriteFactoryWhite(void)
{
  int16_t input[3] = {DEFAULT_WHITE_RED, DEFAULT_WHITE_GREEN, DEFAULT_WHITE_BLUE};
  FileSystem_Write(FileWhite, input, 6);
}

//_____________________________________________________________________________

static void WriteFactoryBlack(void)
{
  int16_t input[3] = {DEFAULT_BLACK_RED, DEFAULT_BLACK_GREEN, DEFAULT_BLACK_BLUE};
  FileSystem_Write(FileBlack, input, 6);
}

//_____________________________________________________________________________

static void WriteFactoryRC5Address(void)
{
  int16_t input = DEFAULT_RC5_ADDRESS;

  FileSystem_Write(FileRC5Address, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

static void WriteFactoryMotFwBwFactor(void)
{
  float input = DEFAULT_MOT_FW_TO_BW;

  FileSystem_Write(FileMotFwBw, &input, sizeof(float));
}

//_____________________________________________________________________________

static void WriteFactoryZeroOffGyro(void)
{
  int16_t input[3] = {DEFAULT_OFFSET_GYRO_X, DEFAULT_OFFSET_GYRO_Y, DEFAULT_OFFSET_GYRO_Z};

  FileSystem_Write(FileZeroOffGyro, input, 6);
}

