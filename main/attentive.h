//_____________________________________________________________________________
//
// Copyright (C) 2023                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    attentive.h
//! \brief   This module provides the useful functions to use the attentive mode
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ATTENTIVE_H_
#define ATTENTIVE_H_

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

//! \brief     Initialize the attentive mode
//! \pre       None
//! \param     None
//! \return    None
extern void Attentive_Init(void);

//! \brief     Start the attentive mode
//! \pre       First initialize the attentive mode
//! \param     None
//! \return    None
extern void Attentive_Start(void);

//! \brief     Stop the attentive mode
//! \pre       First initialize the attentive mode
//! \param     None
//! \return    None
extern void Attentive_Stop(void);

//! \brief     Run the attentive mode
//! \pre       First initialize the attentive mode
//! \param     None
//! \return    None
extern void Attentive_Run(void);

#endif // ATTENTIVE_H_
