//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    mode.h
//! \brief   This module provides the useful functions to use the modes
//!
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef MODE_H_
#define MODE_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define RUNNING_MENU 0
#define RUNNING_BEHAVIOR 1

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

//! \brief     Initialize the modes
//! \pre       None
//! \param     enableVM - Flag used to enable/disable the VM
//! \return    None
extern void Mode_Init(bool enableVM);

//! \brief     Initialize the VM
//! \pre       None
//! \param     None
//! \return    None
extern void Mode_InitVM(void);

//! \brief     Run the modes
//! \pre       First initialize the modes
//! \param     None
//! \return    None
extern void Mode_Run(void);

extern void enter_micropython_mode(void);

extern void exit_micropython_mode(void);

#endif // MODE_H_
