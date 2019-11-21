//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    drawer.h
//! \brief   This module provides the useful functions to use the drawer mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef DRAWER_H_
#define DRAWER_H_

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

//! \brief     Initialize the drawer mode
//! \pre       None
//! \param     None
//! \return    None
extern void Drawer_Init(void);

//! \brief     Start the drawer mode
//! \pre       First initialize the drawer mode
//! \param     None
//! \return    None
extern void Drawer_Start(void);

//! \brief     Stop the drawer mode
//! \pre       First initialize the drawer mode
//! \param     None
//! \return    None
extern void Drawer_Stop(void);

//! \brief     Run the drawer mode
//! \pre       First initialize the drawer mode
//! \param     None
//! \return    None
extern void Drawer_Run(void);

#endif // DRAWER_H_
