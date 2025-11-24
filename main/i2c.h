//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
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

//! \details The status of the I2C bus
enum
{
  E_I2CBus_Busy,
  E_I2CBus_Available
};
typedef uint8_t T_I2CBus;

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

//! \brief     Get the bus status
//! \pre       First initialize the I2C protocol
//! \param     None
//! \return    Bus status
extern T_I2CBus I2C_GetBusStatus(void);

//! \brief     Update the bus status
//! \pre       First initialize the I2C protocol
//! \param     status - Status of the bus
//! \return    None
extern void I2C_UpdateBusStatus(T_I2CBus status);

extern void I2C_WriteAndRead(uint8_t slaveAddress, uint8_t* txData, uint16_t txSize, uint8_t* rxData, uint16_t rxSize);
extern void I2C_Write(uint8_t slaveAddress, uint8_t* data, uint16_t size);
extern void I2C_Read(uint8_t slaveAddress, uint8_t* data, uint16_t size);


extern void I2C_DeleteDriver(void);

#endif // I2C_H_
