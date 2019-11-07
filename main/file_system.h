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
//! \author  Vincent Gonet
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

//! \brief     Initialize the file system
//! \pre       None
//! \param     None
//! \return    None
extern void FileSystem_Init(void);

//! \brief     Create the settings file
//! \pre       First initialize the file system
//! \param     None
//! \return    None
extern void FileSystem_CreateSettingsFile(void);

//! \brief     Write to the settings file
//! \pre       First initialize the file system
//! \param     None
//! \return    None
extern void FileSystem_WriteSettingsFile(void);

//! \brief     Read from the settings file
//! \pre       First initialize the file system
//! \param     None
//! \return    None
extern void FileSystem_ReadSettingsFile(void);

//! \brief     Update the settings (not in the file)
//! \pre       None
//! \param     None
//! \return    None
extern void FileSystem_UpdateSettings(int16_t leftMotor, int16_t rightMotor);

//! \brief     Does the file exist ?
//! \pre       None
//! \param     fileName - File name
//! \return    True if the file exists, false otherwise
extern bool FileSystem_DoesFileExist(char* fileName);

//! \brief     Select a file
//! \pre       None
//! \param     fileName - File name
//! \param     index - Index of the file
//! \param     extension - Extension of the file
//! \return    None
extern void FileSystem_SelectFile(char** fileName, int16_t index, T_Extension extension);

//! \brief     Erase a file
//! \pre       None
//! \param     fileName - File name
//! \return    None
extern void FileSystem_EraseFile(char* fileName);

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
