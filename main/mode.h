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
enum
{
  E_Mode_Menu,
  E_Mode_Sequence,  
  E_Mode_Friendly,
  E_Mode_Explorer,
  E_Mode_Fearful,
  E_Mode_Attentive,
  E_Mode_Investigator,
  E_Mode_Obedient,  
  E_Mode_Painter,
  E_Mode_Musician,
  E_Mode_NN,
  E_Mode_Max = E_Mode_NN
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

//! \brief     Get the current running mode
//! \pre       First initialize the modes
//! \param     None
//! \return    T_Mode
extern T_Mode Mode_get_current(void);

extern void enter_micropython_mode(void);

extern void exit_micropython_mode(void);

#endif // MODE_H_
