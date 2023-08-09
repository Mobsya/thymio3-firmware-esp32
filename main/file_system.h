//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    file_system.h
//! \brief   This module provides the useful functions to use the file system
//!
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef FILE_SYSTEM_H_
#define FILE_SYSTEM_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Extension_MP3,
  E_Extension_WAV
} T_Extension;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------
extern void listDir(void);
//! \brief     Initialize the file system
//! \pre       None
//! \param     None
//! \return    None
//extern void FileSystem_Init(void);

//! \brief     Create the file
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \return    True if the file has been created, false otherwise
extern bool FileSystem_CreateFile(const char* filename);

//! \brief     Write to the file
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \param     input - Data to write
//! \param     size - Size of the data
//! \return    None
extern void FileSystem_Write(const char* filename, void* input, long int size);

//! \brief     Read number of bytes from the file
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \param     output - Read Data
//! \param     size - Size of the data
//! \return    None
extern void FileSystem_Read(const char* filename, void* output, long int size);

//! \brief     Read complete file (with buffer allocation)
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \param     output - Allocate the required buffer size based on file size and fill buffer with read data
//! \param     size - Size of the file
//! \return    None
extern int8_t FileSystem_Read2(const char* filename, void* output, long int *size);

//! \brief     Read complete file (without buffer allocation)
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \param     output - read data
//! \return    None
extern int8_t FileSystem_Read3(const char* filename, void* output);

//! \brief     Get file size
//! \pre       First initialize the file system
//! \param     fileName - File name
//! \return    number of bytes
extern int32_t FileSystem_GetFileSize(const char* filename);

//! \brief     Does the file exist ?
//! \pre       None
//! \param     fileName - File name
//! \return    True if the file exists, false otherwise
extern bool FileSystem_DoesFileExist(char* fileName);

//! \brief     Erase a file
//! \pre       None
//! \param     fileName - File name
//! \return    None
extern void FileSystem_EraseFile(const char* fileName);

//! \brief     Select a file
//! \pre       None
//! \param     fileName - File name
//! \param     index - Index of the file
//! \param     extension - Extension of the file
//! \return    None
extern void FileSystem_SelectFile(char** fileName, int16_t index, T_Extension extension);

//! \brief     Write a WAV file
//! \pre       None
//! \param     fileName - File name
//! \param     numSamples - Number of samples
//! \param     data - Data
//! \param     sampleRate - Sample rate
//! \param     channel - Channel
//! \return    None
extern void FileSystem_WriteWAVFile(char* fileName, uint32_t numSamples, int16_t* data, uint16_t sampleRate,
                                    uint8_t channel);

#endif // FILE_SYSTEM_H_
