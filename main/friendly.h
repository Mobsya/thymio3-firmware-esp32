//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    friendly.h
//! \brief   This module provides the useful functions to use the friendly mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef FRIENDLY_H_
#define FRIENDLY_H_

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

//! \brief     Initialize the friendly mode
//! \pre       None
//! \param     None
//! \return    None
extern void Friendly_Init(void);

//! \brief     Start the friendly mode
//! \pre       First initialize the friendly mode
//! \param     None
//! \return    None
extern void Friendly_Start(void);

//! \brief     Stop the friendly mode
//! \pre       First initialize the friendly mode
//! \param     None
//! \return    None
extern void Friendly_Stop(void);

//! \brief     Run the friendly mode
//! \pre       First initialize the friendly mode
//! \param     None
//! \return    None
extern void Friendly_Run(void);

#endif // FRIENDLY_H_
