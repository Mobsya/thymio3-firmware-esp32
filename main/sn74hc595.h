//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sn74hc595.h
//! \brief   This module provides the useful functions to use the SN74HC595
//!          8-Bit shift registers with 3-state output registers
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SN74HC595_H_
#define SN74HC595_H_

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

//! \brief     Initialize the SN74HC595 device.
//! \pre       None
//! \param     None
//! \return    None
extern void SN74HC595_Init(void);

//! \brief     Disable the outputs QA � QH.
//! \pre       First initialize the shift registers
//! \param     None
//! \return    None
extern void SN74HC595_DisableOutputs(void);

//! \brief     Enable the outputs QA � QH.
//! \pre       First initialize the shift registers
//! \param     None
//! \return    None
extern void SN74HC595_EnableOutputs(void);

//! \brief     Clear the shift register.
//! \pre       First initialize the shift registers
//! \param     None
//! \return    None
extern void SN74HC595_ClearShiftRegister(void);

//! \brief     Fill the shift register.
//! \pre       First initialize the shift registers
//! \param     data - Data to be transmitted to the shift register
//! \param     size - Size of the data
//! \return    None
extern void SN74HC595_Fill(uint8_t* data, uint16_t size);

//! \brief     Store the shift register data in the storage register.
//! \pre       First initialize the shift registers
//! \param     None
//! \return    None
extern void SN74HC595_StoreShiftRegisterData(void);

#endif // SN74HC595_H_
