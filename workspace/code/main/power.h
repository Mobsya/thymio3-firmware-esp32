//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: power.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef POWER_H_
#define POWER_H_

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

//! \brief     Switch off the ESP32 and its peripherals
//! \pre       First initialize the power
//! \param     None
//! \return    None
extern void Power_SwitchOff(void);

#endif // POWER_H_
