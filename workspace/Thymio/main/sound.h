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

extern void Sound_InitSine(T_Volume volume);

//! \brief     Initialize the xxx
//! \pre       None
//! \param     None
//! \return    None
extern void Sound_Init(void);

//! \brief     Run the xxx task
//! \pre       First initialize the xxx
//! \param     None
//! \return    None
extern void Sound_Task(void);

extern void Sound_Generate(void);

extern void Sound_Add(float frequency, float amplitude);

extern void Sound_Enable(void);

void Sound_PlayNote(uint16_t note, uint32_t duration_us, T_Volume volume);

extern void Sound_Record(void);

#endif // SOUND_H_
