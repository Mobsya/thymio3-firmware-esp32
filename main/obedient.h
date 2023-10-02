//_____________________________________________________________________________
//
// Copyright (C) 2023                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    obedient.h
//! \brief   This module provides the useful functions to use the obedient mode
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef OBEDIENT_H_
#define OBEDIENT_H_

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

//! \brief     Initialize the obedient mode
//! \pre       None
//! \param     None
//! \return    None
extern void Obedient_Init(void);

//! \brief     Start the obedient mode
//! \pre       First initialize the obedient mode
//! \param     None
//! \return    None
extern void Obedient_Start(void);

//! \brief     Stop the obedient mode
//! \pre       First initialize the obedient mode
//! \param     None
//! \return    None
extern void Obedient_Stop(void);

//! \brief     Run the obedient mode
//! \pre       First initialize the obedient mode
//! \param     None
//! \return    None
extern void Obedient_Run(void);

#endif // OBEDIENT_H_
