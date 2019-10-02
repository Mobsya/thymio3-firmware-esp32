//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    codec.h
//! \brief   This module provides the useful functions to use the audio codec
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef CODEC_H_
#define CODEC_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

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

//! \brief     Initialize the codec
//! \pre       None
//! \param     None
//! \return    None
extern void Codec_Init(void);

extern void Codec_SetMP3PlayerInfo(int number);

//! \brief     Start the MP3 player task
//! \pre       None
//! \param     None
//! \return    None
extern void Codec_StartMP3Player(int number);

//! \brief     Start the WAV recorder task
//! \pre       None
//! \param     None
//! \return    None
extern void Codec_StartWAVRecorder(int number);

//! \brief     Start the WAV player task
//! \pre       None
//! \param     None
//! \return    None
extern void Codec_StartWAVPlayer(int number);

#endif // CODEC_H_
