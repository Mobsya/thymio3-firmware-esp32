//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sequence.h
//! \brief   This module provides the useful functions to use the sequence mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SEQUENCE_H_
#define SEQUENCE_H_

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

//! \brief     Initialize the sequence mode
//! \pre       None
//! \param     None
//! \return    None
extern void Sequence_Init(void);

//! \brief     Start the sequence mode
//! \pre       First initialize the sequence mode
//! \param     None
//! \return    None
extern void Sequence_Start(void);

//! \brief     Stop the sequence mode
//! \pre       First initialize the sequence mode
//! \param     None
//! \return    None
extern void Sequence_Stop(void);

//! \brief     Run the sequence mode
//! \pre       First initialize the sequence mode
//! \param     None
//! \return    None
extern void Sequence_Run(void);

#endif // SEQUENCE_H_
