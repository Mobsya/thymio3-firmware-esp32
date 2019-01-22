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
//! \version $Id: file_system.h 18076 2017-04-20 12:28:12Z v.gonet $
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

#endif // FILE_SYSTEM_H_
