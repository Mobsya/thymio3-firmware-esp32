//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    musician.h
//! \brief   This module provides the useful functions to use the musician mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef MUSICIAN_H_
#define MUSICIAN_H_

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

//! \brief     Initialize the musician mode
//! \pre       None
//! \param     None
//! \return    None
extern void Musician_Init(void);

//! \brief     Start the musician mode
//! \pre       First initialize the musician mode
//! \param     None
//! \return    None
extern void Musician_Start(void);

//! \brief     Stop the musician mode
//! \pre       First initialize the musician mode
//! \param     None
//! \return    None
extern void Musician_Stop(void);

//! \brief     Run the musician mode
//! \pre       First initialize the musician mode
//! \param     None
//! \return    None
extern void Musician_Run(void);

#endif // MUSICIAN_H_
