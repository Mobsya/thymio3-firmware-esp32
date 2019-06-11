//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ota_update.h
//! \brief   This module provides the useful functions to perform the over-the-air update
//
//! \author  Vincent Gonet
//
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef OTA_UPDATE_H_
#define OTA_UPDATE_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Ota_Status_Ok = 0,
  E_Ota_Status_PartitionNotFound,
  E_Ota_Status_PartitionNotActivated,
  E_Ota_Status_StartFailed,
  E_Ota_Status_WriteFailed,
  E_Ota_Status_EndFailed
} T_Ota_Status;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the over-the-air update.
//! \pre       None
//! \param     None
//! \return    None
extern void OtaUpdate_Init(void);

//! \brief     Check whether the over-the-air update is in progress.
//! \pre       None
//! \param     None
//! \return    None
extern bool OtaUpdate_IsInProgress(void);

//! \brief     Start the over-the-air update.
//! \pre       None
//! \param     None
//! \return    None
extern T_Ota_Status OtaUpdate_Start(void);

//! \brief     Start the over-the-air update.
//! \pre       None
//! \param     None
//! \return    None
// Call this function for every line with up to 4 kBytes of hex data.
extern T_Ota_Status OtaUpdate_WriteHexData(const char* hexData, int len);

//! \brief     Finish the over-the-air update.
//! \pre       None
//! \param     None
//! \return    None
extern T_Ota_Status OtaUpdate_Finish(void);

//! \brief     Dump the over-the-air update information.
//! \pre       None
//! \param     None
//! \return    None
extern void OtaUpdate_DumpInformation(void);

#endif // OTA_UPDATE_H_
