//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    uart.h
//! \brief   This module provides the useful functions to use the UART
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef UART_H_
#define UART_H_

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

//! \brief     Initialize the UART protocol
//! \pre       None
//! \param     None
//! \return    None
extern void UART_Init(void);

//! \brief     Write data to the UART line
//! \pre       First initialize the UART protocol
//! \param     None
//! \return    None
extern void UART_Write(const uint8_t* data, uint16_t size);

//! \brief     Read data from the UART line
//! \pre       First initialize the UART protocol
//! \param     None
//! \return    None
extern int UART_Read(uint8_t* data);

//! \brief     Read byte from the UART line
//! \pre       First initialize the UART protocol
//! \param     None
//! \return    None
int UART_ReadByte(uint8_t* data);

//! \brief     Is the reception buffer empty ?
//! \pre       First initialize the UART protocol
//! \param     None
//! \return    None
bool UART_IsReceptionBufferEmpty(void);

//! \brief     Wait until TX FIFO is empty
//! \pre       First initialize the UART protocol
//! \param     None
//! \return    None
extern void UART_WaitUntilTxFifoIsEmpty(void);

#endif // UART_H_
