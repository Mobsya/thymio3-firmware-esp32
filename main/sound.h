//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
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

//! \details The name of the notes
typedef enum
{
  E_NoteName_Silence     = 0,
  E_NoteName_DO_4        = 31,
  E_NoteName_DO_SHARP_4  = 33,
  E_NoteName_RE_4        = 35,
  E_NoteName_RE_SHARP_4  = 37,
  E_NoteName_MI_4        = 39,
  E_NoteName_FA_4        = 41,
  E_NoteName_FA_SHARP_4  = 44,
  E_NoteName_SOL_4       = 46,
  E_NoteName_SOL_SHARP_4 = 49,
  E_NoteName_LA_4        = 52,
  E_NoteName_LA_SHARP_4  = 55,
  E_NoteName_SI_4        = 58,
  E_NoteName_DO_5        = 62,
  E_NoteName_DO_SHARP_5  = 66,
  E_NoteName_RE_5        = 69,
  E_NoteName_RE_SHARP_5  = 73,
  E_NoteName_MI_5        = 78,
  E_NoteName_FA_5        = 82,
  E_NoteName_FA_SHARP_5  = 87,
  E_NoteName_SOL_5       = 93,
  E_NoteName_SOL_SHARP_5 = 98,
  E_NoteName_LA_5        = 104,
  E_NoteName_LA_SHARP_5  = 110,
  E_NoteName_SI_5        = 117,
  E_NoteName_DO_6        = 124
} T_NoteName;

//! \details The value of the notes
typedef enum
{
  E_NoteValue_TripleCroche,    //!< Thirty-second note (demisemiquaver)
  E_NoteValue_DoubleCroche,    //!< Sixteenth note (semiquaver)
  E_NoteValue_Croche,          //!< Eighth note (quaver)
  E_NoteValue_CrochePointee,   //!<
  E_NoteValue_Noire,           //!< Quarter note (quaver)
  E_NoteValue_NoirePointee,    //!<
  E_NoteValue_Blanche,         //!< Half note (minim)
  E_NoteValue_BlancheCroche,   //!<
  E_NoteValue_BlanchePointee,  //!<
  E_NoteValue_Ronde            //!< Whole note (semibreve)
} T_NoteValue;

//! \details The dynamics of the notes
typedef enum
{
  E_Dynamics_f,   //!< Forte (no scale)
  E_Dynamics_mf,  //!< Mezzo-forte (scale to 1/2)
  E_Dynamics_mp,  //!< Mezzo-piano (scale to 1/4)
  E_Dynamics_p    //!< Piano (scale to 1/8)
} T_Dynamics;

//! \details The definition of a note
typedef struct
{
  T_NoteName  Name;      //!< Name of the note
  T_NoteValue Value;     //!< Value of the note
  T_Dynamics  Dynamics;  //!< Dynamics of the note
} T_Note;

//! \details The tempo of the melody
typedef enum
{
  E_Tempo_Lento    = 50,   //!< Lento (50 bpm)
  E_Tempo_Adagio   = 71,   //!< Adagio (71 bpm)
  E_Tempo_Andante  = 92,   //!< Andante (92 bpm)
  E_Tempo_Moderato = 114,  //!< Moderato (114 bpm)
  E_Tempo_Allegro  = 138,  //!< Allegro (138 bpm)
  E_Tempo_Vivace   = 166,  //!< Vivace (166 bpm)
  E_Tempo_Presto   = 184   //!< Piano (184 bpm)
} T_Tempo;

typedef struct
{
  T_Note* Melody;
  T_Tempo Tempo;
  uint16_t Loop;
  uint16_t Size;
} T_Melody;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern TaskHandle_t AcquisitionTask;

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the sound
//! \pre       None
//! \param     None
//! \return    None
extern void Sound_Init(void);

//! \brief     Start the task to play a sound
//! \pre       First initialize the sound
//! \param     melody - The melody to play
//! \return    None
extern void Sound_StartPlayer(T_Melody* melody);

//! \brief     Start the task to acquire a sound
//! \pre       First initialize the sound
//! \param     None
//! \return    None
extern void Sound_StartAcquisition(void);

//! \brief     Start the task to process a sound
//! \pre       First initialize the sound
//! \param     None
//! \return    None
extern void Sound_StartProcessing(void);

//! \brief     Start the task to record a sound
//! \pre       First initialize the sound
//! \param     None
//! \return    None
extern void Sound_StartRecording(void);

//! \brief     Start the task to replay a sound
//! \pre       First initialize the sound
//! \param     None
//! \return    None
extern void Sound_StartReplaying(void);

//extern void Sound_Process(void);
//extern void Sound_Process(uint16_t* data, uint16_t size);
extern void Sound_Process(float* data, uint16_t size);

//! \brief     Play a note
//! \pre       First initialize the sound
//! \param     note - Note to play
//! \param     duration_ms - Duration of the note in [ms]
//! \return    None
extern void Sound_PlayNote(T_Note note, int16_t duration_ms);

//! \brief     Play a melody
//! \pre       First initialize the sound
//! \param     note - Note to play
//! \param     tempo - Tempo of the melody
//! \param     loop - Number of time the melody will be played
//! \param     size - Number of note in the melody
//! \return    None
extern void Sound_PlayMelody(const T_Note* melody, T_Tempo tempo, uint16_t loop, uint16_t size);

//! \brief     Replay the last sound stored into the FLASH
//! \pre       First initialize the sound
//! \param     None
//! \return    None
extern void Sound_Replay(void);

#endif // SOUND_H_
