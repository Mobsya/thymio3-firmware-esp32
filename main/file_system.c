//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    file_system.c
//! \brief   This module provides the useful functions to use the file system
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_spiffs.h"

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
} T_AsebaSettings;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "file_system";

T_AsebaSettings Input;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void WriteLittleEndian(unsigned int word, int numBytes, FILE* file);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void FileSystem_Init(void)
{
  esp_vfs_spiffs_conf_t conf =
  {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(Tag, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(Tag, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(Tag, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }

    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(NULL, &total, &used);

  if (ret != ESP_OK)
  {
    ESP_LOGE(Tag, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
  }
  else
  {
    ESP_LOGI(Tag, "Partition size: total: %d, used: %d", total, used);
  }

  ESP_LOGI(Tag, "File system is initialized");
}

//_____________________________________________________________________________

void FileSystem_CreateSettingsFile(void)
{
  char filename[30] = "/spiffs/settings.dat";
  FILE* file = fopen(filename, "r");

  if (file == NULL)  // If file does not exist, create it
  {
    file = fopen(filename, "w");

    FileSystem_UpdateSettings(256, 256);

    fwrite(&Input, sizeof(T_AsebaSettings), 1, file);

    ESP_LOGI(Tag, "settings.dat file is created with default values");
  }
  else
  {
    ESP_LOGI(Tag, "settings.dat file already exists");
  }

  fclose(file);
}

//_____________________________________________________________________________

void FileSystem_WriteSettingsFile(void)
{
  char filename[30] = "/spiffs/settings.dat";

  ESP_LOGI(Tag, "Opening file");

  FILE* file = fopen(filename, "w");

  if (file != NULL)
  {
    fwrite(&Input, sizeof(T_AsebaSettings), 1, file);
  }
  else
  {
    ESP_LOGE(Tag, "Failed to open file for writing");
  }

  fclose(file);

  ESP_LOGI(Tag, "File %s written", filename);
}

//_____________________________________________________________________________

void FileSystem_ReadSettingsFile(void)
{
  char filename[30] = "/spiffs/settings.dat";
  T_AsebaSettings output;

  uint16_t fileSize = 0;

  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen(filename, "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    ESP_LOGI(Tag, "File open \"/spiffs/settings.dat\". File size: %d Bytes", fileSize);

    // Read file contents till end of file
    while (fread(&output, sizeof(T_AsebaSettings), 1, file))
    {
      ESP_LOGI(Tag, "Left = %d, Right = %d", output.LeftMotor, output.RightMotor);
    }
  }
  else
  {
    ESP_LOGE(Tag, "Failed to open file for reading");
  }
}

//_____________________________________________________________________________

void FileSystem_UpdateSettings(int16_t leftMotor, int16_t rightMotor)
{
  Input.LeftMotor  = leftMotor;
  Input.RightMotor = rightMotor;
}

//_____________________________________________________________________________

bool FileSystem_DoesFileExist(char* fileName)
{
  struct stat st;
  bool exist = true;

  if (stat(fileName, &st) != 0)
  {
    exist = false;
    ESP_LOGE(Tag, "File doesn't exist: %s", fileName);
  }

  return exist;
}

//_____________________________________________________________________________

void FileSystem_SelectFile(char** fileName, int16_t index, T_Extension extension)
{
  char path[18] = "/spiffs/";
  char name[4];
  char type[6];

  switch (extension)
  {
    case E_Extension_MP3:
      strcpy(type, ".mp3\0");
      break;

    case E_Extension_WAV:
      strcpy(type, ".wav\0");
      break;

    default:
      // Do nothing
      break;
  }

  itoa(index, name, 10);  // Convert the index (in base 10) to a string

  strcat(path, name);
  strcat(path, type);

  *fileName = malloc(sizeof(path));  // Allocated memory

  strcpy(*fileName, path);

  printf("Selected file: %s\n", *fileName);
}

//_____________________________________________________________________________

void FileSystem_EraseFile(char* fileName)
{
  // Check that the file exists
  if (FileSystem_DoesFileExist(fileName))
  {
    if (unlink(fileName) == 0)
    {
      printf("Erased file: %s\n", fileName);
    }
    else
    {
      ESP_LOGE(Tag, "File unsuccessfully erased: %s", fileName);
    }
  }
}

//_____________________________________________________________________________

void FileSystem_WriteWAVFile(char* fileName, uint32_t numSamples, int16_t* data, uint16_t sampleRate, uint8_t channel)
{
  FILE* wav_file;
  unsigned int sample_rate;
  unsigned int bytes_per_sample;
  unsigned int byte_rate;

  bytes_per_sample = 2;

  sample_rate = (unsigned int) sampleRate;

  byte_rate = sample_rate * channel * bytes_per_sample;

  wav_file = fopen(fileName, "w");
  assert(wav_file);  // make sure it opened

  // Write RIFF header
  fwrite("RIFF", 1, 4, wav_file);
  WriteLittleEndian(36 + (bytes_per_sample * numSamples * channel), 4, wav_file);
  fwrite("WAVE", 1, 4, wav_file);

  // Write fmt subchunk
  fwrite("fmt ", 1, 4, wav_file);
  WriteLittleEndian(16, 4, wav_file);                          // SubChunk1Size is 16
  WriteLittleEndian(1, 2, wav_file);                           // PCM is format 1
  WriteLittleEndian(channel, 2, wav_file);
  WriteLittleEndian(sample_rate, 4, wav_file);
  WriteLittleEndian(byte_rate, 4, wav_file);
  WriteLittleEndian(channel * bytes_per_sample, 2, wav_file);  // block align
  WriteLittleEndian(8 * bytes_per_sample, 2, wav_file);        // bits/sample

  // Write data subchunk
  fwrite("data", 1, 4, wav_file);
  WriteLittleEndian(bytes_per_sample * numSamples * channel, 4, wav_file);

  for (uint32_t i = 0u; i < numSamples; i++)
  {
    WriteLittleEndian((unsigned int)(data[i]), bytes_per_sample, wav_file);
  }

  fclose(wav_file);
}

//_____________________________________________________________________________

static void WriteLittleEndian(unsigned int word, int numBytes, FILE* file)
{
  unsigned buffer;

  while (numBytes > 0)
  {
    buffer = word & 0xff;
    fwrite(&buffer, 1, 1, file);
    numBytes--;
    word >>= 8;
  }
}
