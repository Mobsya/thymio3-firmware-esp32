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

extern void Codec_CreateWAVFile(int16_t index, int16_t freq_Hz);

//! \brief     Play a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PlayMP3FileFromFlash(int16_t index);

//! \brief     Play a MP3 file from the file system
//! \pre       First initialize the codec
//! \param     index - Index of the file to play
//! \return    None
extern void Codec_PlayMP3File(int16_t index);

//! \brief     Play a WAV file from the file system
//! \pre       First initialize the codec
//! \param     index - Index of the file to play
//! \return    None
extern void Codec_PlayWAVFile(int16_t index);

//! \brief     Pause a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PauseMP3FileFromFlash(void);

//! \brief     Pause a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PauseMP3File(void);

//! \brief     Pause a WAV file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_PauseWAVFile(void);

//! \brief     Resume a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_ResumeMP3FileFromFlash(void);

//! \brief     Resume a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_ResumeMP3File(void);

//! \brief     Resume a WAV file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern void Codec_ResumeWAVFile(void);

extern int Codec_GetMP3PlayedTime(void);

extern int Codec_GetWAVPlayedTime(void);

//! \brief     Record a WAV file
//! \pre       First initialize the codec
//! \param     index - Index of the sound to record
//! \param     duration_s - Duration of the sound to record in [s]
//! \return    None
extern void Codec_RecordWAVFile(int16_t index, uint16_t duration_s);

//! \brief     Set the sound volume
//! \pre       First initialize the codec
//! \param     volume - Volume
//! \return    None
extern void Codec_SetVolume(int16_t volume);

//! \brief     Is the closing sound finished?
//! \pre       First initialize the codec
//! \param     None
//! \return    True if the closing sound is finished, false otherwise
extern bool Codec_IsClosingSoundFinished(void);

#endif // CODEC_H_
