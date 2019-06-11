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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "fifo.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  uint8_t* data;
  int16_t  length;
} T_Buffer;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern T_FifoBytes* TCPFifoRx;

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

//! \brief     Shut down the socket
//! \pre       First initialize the TCP server
//! \param     None
//! \return    None
extern void TCPServer_ShutDownSocket(void);

//! \brief     Send data
//! \pre       First initialize the TCP server
//! \param     data - Data to be transmitted
//! \param     size - Size of the data
//! \return    None
extern void TCPServer_Send(const uint8_t* data, uint16_t size);

#endif // TCP_SERVER_H_
