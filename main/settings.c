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

static const char* FileMotors     = "/spiffs/motors.dat";
static const char* FileOffsetGyro = "/spiffs/offset_gyro.dat";
static const char* FileVolume     = "/spiffs/volume.dat";
static const char* FileWhite      = "/spiffs/white.dat";
static const char* FileBlack      = "/spiffs/black.dat";
static const char* FileRC5Address = "/spiffs/rc5_address.dat";
static const char* FileMotFwBw    = "/spiffs/mot_fw_bw.dat";
static const char* FileMot15cm    = "/spiffs/mot_15cm.dat";
static const char* FileZeroOffGyro = "/spiffs/zero_off_gyro.dat";
static const char* FileGyroRotFactor = "/spiffs/gyro_rot.dat";
static const char* FileGroundBlack = "/spiffs/ground_black.dat";
static const char* FileGroundWhite = "/spiffs/ground_white.dat";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Settings_Init(void)
{
  Settings_LoadMotorsFile();
  ESP_LOGI(Tag, "Motors: %d, %d", Settings.Motors[0], Settings.Motors[1]);
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
  Settings_LoadMot15cmFile();
  ESP_LOGI(Tag, "Mot 15 cm timer values: %lld, %lld", Settings.Mot15cm[0], Settings.Mot15cm[1]);
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
  Settings_LoadGyroRotFactorFile();
  ESP_LOGI(Tag, "Gyro rot factor: %d", Settings.GyroRotFactor);  
  Settings_LoadGroundBlackFile();
  ESP_LOGI(Tag, "Ground black: %d, %d", Settings.GroundBlack[0], Settings.GroundBlack[1]);  
  Settings_LoadGroundWhiteFile();
  ESP_LOGI(Tag, "Ground white: %d, %d", Settings.GroundWhite[0], Settings.GroundWhite[1]);  
  if(Settings.GroundWhite[0] == 0) // Something wrong is happening, start with default values
  {
    Settings.GroundWhite[0] = DEFAULT_GROUND_WHITE;
  }
  if(Settings.GroundWhite[1] == 0) // Something wrong is happening, start with default values
  {
    Settings.GroundWhite[1] = DEFAULT_GROUND_WHITE;
  }

  ESP_LOGI(Tag, "Settings are initialized");
}

//_____________________________________________________________________________

void Settings_UpdateSettings(void)
{
  static int16_t oldSoundVolume = 0;

  Settings.Motors[0]  = vmVariables.settings[0];
  Settings.Motors[1] = vmVariables.settings[1];
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
  Settings.Motors[0] = leftMotor;
}

//_____________________________________________________________________________

void Settings_SetRightMotorSettings(int16_t rightMotor)
{
  Settings.Motors[1] = rightMotor;
}

//_____________________________________________________________________________

void Settings_SetMotorsSettings(int16_t* values)
{
  Settings.Motors[0] = values[0];
  Settings.Motors[1] = values[1];
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


void Settings_SetGyroRotFactorSettings(int16_t factor)
{
  Settings.GyroRotFactor = factor;
}

//_____________________________________________________________________________

void Settings_SetGroundBlackSettings(int16_t* values)
{
  memcpy(Settings.GroundBlack, values, 4);
}

//_____________________________________________________________________________

void Settings_SetGroundWhiteSettings(int16_t* values)
{
  memcpy(Settings.GroundWhite, values, 4);
}

//_____________________________________________________________________________

void Settings_SetMot15cmSettings(uint64_t* values)
{
  memcpy(Settings.Mot15cm, values, 16);
}

//_____________________________________________________________________________

int16_t Settings_GetLeftMotorSettings(void)
{
  return Settings.Motors[0];
}

//_____________________________________________________________________________

int16_t Settings_GetRightMotorSettings(void)
{
  return Settings.Motors[1];
}

//_____________________________________________________________________________

void Settings_GetMotorsSettings(int16_t* values)
{
  memcpy(values, Settings.Motors, 4);
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

int16_t Settings_GetGyroRotFactorSettings(void)
{
  return Settings.GyroRotFactor;
}

//_____________________________________________________________________________

void Settings_GetGroundBlackSettings(int16_t* values)
{
  memcpy(values, Settings.GroundBlack, 4);
}

//_____________________________________________________________________________

void Settings_GetGroundWhiteSettings(int16_t* values)
{
  memcpy(values, Settings.GroundWhite, 4);
}

//_____________________________________________________________________________

void Settings_GetMot15cmSettings(uint64_t* values)
{
  memcpy(values, Settings.Mot15cm, 16);
}

//_____________________________________________________________________________

void Settings_LoadMotorsFile(void)
{
  if (FileSystem_CreateFile(FileMotors))
  {
    WriteFactoryMotors();
  } 
  else 
  {
    Settings_ReadMotors(Settings.Motors);
  }
}

//_____________________________________________________________________________

void Settings_LoadOffsetGyroFile(void)
{
  if (FileSystem_CreateFile(FileOffsetGyro))
  {
    WriteFactoryOffsetGyro();    
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
  }
  else
  {
    Settings_ReadZeroOffGyro(Settings.ZeroOffGyro);
  }
}

//_____________________________________________________________________________

void Settings_LoadGyroRotFactorFile(void)
{
  if (FileSystem_CreateFile(FileGyroRotFactor))
  {
    WriteFactoryGyroRotFactor();    
  }
  else
  {
    Settings.GyroRotFactor = Settings_ReadGyroRotFactor();
  }  
}

//_____________________________________________________________________________

void Settings_LoadGroundBlackFile(void)
{
  if (FileSystem_CreateFile(FileGroundBlack))
  {
    WriteFactoryGroundBlack();
  }
  else
  {
    Settings_ReadGroundBlack(Settings.GroundBlack);
  }
}

//_____________________________________________________________________________

void Settings_LoadGroundWhiteFile(void)
{
  if (FileSystem_CreateFile(FileGroundWhite))
  {
    WriteFactoryGroundWhite();
  }
  else
  {
    Settings_ReadGroundWhite(Settings.GroundWhite);
  }
}

//_____________________________________________________________________________

void Settings_LoadMot15cmFile(void)
{
  if (FileSystem_CreateFile(FileMot15cm))
  {
    WriteFactoryMot15cm();
  }
  else
  {
    Settings_ReadMot15cm(Settings.Mot15cm);
  }
}

//_____________________________________________________________________________

int Settings_WriteMotors(int16_t* values)
{
  return FileSystem_Write(FileMotors, values, 4);
}

//_____________________________________________________________________________

int Settings_WriteOffsetGyro(int16_t offsetGyro)
{
  int16_t input = offsetGyro;

  return FileSystem_Write(FileOffsetGyro, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

int Settings_WriteVolume(int16_t volume)
{
  int16_t input = volume;

  return FileSystem_Write(FileVolume, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

int Settings_WriteWhite(int16_t* values)
{
  return FileSystem_Write(FileWhite, values, 6);
}

//_____________________________________________________________________________

int Settings_WriteBlack(int16_t* values)
{
  return FileSystem_Write(FileBlack, values, 6);
}

//_____________________________________________________________________________

int Settings_WriteRC5Address(int16_t address)
{
  int16_t input = address;

  return FileSystem_Write(FileRC5Address, &input, sizeof(int16_t));
}

//_____________________________________________________________________________

int Settings_WriteMotFwBwFactor(float factor)
{
  float input = factor;

  return FileSystem_Write(FileMotFwBw, &input, sizeof(float));
}

//_____________________________________________________________________________

int Settings_WriteZeroOffGyro(int16_t* values)
{
  return FileSystem_Write(FileZeroOffGyro, values, 6);
}

//_____________________________________________________________________________

int Settings_WriteGyroRotFactor(int16_t factor)
{
  int16_t input = factor;

  return FileSystem_Write(FileGyroRotFactor, &input, sizeof(int16_t));
}

//_____________________________________________________________________________


int Settings_WriteGroundBlack(int16_t* offsets)
{
  return FileSystem_Write(FileGroundBlack, offsets, 4);
}

//_____________________________________________________________________________

int Settings_WriteGroundWhite(int16_t* offsets)
{
  return FileSystem_Write(FileGroundWhite, offsets, 4);
}

//_____________________________________________________________________________

int Settings_WriteMot15cm(uint64_t* values)
{
  return FileSystem_Write(FileMot15cm, values, 16);
}

//_____________________________________________________________________________

void Settings_ReadMotors(int16_t* values)
{
  FileSystem_Read(FileMotors, values, 4);
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

int16_t Settings_ReadGyroRotFactor()
{
  int16_t output = 0;

  FileSystem_Read(FileGyroRotFactor, &output, sizeof(int16_t));

  return output;
}

//_____________________________________________________________________________

void Settings_ReadGroundBlack(int16_t* values)
{
  FileSystem_Read(FileGroundBlack, values, 4);
}

//_____________________________________________________________________________

void Settings_ReadGroundWhite(int16_t* values)
{
  FileSystem_Read(FileGroundWhite, values, 4);
}

//_____________________________________________________________________________

void Settings_ReadMot15cm(uint64_t* values)
{
  FileSystem_Read(FileMot15cm, values, 16);
}

//_____________________________________________________________________________

void Settings_EraseMotorsFile(void)
{
  FileSystem_EraseFile(FileMotors);
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

void Settings_EraseGyroRotFactorFile(void)
{
  FileSystem_EraseFile(FileGyroRotFactor);
}

//_____________________________________________________________________________

void Settings_EraseGroundBlackFile(void)
{
  FileSystem_EraseFile(FileGroundBlack);
}

//_____________________________________________________________________________

void Settings_EraseGroundWhiteFile(void)
{
  FileSystem_EraseFile(FileGroundWhite);
}

//_____________________________________________________________________________

void Settings_EraseMot15cm(void)
{
  FileSystem_EraseFile(FileMot15cm);
}

//_____________________________________________________________________________

void WriteFactoryMotors(void)
{
  int16_t input[2] = {DEFAULT_LEFT_MOTOR, DEFAULT_RIGHT_MOTOR};

  FileSystem_Write(FileMotors, input, 4);

  Settings.Motors[0] = DEFAULT_LEFT_MOTOR;
  Settings.Motors[1] = DEFAULT_RIGHT_MOTOR;  
}

//_____________________________________________________________________________

void WriteFactoryOffsetGyro(void)
{
  int16_t input = DEFAULT_OFFSET_GYRO;

  FileSystem_Write(FileOffsetGyro, &input, sizeof(int16_t));

  Settings.OffsetGyro = DEFAULT_OFFSET_GYRO;
}

//_____________________________________________________________________________

void WriteFactoryVolume(void)
{
  int16_t input = DEFAULT_VOLUME;

  FileSystem_Write(FileVolume, &input, sizeof(int16_t));

  Settings.Volume = DEFAULT_VOLUME;
}

//_____________________________________________________________________________

void WriteFactoryWhite(void)
{
  int16_t input[3] = {DEFAULT_WHITE_RED, DEFAULT_WHITE_GREEN, DEFAULT_WHITE_BLUE};
  FileSystem_Write(FileWhite, input, 6);
  Settings.White[0] = DEFAULT_WHITE_RED;
  Settings.White[1] = DEFAULT_WHITE_GREEN;
  Settings.White[2] = DEFAULT_WHITE_BLUE;  
}

//_____________________________________________________________________________

void WriteFactoryBlack(void)
{
  int16_t input[3] = {DEFAULT_BLACK_RED, DEFAULT_BLACK_GREEN, DEFAULT_BLACK_BLUE};
  FileSystem_Write(FileBlack, input, 6);
  Settings.Black[0] = DEFAULT_BLACK_RED;
  Settings.Black[1] = DEFAULT_BLACK_GREEN;
  Settings.Black[2] = DEFAULT_BLACK_BLUE;  
}

//_____________________________________________________________________________

void WriteFactoryRC5Address(void)
{
  int16_t input = DEFAULT_RC5_ADDRESS;

  FileSystem_Write(FileRC5Address, &input, sizeof(int16_t));

  Settings.RC5Address = DEFAULT_RC5_ADDRESS;
}

//_____________________________________________________________________________

void WriteFactoryMotFwBwFactor(void)
{
  float input = DEFAULT_MOT_FW_TO_BW;

  FileSystem_Write(FileMotFwBw, &input, sizeof(float));

  Settings.MotFwBw = DEFAULT_MOT_FW_TO_BW;
}

//_____________________________________________________________________________

void WriteFactoryZeroOffGyro(void)
{
  int16_t input[3] = {DEFAULT_OFFSET_GYRO_X, DEFAULT_OFFSET_GYRO_Y, DEFAULT_OFFSET_GYRO_Z};

  FileSystem_Write(FileZeroOffGyro, input, 6);

  Settings.ZeroOffGyro[0] = DEFAULT_OFFSET_GYRO_X;
  Settings.ZeroOffGyro[1] = DEFAULT_OFFSET_GYRO_Y;
  Settings.ZeroOffGyro[2] = DEFAULT_OFFSET_GYRO_Z;  
}

//_____________________________________________________________________________

void WriteFactoryGyroRotFactor(void)
{
  int16_t input = DEFAULT_GYRO_ROT_FACTOR;

  FileSystem_Write(FileGyroRotFactor, &input, sizeof(int16_t));

  Settings.GyroRotFactor = DEFAULT_GYRO_ROT_FACTOR;
}

//_____________________________________________________________________________

void WriteFactoryGroundBlack(void)
{
  int16_t input[2] = {DEFAULT_GROUND_BLACK, DEFAULT_GROUND_BLACK};

  FileSystem_Write(FileGroundBlack, input, 4);

  Settings.GroundBlack[0] = DEFAULT_GROUND_BLACK;
  Settings.GroundBlack[1] = DEFAULT_GROUND_BLACK;  
}

//_____________________________________________________________________________

void WriteFactoryGroundWhite(void)
{
  int16_t input[2] = {DEFAULT_GROUND_WHITE, DEFAULT_GROUND_WHITE};

  FileSystem_Write(FileGroundWhite, input, 4);

  Settings.GroundWhite[0] = DEFAULT_GROUND_WHITE;
  Settings.GroundWhite[1] = DEFAULT_GROUND_WHITE;  
}

//_____________________________________________________________________________

void WriteFactoryMot15cm(void)
{
  uint64_t input[2] = {DEFAULT_MOT15CM, DEFAULT_MOT15CM};

  FileSystem_Write(FileMot15cm, input, 16);

  Settings.Mot15cm[0] = DEFAULT_MOT15CM;
  Settings.Mot15cm[1] = DEFAULT_MOT15CM; 
}

