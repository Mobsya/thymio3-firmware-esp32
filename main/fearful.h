//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    fearful.h
//! \brief   This module provides the useful functions to use the fearful mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef FEARFUL_H_
#define FEARFUL_H_

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

//! \brief     Initialize the fearful mode
//! \pre       None
//! \param     None
//! \return    None
extern void Fearful_Init(void);

//! \brief     Start the fearful mode
//! \pre       First initialize the fearful mode
//! \param     None
//! \return    None
extern void Fearful_Start(void);

//! \brief     Stop the fearful mode
//! \pre       First initialize the fearful mode
//! \param     None
//! \return    None
extern void Fearful_Stop(void);

//! \brief     Run the fearful mode
//! \pre       First initialize the fearful mode
//! \param     None
//! \return    None
extern void Fearful_Run(void);

#endif // FEARFUL_H_
