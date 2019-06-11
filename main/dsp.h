//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    dsp.h
//! \brief   This module provides the useful functions to use the xxx
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef DSP_H_
#define DSP_H_

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

typedef struct
{
  float    Value;
  uint16_t Index;
} T_Max;  //!< Maximum value (and his index) of a buffer

typedef struct
{
  uint16_t Value;
  uint16_t Index;
} T_MaxUnsigned;  //!< Maximum value (and his index) of a buffer

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Get the highest value of a buffer
//! \pre       None
//! \param     None
//! \return    None
//extern float DSP_GetMaxValue(float* buffer, uint16_t size);
extern void DSP_GetMaxValue(float* buffer, uint16_t size, T_Max* max);

extern void DSP_GetMaxUnsignedValue(uint16_t* buffer, uint16_t size, T_MaxUnsigned* max);

//! \brief     Calculate the mean value of a buffer
//! \pre       None
//! \param     None
//! \return    None
extern float DSP_CalculateMeanValue(float* buffer, uint16_t size);

//! \brief     Calculate the DFT of a buffer
//! \pre       None
//! \param     None
//! \return    None
//extern void DSP_CalculateDFT(uint16_t* src, uint16_t* destRe, uint16_t* destIm, uint16_t size);
extern void DSP_CalculateDFT(float* src, float* destRe, float* destIm, uint16_t size);

//extern void DSP_CalculateDFTOutputMag(uint16_t* srcRe, uint16_t* srcIm, uint16_t* dest, uint16_t size);
extern void DSP_CalculateDFTOutputMag(float* srcRe, float* srcIm, float* dest, uint16_t size);

#endif // DSP_H_
