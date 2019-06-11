//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    cosine_generator.h
//! \brief   This module provides the useful functions to use the cosine generator
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef COSINE_GENERATOR_H_
#define COSINE_GENERATOR_H_

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

//! \brief     Initialize the cosine generator
//! \pre       None
//! \param     None
//! \return    None
extern void CosineGenerator_Init(void);

//! \brief     Enable the cosine generator
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
extern void CosineGenerator_Enable(void);

//! \brief     Disable the cosine generator
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
extern void CosineGenerator_Disable(void);

//! \brief     Configure the cosine signal
//! \pre       First initialize the cosine generator
//! \param     noteName - Name of the note
//! \param     noteDynamics - Dynamics of the note
//! \return    None
extern void CosineGenerator_ConfigureSignal(int16_t noteName, int16_t noteDynamics);

#endif // COSINE_GENERATOR_H_
