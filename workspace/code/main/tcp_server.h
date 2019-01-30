//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    tcp_server.h
//! \brief   This module provides the useful functions to use the TCP server
//!
//! \author  Vincent Gonet
//!
//! \version $Id: tcp_server.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

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

//! \brief     Initialize the TCP server
//! \pre       None
//! \param     None
//! \return    None
extern void TCPServer_Init(void);

//! \brief     Run the TCP server task
//! \pre       First initialize the TCP server
//! \param     None
//! \return    None
extern void TCPServer_RunTask(void);

//! \brief     Check that the socket is accepted
//! \pre       First initialize the TCP server
//! \param     None
//! \return    True if the socket is accepted, false otherwise
extern bool TCPServer_IsSocketAccepted(void);

extern void TCPServer_Send(const uint8_t* data, uint16_t length);

extern uint8_t* TCPServer_GetRxBuffer(void);

#endif // TCP_SERVER_H_
