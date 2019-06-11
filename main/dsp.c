//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    dsp.c
//! \brief   This module provides the useful functions to use the xxx
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <math.h>

#include "dsp.h"

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
// Private Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void DSP_GetMaxValue(float* buffer, uint16_t size, T_Max* max)
{
  max->Value = buffer[0];
  max->Index = 0;

  for (uint16_t index = 1u; index < size; index++)
  {
    if (buffer[index] > max->Value)
    {
      max->Value = buffer[index];
      max->Index = index;
    }
  }
}

//_____________________________________________________________________________

void DSP_GetMaxUnsignedValue(uint16_t* buffer, uint16_t size, T_MaxUnsigned* max)
{
  max->Value = buffer[0];
  max->Index = 0;

  for (uint16_t index = 1u; index < size; index++)
  {
    if (buffer[index] > max->Value)
    {
      max->Value = buffer[index];
      max->Index = index;
    }
  }
}

//_____________________________________________________________________________

float DSP_CalculateMeanValue(float* buffer, uint16_t size)
{
  float mean = buffer[0];

  for (uint16_t index = 1u; index < size; index++)
  {
    mean += buffer[index];
  }

  return (mean / size);
}

//_____________________________________________________________________________

//void DSP_CalculateDFT(uint16_t* src, uint16_t* destRe, uint16_t* destIm, uint16_t size)
void DSP_CalculateDFT(float* src, float* destRe, float* destIm, uint16_t size)
{
  float angle = 2 * M_PI;

  for (uint16_t i = 0u; i < (size / 2u); i++)
  {
    destRe[i] = 0;
    destIm[i] = 0;
  }

  for (uint16_t j = 0u; j < (size / 2u); j++)
  {
    for (uint16_t k = 0u; k < size; k++)
    {
      destRe[j] += (src[k] * cos((angle * j * k) / size));
      destIm[j] -= (src[k] * sin((angle * j * k) / size));
    }
  }
}

//_____________________________________________________________________________

//void DSP_CalculateDFTOutputMag(uint16_t* srcRe, uint16_t* srcIm, uint16_t* dest, uint16_t size)
void DSP_CalculateDFTOutputMag(float* srcRe, float* srcIm, float* dest, uint16_t size)
{
  for (uint16_t index = 0u; index < size; index++)
  {
    dest[index] = sqrt(pow(srcRe[index], 2) + pow(srcIm[index], 2));
  }
}
