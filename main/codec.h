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
//! \author  Vincent Gonet, Stefano Morgani.
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef CODEC_H_
#define CODEC_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#include "esp_err.h"
#include "audio_tone_uri.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define MAX_RECORD_SIZE 240000 // 12 KHz sampling rate * 2 bytes per sample * 10 seconds

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

extern uint32_t Codec_CreateWAVFile(int16_t *buffer, int16_t freq_Hz, uint16_t msec);

//! \brief     Play pre-built sounds (from flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    Error code
extern esp_err_t Codec_PlayOnboardSound(tone_type_t index);

//! \brief     Play a MP3 file from RAM memory
//! \pre       First initialize the codec
//! \param     mp3 - wav data
//! \param     num_bytes - size of wav file
//! \return    Error code
esp_err_t Codec_PlayMP3File(uint8_t* mp3, uint32_t num_bytes);

//! \brief     Play a WAV file from RAM memory
//! \pre       First initialize the codec
//! \param     wav - wav data
//! \param     num_bytes - size of wav file
//! \return    Error code
extern esp_err_t Codec_PlayWAVFile(uint8_t* wav, uint32_t num_bytes);

//! \brief     Get the played time [s/10] of a MP3 file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern int Codec_GetMP3PlayedTime(void);

//! \brief     Get the played time [s/10] of a WAV file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern int Codec_GetWAVPlayedTime(void);

//! \brief     Record a WAV file and save it to RAM memory
//! \pre       First initialize the codec
//! \param     duration_s - Duration of the sound to record in [s]
//! \return    None
extern void Codec_RecordWAVFile(uint16_t duration_s);

//! \brief     Set the sound volume
//! \pre       First initialize the codec
//! \param     volume - Volume
//! \return    None
extern void Codec_SetVolume(int16_t volume);

//! \brief     Is the sound finished?
//! \pre       First initialize the codec
//! \param     None
//! \return    True if the sound is finished, false otherwise
extern bool Codec_IsSoundFinished(void);

//! \brief     Is the recording finished?
//! \pre       First initialize the codec
//! \param     None
//! \return    True if the recording is finished, false otherwise
bool Codec_IsRecordFinished(void);

//! \brief     Get the memory pointer of the recorded sound data
//! \pre       First initialize the codec
//! \param     None
//! \return    Memory pointer
uint8_t* Codec_GetRecordPtr(void);

//! \brief     Get the number of bytes of the recorded sound data
//! \pre       First initialize the codec
//! \param     None
//! \return    Number of bytes
uint32_t Codec_GetRecordSize(void);

//! \brief     Play the last recorded sound directly from RAM memory
//! \pre       First initialize the codec
//! \param     None
//! \return    Error code
esp_err_t Codec_PlayRecorded(void);

//! \brief     Pause any running play or recording (can be resumed with "Codec_Resume").
//! \pre       First initialize the codec
//! \param     None
//! \return    Error code
esp_err_t  Codec_Pause(void);

//! \brief     Stop any running play or recording (cannot be resumed).
//! \pre       First initialize the codec
//! \param     None
//! \return    Error code
esp_err_t  Codec_Stop(void);

//! \brief     Resume a previously paused (with "Codec_Pasue") play or recording.
//! \pre       First initialize the codec
//! \param     None
//! \return    Error code
esp_err_t  Codec_Resume(void);

//! \brief     Clear audio events (played and recorded).
//! \pre       First initialize the codec
//! \param     None
//! \return    None
void  Codec_ClearEvents(void);


#endif // CODEC_H_
