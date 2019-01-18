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
//! \version $Id: file_system.c 18076 2017-04-20 12:28:12Z v.gonet $
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
#if 0
void FileSystem_CreateFile(void)
{
  char filename[30] = "/spiffs/settings.txt";

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
  ESP_LOGI(Tag, "Opening file");

  FILE* f = fopen(filename, "w");

  if (f == NULL)
  {
    ESP_LOGE(Tag, "Failed to open file for writing");
    return;
  }

  fprintf(f, "256\n");
  fprintf(f, "253\n");
  fclose(f);

  ESP_LOGI(Tag, "File %s written", filename);
}
#endif
void FileSystem_CreateFile(void)
{
  char filename[30] = "/spiffs/settings.txt";

  // Use POSIX and C standard library functions to work with files.
  // First create a file.
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

void FileSystem_RenameFile(void)
{
  // Check if destination file exists before renaming
  struct stat st;

  if (stat("/spiffs/foo.txt", &st) == 0)
  {
    // Delete it if it exists
    unlink("/spiffs/foo.txt");
  }

  // Rename original file
  ESP_LOGI(Tag, "Renaming file");

  if (rename("/spiffs/settings.txt", "/spiffs/foo.txt") != 0)
  {
    ESP_LOGE(Tag, "Rename failed");
    return;
  }
}

//_____________________________________________________________________________
#if 0
void FileSystem_ReadFile(void)
{
  // Open renamed file for reading
  ESP_LOGI(Tag, "Reading file");

  FILE* f = fopen("/spiffs/settings.txt", "r");

  if (f == NULL)
  {
    ESP_LOGE(Tag, "Failed to open file for reading");
    return;
  }

#if 0
  char c;

  while((c=fgetc(f))!=EOF)
  {
	//fprintf("%c", c);
  }

  fclose(f);
#endif

//#if 0
  char line[64];
  fgets(line, sizeof(line), f);
  fclose(f);

  // strip newline
  char* pos = strchr(line, '\n');

  if (pos)
  {
    *pos = '\0';
  }


  ESP_LOGI(Tag, "Read from file: '%s'", line);
//#endif
}
#endif
#if 0
void FileSystem_ReadFile(void)
{
  char *file_buf;

  unsigned int file_size = 0;
  char cont_len[8];

  T_AsebaSettings output;

  // Open renamed file for reading
  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen("/spiffs/settings.txt", "r");

  if (file != NULL)
  {
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	sprintf(cont_len, "%d", file_size);
	ESP_LOGI(Tag, "File open \"/spiffs/settings.txt\". File size: %d Bytes", file_size);

    ESP_LOGI(Tag, "content-length %s", cont_len);

    file_buf = (char *)malloc(file_size);

    if (file_buf == NULL)
    {
	  ESP_LOGE("SPIFFS", "Failed to allocate memory");
	  //return ESP_FAIL;
    }

    //bytes_read = fread(file_buf, sizeof(file_buf), 1, file);
    //bytes_read = fread(file_buf, file_size, 1, file);

    // Read file contents till end of file
    while (fread(&output, sizeof(T_AsebaSettings), 1, file))
    {
      printf("Left = %d, Right = %d\n", output.LeftMotor, output.RightMotor);
    }
  }
  else
  {
    ESP_LOGE(Tag, "Failed to open file for reading");
  }
}
#endif

void FileSystem_ReadFile(void)
{
  T_AsebaSettings output;

  uint16_t fileSize = 0;

  // Open renamed file for reading
  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen("/spiffs/settings.txt", "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END);
	fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	ESP_LOGI(Tag, "File open \"/spiffs/settings.txt\". File size: %d Bytes", fileSize);

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
