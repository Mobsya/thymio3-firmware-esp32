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

extern void Codec_StartMP3Player(int16_t index);

extern void Codec_StartWAVRecorder(int16_t index, uint16_t duration_s);

extern void Codec_StartWAVPlayer(int16_t index);

//! \brief     Play a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PlayMP3(int16_t index);

//! \brief     Play a MP3 file from the file system
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PlayMP3FromFileSystem(int16_t index);

//! \brief     Record a WAV file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_RecordWAV(int16_t index, uint16_t duration_s);

//! \brief     Play a WAV file from the file system
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PlayWAV(int16_t index);

//! \brief     Set the sound volume
//! \pre       None
//! \param     volume - Volume
//! \return    None
extern void Codec_SetVolume(int16_t volume);

#endif // CODEC_H_
