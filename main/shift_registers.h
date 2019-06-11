//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    shift_registers.h
//! \brief   This module provides the useful functions to use the SN74HC595
//!          8-Bit shift registers with 3-state output registers
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SHIFT_REGISTERS_H_
#define SHIFT_REGISTERS_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define REGISTERS_NUM             5u  //!< Number of shift registers
#define PINS_PER_REGISTER_NUM     8u  //!< Number of pins per shift register

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

//! \brief     Initialize the shift registers.
//! \pre       None
//! \param     None
//! \return    None
extern void ShiftRegisters_Init(void);

//! \brief     Fill the shift registers.
//! \pre       First initialize the shift registers
//! \param     data - Data to be transmitted to the shift register
//! \param     size - Size of the data
//! \return    None
extern void ShiftRegisters_Fill(uint8_t* data, uint16_t size);

#endif // SHIFT_REGISTERS_H_
