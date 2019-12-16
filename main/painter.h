//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    painter.h
//! \brief   This module provides the useful functions to use the painter mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef PAINTER_H_
#define PAINTER_H_

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

//! \brief     Initialize the painter mode
//! \pre       None
//! \param     None
//! \return    None
extern void Painter_Init(void);

//! \brief     Start the painter mode
//! \pre       First initialize the painter mode
//! \param     None
//! \return    None
extern void Painter_Start(void);

//! \brief     Stop the painter mode
//! \pre       First initialize the painter mode
//! \param     None
//! \return    None
extern void Painter_Stop(void);

//! \brief     Run the painter mode
//! \pre       First initialize the painter mode
//! \param     None
//! \return    None
extern void Painter_Run(void);

#endif // PAINTER_H_
