//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
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

//! \brief     Initialize the HSPI SPI protocol
//! \pre       None
//! \param     None
//! \return    None
extern void Spi_InitHSPI(void);

//! \brief     Initialize the HSPI SPI protocol
//! \pre       None
//! \param     None
//! \return    None
extern void Spi_InitVSPI(void);

//! \brief     Add a device on the HSPI SPI bus
//! \pre       First initialize the HSPI SPI
//! \param     device - Device to add on the HSPI SPI bus
//! \param     csPin, Chip select pin number
//! \return    None
extern void Spi_AddDeviceHSPI(spi_device_handle_t* device, int csPin);

//! \brief     Add a device on the VSPI SPI bus
//! \pre       First initialize the VSPI SPI
//! \param     device - Device to add on the VSPI SPI bus
//! \param     csPin, Chip select pin number
//! \return    None
extern void Spi_AddDeviceVSPI(spi_device_handle_t* device, int csPin);

//! \brief     Write a data
//! \pre       First initialize the SPI
//! \param     device - Device
//! \param     data - Data to write
//! \param     size - Size of the data
//! \return    None
extern void Spi_Write(spi_device_handle_t device, uint8_t* data, uint16_t size);
//extern void Spi_Write(uint8_t* data, uint16_t size);

//extern void Spi_WriteVSPI(spi_device_handle_t device, uint16_t* txBuffer, uint16_t size);
extern void Spi_WriteVSPI(spi_device_handle_t device, uint16_t* txBuffer, uint16_t size);
//extern void Spi_WriteVSPI(spi_device_handle_t device, uint8_t* txBuffer, uint8_t* rxBuffer, uint16_t size);

extern void Spi_ReadVSPI(spi_device_handle_t device, uint16_t* rxBuffer, uint16_t size);
//extern void Spi_ReadVSPI(spi_device_handle_t device, uint8_t* rxBuffer, uint16_t size);

//extern uint32_t lcd_get_id(spi_device_handle_t device, uint8_t* rxBuffer, uint16_t size);
//extern uint8_t* lcd_get_id(spi_device_handle_t device, uint8_t* rxBuffer, uint16_t size);

#endif // SPI_H_
