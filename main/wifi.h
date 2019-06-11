//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    wifi.h
//! \brief   This module provides the useful functions to use the WIFI
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef WIFI_H_
#define WIFI_H_

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

extern void WIFI_Configure(void);

//! \brief     Initialize the WIFI
//! \pre       None
//! \param     None
//! \return    None
extern void WIFI_Init(void);

extern void WIFI_InitNVS(void);

extern void WIFI_Start(void);

extern void WIFI_Connect(void);

extern void WIFI_Disconnect(void);

extern bool WIFI_IsConnected(void);

extern void WIFI_GetIPAddress(void);

extern void WIFI_WaitForIP(void);

#endif // WIFI_H_
