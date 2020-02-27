//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    comm.h
//! \brief   This module provides the useful functions to communicate
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef COMM_H_
#define COMM_H_

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

//! \brief     Initialize the communication
//! \pre       None
//! \param     None
//! \return    None
extern void Comm_Init(void);

//! \brief     Start the communication task
//! \pre       First initialize the communication
//! \param     None
//! \return    None
extern void Comm_Start(void);

//! \brief     Stop the communication task
//! \pre       First initialize the communication
//! \param     None
//! \return    None
extern void Comm_Stop(void);

#endif // COMM_H_
