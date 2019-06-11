//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    wifi_update.h
//! \brief   This module provides the useful functions to use the WIFI for the update
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef WIFI_UPDATE_H_
#define WIFI_UPDATE_H_

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

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

extern void WIFIUpdate_InitNVS(void);

//! \brief     Initialize the WIFI for the update
//! \pre       None
//! \param     None
//! \return    None
extern void WIFIUpdate_Init(void);

extern void WIFIUpdate_RunTask(void* pvParameter);

//! \brief     Connect asynchronously to the defined access point.
//! \pre       None
//! \param     None
//! \return    None
extern void WifiUpdate_Connect(const char* ssid, const char* password);

extern void WifiUpdate_Disconnect(void);

//! \brief     Check whether the module is currently connected to an access point.
//! \pre       None
//! \param     None
//! \return    None
extern bool WIFIUpdate_IsConnected(void);

extern void WIFIUpdate_GetIPAddress(void);

#endif // WIFI_UPDATE_H_
