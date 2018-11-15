//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    prox_ir.h
//! \brief   This module provides the useful functions to use the proximity IR sensors
//!
//! \author  Vincent Gonet
//!
//! \version $Id: prox_ir.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef PROX_IR_H_
#define PROX_IR_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "stdint.h"

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

//! \brief     Initialize the proximity IR sensors
//! \pre       None
//! \param     None
//! \return    None
extern void ProxIR_Init(void);

//! \brief     Run the proximity IR sensors task
//! \pre       First initialize the proximity IR sensors
//! \param     None
//! \return    None
//extern void GroundIR_Task(void);
extern int16_t ProxIR_EmitPulses(uint16_t tick);

extern void ProxIR_ReadPulseDuration(void);

//! \brief     Enable the network
//! \pre       First initialize the proximity IR sensors
//! \param     None
//! \return    None
extern void ProxIR_EnableNetwork(void);

//! \brief     Disable the network
//! \pre       First initialize the proximity IR sensors
//! \param     None
//! \return    None
extern void ProxIR_DisableNetwork(void);

//! \brief     Shut down the proximity IR sensors
//! \pre       First initialize the proximity IR sensors
//! \param     None
//! \return    None
extern void ProxIR_Shutdown(void);

#endif // PROX_IR_H_
