//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    rc5.h
//! \brief   This module provides the useful functions to use the RC5 receiver
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef RC5_H_
#define RC5_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdbool.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Command_0          = 0,
  E_Command_1          = 1,
  E_Command_2          = 2,
  E_Command_3          = 3,
  E_Command_4          = 4,
  E_Command_5          = 5,
  E_Command_6          = 6,
  E_Command_7          = 7,
  E_Command_8          = 8,
  E_Command_9          = 9,
  E_Command_Plus       = 16,
  E_Command_Minus      = 17,
  E_Command_Go         = 53,
  E_Command_UpArrow    = 80,
  E_Command_DownArrow  = 81,
  E_Command_LeftArrow  = 85,
  E_Command_RightArrow = 86,
  E_Command_Stop       = 87
} typedef T_Command;  //!< The command received

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the RC5 driver
//! \pre       None
//! \param     None
//! \return    None
extern void RC5_Init(void);

//! \brief     Start the RC5 task
//! \pre       First initialize the RC5 driver
//! \param     None
//! \return    None
extern void RC5_Start(void);

//! \brief     Stop the RC5 task
//! \pre       First initialize the RC5 driver
//! \param     None
//! \return    None
extern void RC5_Stop(void);

//! \brief     Get the last received command
//! \pre       First initialize the RC5 driver
//! \param     None
//! \return    The last received command
extern int16_t RC5_GetCommand(void);

//! \brief     Has a new message been received
//! \pre       First initialize the RC5 driver
//! \param     last - Last toggle bit received
//! \return    True if a new message has been received, false otherwise
extern bool RC5_IsNewMessageReceived(int16_t* last);

//! \brief     Is the frame valid
//! \pre       First initialize the RC5 driver
//! \param     None
//! \return    True if the frame is valid, false otherwise
extern bool RC5_IsFrameValid(void);

//! \brief     Clear the frame validity
//! \pre       First initialize the RC5 driver
//! \param     None
//! \return    None
extern void RC5_ClearFrameValidity(void);

#endif // RC5_RECEIVER_H_
