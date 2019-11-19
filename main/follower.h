//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    follower.h
//! \brief   This module provides the useful functions to use the follower mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef FOLLOWER_H_
#define FOLLOWER_H_

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

//! \brief     Initialize the follower mode
//! \pre       None
//! \param     None
//! \return    None
extern void Follower_Init(void);

//! \brief     Start the follower mode
//! \pre       First initialize the explorer mode
//! \param     None
//! \return    None
extern void Follower_Start(void);

//! \brief     Stop the follower mode
//! \pre       First initialize the explorer mode
//! \param     None
//! \return    None
extern void Follower_Stop(void);

//! \brief     Run the follower mode
//! \pre       First initialize the explorer mode
//! \param     None
//! \return    None
extern void Follower_Run(void);

#endif // FOLLOWER_H_
