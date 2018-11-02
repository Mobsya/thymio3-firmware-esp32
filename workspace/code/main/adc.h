//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: adc.h 18076 2017-04-20 12:28:12Z v.gonet $
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

//! \brief     Acquire the ADC values
//! \pre       First initialize the internal ADC
//! \param     None
//! \return    None
extern void ADC_AcquireValues(int16_t* value);

#if 0
//! \brief     Get the microphone value
//! \pre       First initialize the internal ADC
//! \param     None
//! \return    None
extern uint16_t ADC_GetMicrophoneValue(void);

//! \brief     Get the left ground IR value
//! \pre       First initialize the internal ADC
//! \param     None
//! \return    None
extern uint16_t ADC_GetLeftGroundIRValue(void);

//! \brief     Get the right ground IR value
//! \pre       First initialize the internal ADC
//! \param     None
//! \return    None
extern uint16_t ADC_GetRightGroundIRValue(void);
#endif
#endif // ADC_H_
