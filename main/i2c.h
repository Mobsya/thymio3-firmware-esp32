//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    i2c.h
//! \brief   This module provides the useful functions to use the I2C protocol
//!
//! \author  Vincent Gonet
//!
//! \version $Id: i2c.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef I2C_H_
#define I2C_H_

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

//! \brief     Initialize the I2C protocol
//! \pre       None
//! \param     None
//! \return    None
extern void I2C_Init(void);

//! \brief     Write a byte
//! \pre       First initialize the I2C protocol
//! \param     slaveAddress - Slave address of the device
//! \param     data - Byte to write
//! \return    None
extern void I2C_WriteByte(uint8_t slaveAddress, uint8_t data);

//! \brief     Read a byte
//! \pre       First initialize the I2C protocol
//! \param     slaveAddress - Slave address of the device
//! \param     registerAddress - Register address to read
//! \return    Read byte
extern uint8_t I2C_ReadByte(uint8_t slaveAddress, uint8_t registerAddress);

//! \brief     Write data to a specific register
//! \pre       First initialize the I2C protocol
//! \param     slaveAddress - Slave address of the device
//! \param     registerAddress - Register address to write
//! \param     data - Data to write
//! \param     size - Size of the data
//! \return    None
extern void I2C_WriteToAddress(uint8_t slaveAddress, uint8_t registerAddress, uint8_t* data, uint16_t size);

//! \brief     Read data from a specific register
//! \pre       First initialize the I2C protocol
//! \param     slaveAddress - Slave address of the device
//! \param     registerAddress - Register address to read
//! \param     data - Data to read
//! \param     size - Size of the data
//! \return    None
extern void I2C_ReadFromAddress(uint8_t slaveAddress, uint8_t registerAddress, uint8_t* data, uint16_t size);

#endif // I2C_H_
