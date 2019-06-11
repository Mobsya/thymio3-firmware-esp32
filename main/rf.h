//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    rf.h
//! \brief   This module provides the useful functions to use the xxx
//
//! \author  Vincent Gonet
//
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef RF_H_
#define RF_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

// Module is there ...
#define RF_PRESENT    (1 << 0)

// Module is enable and forward all messages
#define RF_LINK_UP    (1 << 1)

// Module has detected a PC beacon
#define RF_PC_PRESENT   (1 << 2)

// Module has detected a node beacon
#define RF_NEIGHBOR_PRESENT (1 << 3)

// Module has got some data
#define RF_DATA_RX    (1 << 4)

// Module is in pairing mode
#define RF_PAIRING_MODE (1 << 5)

#define RF_DOWN     0x0
#define RF_UP     0x1
// Listen to presence msg only.
#define RF_PRESENCE_ONLY  0x2

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

#endif // RF_H_
