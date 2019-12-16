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
//! \author  Vincent Gonet
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

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Mode_Menu,
  E_Mode_Friendly,
  E_Mode_Explorer,
  E_Mode_Fearful,
  E_Mode_Painter,
  E_Mode_LineTracker,
  E_Mode_Responsive,
  E_Mode_Musician,
  E_Mode_Max = E_Mode_Musician
};
typedef int16_t T_Mode;  // Mode selection

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
//! \param     None
//! \return    None
extern void Mode_Init(bool enableVM);

extern void Mode_InitVM(void);

extern void Mode_Run(void);

#endif // MODE_H_
