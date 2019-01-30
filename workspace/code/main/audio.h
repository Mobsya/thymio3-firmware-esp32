//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    audio.h
//! \brief   This module provides the useful functions to play sounds
//!
//! \author  Vincent Gonet
//!
//! \version $Id: audio.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef AUDIO_H_
#define AUDIO_H_

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
  uint16_t SampleRate;
  volatile uint32_t DataSize;                         // The last integer part of count
  volatile uint32_t DataIdx;
  volatile unsigned char *Data;
  volatile float IncreaseBy;                          // The amount to increase the counter by per call to "onTimer"
  volatile float Count;                               // The counter counting up, we check this to see if we need to send
  volatile int32_t LastIntCount;                     // The last integer part of count
  volatile bool Completed;
  volatile bool Mix;								// Should sound be mixed with others or stop all and play on its own
  //volatile T_PlayListItem *ParentPlayListItem;
  volatile uint8_t LastValue;							// Last value returned from NextByte function
  //T_Wav(unsigned char *WavData);
  //uint8_t NextByte();
} T_Wav;

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
//extern void Audio_Init(void);
extern void Audio_Init(T_Wav* wav, uint8_t* wavData);

extern void Audio_FillBuffer(void);

//! \brief     Run the xxx task
//! \pre       First initialize the xxx
//! \param     None
//! \return    None
//extern void Audio_PlayWav(T_Wav* wav, bool mix);
extern void Audio_PlayWav(T_Wav* wav);

#endif // AUDIO_H_
