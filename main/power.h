//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    power.h
//! \brief   This module provides the useful functions to manage the power
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef POWER_H_
#define POWER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

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

//! \brief     Initialize the power
//! \pre       None
//! \param     None
//! \return    None
extern void Power_Init(void);

//! \brief     Enable the power VA
//! \pre       First initialize the power
//! \param     None
//! \return    None
extern void Power_EnableVA(void);

//! \brief     Disable the power VA
//! \pre       First initialize the power
//! \param     None
//! \return    None
extern void Power_DisableVA(void);

//! \brief     Handle the power mode request
//! \pre       First initialize the power
//! \param     None
//! \return    None
extern void Power_HandlePowerModeRequest(void);

#endif // POWER_H_
