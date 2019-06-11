//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    adc.h
//! \brief   This module provides the useful functions to use the internal ADC
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ADC_H_
#define ADC_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "stdint.h"

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

//! \brief     Initialize the internal ADC
//! \pre       None
//! \param     None
//! \return    None
extern void ADC_Init(void);

//! \brief     Acquire the Ground IR ADC values
//! \pre       First initialize the internal ADC
//! \param     value - Value read on the ADC channels
//! \return    None
extern void ADC_AcquireGroundIRValues(uint16_t* value);

extern void ADC_AcquireMicrophoneValues(uint16_t* value);

#endif // ADC_H_
