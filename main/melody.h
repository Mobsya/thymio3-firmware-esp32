//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    melody.h
//! \brief   This module provides the useful functions to use the melodies
//!
//! \author  Vincent Gonet
//!
//! \version $Id: melody.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef MELODY_H_
#define MELODY_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "sound.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

extern const T_Note ButtonSound[1] =
{
  {E_NoteName_DO_4,  E_NoteValue_Noire, E_Dynamics_p}
};

extern const T_Note CenterButtonSound[3] =
{
  {E_NoteName_DO_4,   E_NoteValue_Croche, E_Dynamics_p},
  {E_NoteName_MI_4,   E_NoteValue_Croche, E_Dynamics_p},
  {E_NoteName_SOL_4,  E_NoteValue_Croche, E_Dynamics_p}
};

extern const T_Note Gamme[8] =
{
  {E_NoteName_DO_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_RE_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_MI_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_FA_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_SOL_4, E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_LA_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_SI_4,  E_NoteValue_DoubleCroche, E_Dynamics_mf},
  {E_NoteName_DO_5,  E_NoteValue_DoubleCroche, E_Dynamics_mf}
};

extern const T_Note FrereJacques[8] =
{
  {E_NoteName_FA_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_SOL_5, E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_LA_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_FA_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_FA_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_SOL_5, E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_LA_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_FA_5,  E_NoteValue_Noire, E_Dynamics_p}
};

extern const T_Note IndianaJones[16] =
{
  {E_NoteName_MI_4,  E_NoteValue_CrochePointee,  E_Dynamics_mf},
  {E_NoteName_FA_4,  E_NoteValue_DoubleCroche,   E_Dynamics_mf},
  {E_NoteName_SOL_4, E_NoteValue_Croche,         E_Dynamics_mf},
  {E_NoteName_DO_5,  E_NoteValue_BlancheCroche,  E_Dynamics_mf},
  {E_NoteName_RE_4,  E_NoteValue_CrochePointee,  E_Dynamics_mf},
  {E_NoteName_MI_4,  E_NoteValue_DoubleCroche,   E_Dynamics_mf},
  {E_NoteName_FA_4,  E_NoteValue_BlanchePointee, E_Dynamics_mf},
  {E_NoteName_SOL_4, E_NoteValue_CrochePointee,  E_Dynamics_mf},
  {E_NoteName_LA_4,  E_NoteValue_DoubleCroche,   E_Dynamics_mf},
  {E_NoteName_SI_4,  E_NoteValue_Croche,         E_Dynamics_mf},
  {E_NoteName_FA_5,  E_NoteValue_BlancheCroche,  E_Dynamics_mf},
  {E_NoteName_LA_4,  E_NoteValue_CrochePointee,  E_Dynamics_mf},
  {E_NoteName_SI_4,  E_NoteValue_DoubleCroche,   E_Dynamics_mf},
  {E_NoteName_DO_5,  E_NoteValue_Noire,          E_Dynamics_mf},
  {E_NoteName_RE_5,  E_NoteValue_Noire,          E_Dynamics_mf},
  {E_NoteName_MI_5,  E_NoteValue_Noire,          E_Dynamics_mf}
};

extern const T_Note Accord[4] =
{
  {E_NoteName_DO_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_MI_5,  E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_SOL_5, E_NoteValue_Noire, E_Dynamics_p},
  {E_NoteName_DO_6,  E_NoteValue_Noire, E_Dynamics_p}
};

extern const T_Note HarryPotter[14] =
{
  {E_NoteName_SI_4,       E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_MI_5,       E_NoteValue_NoirePointee,   E_Dynamics_mp},
  {E_NoteName_SOL_5,      E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_FA_SHARP_5, E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_MI_5,       E_NoteValue_Blanche,        E_Dynamics_mp},
  {E_NoteName_SI_5,       E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_LA_5,       E_NoteValue_BlanchePointee, E_Dynamics_mp},
  {E_NoteName_FA_SHARP_5, E_NoteValue_BlanchePointee, E_Dynamics_mp},
  {E_NoteName_MI_5,       E_NoteValue_NoirePointee,   E_Dynamics_mp},
  {E_NoteName_SOL_5,      E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_FA_SHARP_5, E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_RE_SHARP_5, E_NoteValue_Blanche,        E_Dynamics_mp},
  {E_NoteName_FA_5,       E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_SI_4,       E_NoteValue_BlanchePointee, E_Dynamics_mp}
};

extern const T_Note LeLionEstMort[15] =
{
  {E_NoteName_SOL_4, E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_SI_4,  E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_SI_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_DO_5,  E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_SI_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_SOL_4, E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_SI_4,  E_NoteValue_Noire,          E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_SOL_4, E_NoteValue_NoirePointee,   E_Dynamics_mp},
  {E_NoteName_SI_4,  E_NoteValue_Croche,         E_Dynamics_mp},
  {E_NoteName_LA_4,  E_NoteValue_BlanchePointee, E_Dynamics_mp}
};

extern const T_Note Simpsons[12] =
{
  {E_NoteName_DO_5,       E_NoteValue_NoirePointee, E_Dynamics_f},
  {E_NoteName_MI_5,       E_NoteValue_Noire,        E_Dynamics_f},
  {E_NoteName_FA_SHARP_5, E_NoteValue_Noire,        E_Dynamics_f},
  {E_NoteName_LA_5,       E_NoteValue_Croche,       E_Dynamics_f},
  {E_NoteName_SOL_5,      E_NoteValue_NoirePointee, E_Dynamics_f},
  {E_NoteName_MI_5,       E_NoteValue_Noire,        E_Dynamics_f},
  {E_NoteName_DO_5,       E_NoteValue_Noire,        E_Dynamics_f},
  {E_NoteName_LA_4,       E_NoteValue_Croche,       E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Croche,       E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Croche,       E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Croche,       E_Dynamics_f},
  {E_NoteName_SOL_4,      E_NoteValue_Noire,        E_Dynamics_f}
};

extern const T_Note JamesBond[21] =
{
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_DoubleCroche,   E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_DoubleCroche,   E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Noire,          E_Dynamics_f},
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_SOL_4,      E_NoteValue_DoubleCroche,   E_Dynamics_f},
  {E_NoteName_SOL_4,      E_NoteValue_DoubleCroche,   E_Dynamics_f},
  {E_NoteName_SOL_4,      E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_SOL_4,      E_NoteValue_Noire,          E_Dynamics_f},
  {E_NoteName_FA_SHARP_4, E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_FA_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_MI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_RE_SHARP_5, E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_RE_5,       E_NoteValue_BlancheCroche,  E_Dynamics_f},
  {E_NoteName_SI_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_LA_4,       E_NoteValue_Croche,         E_Dynamics_f},
  {E_NoteName_SI_4,       E_NoteValue_BlanchePointee, E_Dynamics_f},
};

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

#endif // MELODY_H_
