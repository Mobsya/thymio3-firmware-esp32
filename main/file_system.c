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
//! \author  Vincent Gonet, Stefano Morgani
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
#include <sys/dirent.h>
#include <ctype.h>

#include "esp_log.h"
#include "esp_spiffs.h"

#include "file_system.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define FNM_PERIOD    0x04  // Period must be matched by period.

// fnmatch defines
#define FNM_NOMATCH     1       // Match failed.
#define FNM_NOESCAPE  0x01  // Disable backslash escaping.
#define FNM_PATHNAME  0x02  // Slash must be matched by slash.
#define FNM_PERIOD    0x04  // Period must be matched by period.
#define FNM_LEADING_DIR 0x08  // Ignore /<tail> after Imatch.
#define FNM_CASEFOLD  0x10  // Case insensitive search.
#define FNM_PREFIX_DIRS 0x20  // Directory prefixes of pattern match too.
#define EOS             '\0'

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "file_system";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void WriteLittleEndian(unsigned int word, int numBytes, FILE* file);

static void list(char* path, char* match);

static int fnmatch(const char* pattern, const char* string, int flags);

static const char* rangematch(const char* pattern, char test, int flags);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------
#if 0
void FileSystem_Init(void)
{
  esp_vfs_spiffs_conf_t conf =
  {
    .base_path = "/spiffs",
    .partition_label = NULL, // First partition with subtype=spiffs will be used
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
#endif
//_____________________________________________________________________________

bool FileSystem_CreateFile(const char* filename)
{
  FILE* file = fopen(filename, "r");
  bool isCreated = false;

  if (file == NULL)  // If file does not exist, create it
  {
    file = fopen(filename, "w");
    isCreated = true;
    ESP_LOGI(Tag, "File %s is created", filename);
  }
  else
  {
    ESP_LOGI(Tag, "File %s already exists", filename);
  }

  fclose(file);

  return isCreated;
}

//_____________________________________________________________________________

void FileSystem_Write(const char* filename, void* input, long int size)
{
  ESP_LOGI(Tag, "Opening file");

  FILE* file = fopen(filename, "w");

  if (file != NULL)
  {
    fwrite(input, size, 1, file);
    ESP_LOGI(Tag, "File %s written", filename);
  }
  else
  {
    ESP_LOGE(Tag, "Failed to open file for writing");
  }

  fclose(file);

}

//_____________________________________________________________________________

void FileSystem_Read(const char* filename, void* output, long int size)
{
  uint16_t fileSize = 0;

  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen(filename, "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END);
    fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    ESP_LOGI(Tag, "File %s open. File size: %d Bytes", filename, fileSize);

    // Read file contents till end of file
    while (fread(output, size, 1, file))
    {}

    fclose(file);
  }
  else
  {
    ESP_LOGE(Tag, "Failed to open file for reading");
  }
}

//_____________________________________________________________________________

int8_t FileSystem_Read2(const char* filename, void* output, long int *size)
{
  uint16_t fileSize = 0;

  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen(filename, "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END); // To get file size
    fileSize = ftell(file);
    if(fileSize < 0) {
      return -2;
    }
    *size = fileSize;
    fseek(file, 0, SEEK_SET);
    ESP_LOGI(Tag, "File %s open. File size: %d Bytes", filename, fileSize);

    output = malloc((size_t) fileSize);
    if(output == NULL) {
      return -3;
    }

    size_t size_read = fread(output, 1, fileSize, file);
    fclose(file);
    if(size_read != fileSize) {
      return -4;
    }

  } else {
    ESP_LOGE(Tag, "Failed to open file for reading");
    return -1;
  }
  
  return 0;
}

//_____________________________________________________________________________

int8_t FileSystem_Read3(const char* filename, void* output)
{
  uint16_t fileSize = 0;

  ESP_LOGI(Tag, "Reading file");

  FILE* file = fopen(filename, "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END); // To get file size
    fileSize = ftell(file);
    if(fileSize < 0) {
      return -2;
    }
    fseek(file, 0, SEEK_SET);
    ESP_LOGI(Tag, "File %s open. File size: %d Bytes", filename, fileSize);

    size_t size_read = fread(output, 1, fileSize, file);
    fclose(file);
    if(size_read != fileSize) {
      return -3;
    }

  } else {
    ESP_LOGE(Tag, "Failed to open file for reading");
    return -1;
  }
  
  return 0;
}

//_____________________________________________________________________________

int32_t FileSystem_GetFileSize(const char* filename)
{
  int32_t fileSize = 0;

  FILE* file = fopen(filename, "r");

  if (file != NULL)
  {
    fseek(file, 0, SEEK_END); // To get file size
    fileSize = ftell(file);
    if(fileSize < 0) {
      return -2;
    }
    fclose(file);
    return fileSize;
  } else {
    ESP_LOGE(Tag, "Failed to open file for reading");
    return -1;
  }

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

void FileSystem_EraseFile(const char* fileName)
{
  // Check that the file exists
  if (FileSystem_DoesFileExist(fileName))
  {
    if (unlink(fileName) == 0)
    {
    	ESP_LOGI(Tag, "Erased file: %s\n", fileName);
    }
    else
    {
      ESP_LOGE(Tag, "File unsuccessfully erased: %s", fileName);
    }
  }
}

//_____________________________________________________________________________

void FileSystem_SelectFile(char** fileName, int16_t index, T_Extension extension)
{
  char path[18] = "/spiffs/";
  char name[4];
  char type[6];

  //list("/spiffs/", NULL);

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

  ESP_LOGI(Tag, "Selected file: %s\n", *fileName);
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

//_____________________________________________________________________________

static void list(char* path, char* match)
{
  DIR* dir = NULL;
  struct dirent* ent;
  char type;
  char size[12];
  char tpath[255];
  char tbuffer[80];
  struct stat sb;
  struct tm* tm_info;
  char* lpath = NULL;
  int statok;

  printf("\nList of Directory [%s]\n", path);
  printf("-----------------------------------\n");

  // Open directory
  dir = opendir(path);

  if (!dir)
  {
    printf("Error opening directory\n");
    return;
  }

  // Read directory entries
  uint64_t total = 0;
  int nfiles = 0;
  printf("T  Size      Date/Time         Name\n");
  printf("-----------------------------------\n");

  while ((ent = readdir(dir)) != NULL)
  {
    sprintf(tpath, path);

    if (path[strlen(path) - 1] != '/')
    {
      strcat(tpath, "/");
    }

    strcat(tpath, ent->d_name);
    tbuffer[0] = '\0';

    if ((match == NULL) || (fnmatch(match, tpath, (FNM_PERIOD)) == 0))
    {
      // Get file stat
      statok = stat(tpath, &sb);

      if (statok == 0)
      {
        tm_info = localtime(&sb.st_mtime);
        strftime(tbuffer, 80, "%d/%m/%Y %R", tm_info);
      }
      else
      {
        sprintf(tbuffer, "                ");
      }

      if (ent->d_type == DT_REG)
      {
        type = 'f';
        nfiles++;

        if (statok)
        {
          strcpy(size, "       ?");
        }
        else
        {
          total += sb.st_size;
          //printf("file size = %ld\n", sb.st_size);

          if (sb.st_size < (1024 * 1024))
          {
            sprintf(size, "%8d", (int)sb.st_size);
          }
          else if ((sb.st_size / 1024) < (1024 * 1024))
          {
            sprintf(size, "%6dKB", (int)(sb.st_size / 1024));
          }
          else
          {
            sprintf(size, "%6dMB", (int)(sb.st_size / (1024 * 1024)));
          }
        }
      }
      else
      {
        type = 'd';
        strcpy(size, "       -");
      }

      printf("%c  %s  %s  %s\r\n",
             type,
             size,
             tbuffer,
             ent->d_name
            );
    }
  }

  if (total)
  {
    printf("-----------------------------------\n");

    if (total < (1024 * 1024))
    {
      printf("   %8d", (int)total);
    }
    else if ((total / 1024) < (1024 * 1024))
    {
      printf("   %6dKB", (int)(total / 1024));
    }
    else
    {
      printf("   %6dMB", (int)(total / (1024 * 1024)));
    }

    printf(" in %d file(s)\n", nfiles);
  }

  printf("-----------------------------------\n");

  closedir(dir);

  free(lpath);

  uint32_t tot = 0, used = 0;
  esp_spiffs_info(NULL, &tot, &used);
  printf("SPIFFS: free %d KB of %d KB\n", (tot - used) / 1024, tot / 1024);
  printf("-----------------------------------\n\n");
}

//_____________________________________________________________________________

static int fnmatch(const char* pattern, const char* string, int flags)
{
  const char* stringstart;
  char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++)
    {
      case EOS:
        if ((flags & FNM_LEADING_DIR) && *string == '/')
        {
          return (0);
        }

        return (*string == EOS ? 0 : FNM_NOMATCH);

      case '?':
        if (*string == EOS)
        {
          return (FNM_NOMATCH);
        }

        if (*string == '/' && (flags & FNM_PATHNAME))
        {
          return (FNM_NOMATCH);
        }

        if (*string == '.' && (flags & FNM_PERIOD) &&
            (string == stringstart ||
             ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
        {
          return (FNM_NOMATCH);
        }
        ++string;
        break;

      case '*':
        c = *pattern;
        // Collapse multiple stars.
        while (c == '*')
        {
          c = *++pattern;
        }

        if (*string == '.' && (flags & FNM_PERIOD) &&
            (string == stringstart ||
             ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
        {
          return (FNM_NOMATCH);
        }

        // Optimize for pattern with * at end or before /.
        if (c == EOS)
        {
          if (flags & FNM_PATHNAME)
          {
            return ((flags & FNM_LEADING_DIR) ||
                    strchr(string, '/') == NULL ?
                    0 : FNM_NOMATCH);
          }
          else
          {
            return (0);
          }
        }
        else if ((c == '/') && (flags & FNM_PATHNAME))
        {
          if ((string = strchr(string, '/')) == NULL)
          {
            return (FNM_NOMATCH);
          }

          break;
        }

        // General case, use recursion.
        while ((test = *string) != EOS)
        {
          if (!fnmatch(pattern, string, flags & ~FNM_PERIOD))
          {
            return (0);
          }

          if ((test == '/') && (flags & FNM_PATHNAME))
          {
            break;
          }

          ++string;
        }

        return (FNM_NOMATCH);

      case '[':
        if (*string == EOS)
        {
          return (FNM_NOMATCH);
        }

        if ((*string == '/') && (flags & FNM_PATHNAME))
        {
          return (FNM_NOMATCH);
        }

        if ((pattern = rangematch(pattern, *string, flags)) == NULL)
        {
          return (FNM_NOMATCH);
        }

        ++string;
        break;

      case '\\':
        if (!(flags & FNM_NOESCAPE))
        {
          if ((c = *pattern++) == EOS)
          {
            c = '\\';
            --pattern;
          }
        }
        break;
      // FALLTHROUGH
      default:
        if (c == *string)
        {
        }
        else if ((flags & FNM_CASEFOLD) && (tolower((unsigned char)c) == tolower((unsigned char)*string)))
        {
        }
        else if ((flags & FNM_PREFIX_DIRS) && *string == EOS && ((c == '/' && string != stringstart) ||
                 (string == stringstart + 1 && *stringstart == '/')))
        {
          return (0);
        }
        else
        {
          return (FNM_NOMATCH);
        }

        string++;
        break;
    }

  // NOTREACHED
  return 0;
}

//_____________________________________________________________________________

static const char* rangematch(const char* pattern, char test, int flags)
{
  int negate, ok;
  char c, c2;

  /*
   * A bracket expression starting with an unquoted circumflex
   * character produces unspecified results (IEEE 1003.2-1992,
   * 3.13.2).  This implementation treats it like '!', for
   * consistency with the regular expression syntax.
   * J.T. Conklin (conklin@ngai.kaleida.com)
   */
  if ((negate = (*pattern == '!' || *pattern == '^')))
  {
    ++pattern;
  }

  if (flags & FNM_CASEFOLD)
  {
    test = tolower((unsigned char)test);
  }

  for (ok = 0; (c = *pattern++) != ']';)
  {
    if (c == '\\' && !(flags & FNM_NOESCAPE))
    {
      c = *pattern++;
    }

    if (c == EOS)
    {
      return (NULL);
    }

    if (flags & FNM_CASEFOLD)
    {
      c = tolower((unsigned char)c);
    }

    if (*pattern == '-' && (c2 = *(pattern + 1)) != EOS && c2 != ']')
    {
      pattern += 2;

      if (c2 == '\\' && !(flags & FNM_NOESCAPE))
      {
        c2 = *pattern++;
      }

      if (c2 == EOS)
      {
        return (NULL);
      }

      if (flags & FNM_CASEFOLD)
      {
        c2 = tolower((unsigned char)c2);
      }

      if ((unsigned char)c <= (unsigned char)test &&
          (unsigned char)test <= (unsigned char)c2)
      {
        ok = 1;
      }
    }
    else if (c == test)
    {
      ok = 1;
    }
  }

  return (ok == negate ? NULL : pattern);
}

void listDir(void) {
	list("/spiffs/", NULL);
}

