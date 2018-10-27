//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sound.h
//! \brief   This module provides the useful functions to generate the sound
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sound.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef SOUND_H_
#define SOUND_H_

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

typedef enum
{
  E_Volume_ppp = 16,
  E_Volume_pp  = 32,
  E_Volume_p   = 48,
  E_Volume_mp  = 64,
  E_Volume_mf  = 80,
  E_Volume_f   = 96,
  E_Volume_ff  = 112,
  E_Volume_fff = 127
} T_Volume;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the xxx
//! \pre       None
//! \param     None
//! \return    None
extern void Sound_Init(void);

//! \brief     Run the xxx task
//! \pre       First initialize the xxx
//! \param     None
//! \return    None
extern void Sound_Task(int16_t note);

extern void Sound_SetFrequency(int16_t clk_8m_div, int16_t frequency_step);

extern void Sound_ScaleOutput(int16_t scale);

extern void Sound_OffsetOutput(int16_t offset);

extern void Sound_InvertOutput(int16_t invert);

#endif // SOUND_H_
