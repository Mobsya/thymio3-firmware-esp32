//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.h
//! \brief   This module provides the useful functions to use the capacitive buttons
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef BUTTONS_H_
#define BUTTONS_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "stdint.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUTTONS_NUM       5u  //!< Number of buttons

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Button_Backward,  // Button 1
  E_Button_Left,      // Button 2
  E_Button_Center,    // Button 3
  E_Button_Forward,   // Button 4
  E_Button_Right      // Button 5
};
typedef uint8_t T_Button;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the internal ADC
//! \pre       None
//! \param     None
//! \return    None
extern void Buttons_Init(void);

//! \brief     Get the button status
//! \pre       None
//! \param     None
//! \return    The status of the buttons
extern uint8_t* Buttons_GetStatus(void);

extern void Buttons_ClearStatus(void);

//! \brief     Update the button status
//! \pre       None
//! \param     None
//! \return    None
extern void Buttons_UpdateStatus(void);

#endif // BUTTONS_H_
