//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    spi.h
//! \brief   This module provides the useful functions to use the SPI protocol
//!
//! \author  Vincent Gonet
//!
//! \version $Id: spi.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef SPI_H_
#define SPI_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <driver/spi_master.h>

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

//! \brief     Initialize the SPI protocol
//! \pre       None
//! \param     None
//! \return    None
extern void Spi_Init(void);

//! \brief     Add a device on the SPI bus
//! \pre       First initialize the SPI
//! \param     device - Device to add on the SPI bus
//! \param     csPin, Chip select pin number
//! \return    None
extern void Spi_AddDevice(spi_device_handle_t* device, int csPin);

//! \brief     Write a data
//! \pre       First initialize the SPI
//! \param     device - Device
//! \param     data - Data to write
//! \param     size - Size of the data
//! \return    None
extern void Spi_Write(spi_device_handle_t device, uint8_t* data, uint16_t size);
//extern void Spi_Write(uint8_t* data, uint16_t size);

#endif // SPI_H_
