//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.h
//! \brief   This module provides the useful functions to use the buttons
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

#define BUTTONS_NUM       5u  //!< Number of capacitive buttons

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Button_Backward,  // Capacitive button 1
  E_Button_Left,      // Capacitive button 2
  E_Button_Center,    // Capacitive button 3
  E_Button_Forward,   // Capacitive button 4
  E_Button_Right      // Capacitive button 5
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

//! \brief     Initialize the touch buttons and the mechanical buttons
//! \pre       None
//! \param     None
//! \return    None
extern void Buttons_Init(void);

//! \brief     Get the button status
//! \pre       First initialize the buttons
//! \param     None
//! \return    The status of the buttons
extern uint8_t* Buttons_GetStatus(void);

//! \brief     Get the button raw values
//! \pre       First initialize the buttons
//! \param     None
//! \return    The raw values of the buttons
extern uint16_t* Buttons_GetRaw(void);

//! \brief     Get the button filtered values
//! \pre       First initialize the buttons
//! \param     None
//! \return    The filtered values of the buttons
extern uint16_t* Buttons_GetFiltered(void);

//! \brief     Update the button status
//! \pre       First initialize the buttons
//! \param     None
//! \return    None
extern void Buttons_UpdateStatus(void);

#endif // BUTTONS_H_
