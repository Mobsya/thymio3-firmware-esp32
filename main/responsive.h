//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    responsive.h
//! \brief   This module provides the useful functions to use the responsive mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef RESPONSIVE_H_
#define RESPONSIVE_H_

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

//! \brief     Initialize the responsive mode
//! \pre       None
//! \param     None
//! \return    None
extern void Responsive_Init(void);

//! \brief     Start the responsive mode
//! \pre       First initialize the responsible mode
//! \param     None
//! \return    None
extern void Responsive_Start(void);

//! \brief     Stop the responsive mode
//! \pre       First initialize the responsible mode
//! \param     None
//! \return    None
extern void Responsive_Stop(void);

//! \brief     Run the responsive mode
//! \pre       First initialize the responsible mode
//! \param     None
//! \return    None
extern void Responsive_Run(void);

#endif // RESPONSIVE_H_
