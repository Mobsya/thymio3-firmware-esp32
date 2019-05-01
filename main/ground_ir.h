//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ground_ir.h
//! \brief   This module provides the useful functions to use the Ground IR sensors
//!
//! \author  Vincent Gonet
//!
//! \version $Id: ground_ir.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef GROUND_IR_H_
#define GROUND_IR_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "stdint.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define GROUND_IR_SENSORS_NUM    2u  //!< Number of ground IR sensors

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

//! \brief     Initialize the ground IR sensors
//! \pre       None
//! \param     None
//! \return    None
extern void GroundIR_Init(void);

//! \brief     Run the ground IR sensors task
//! \pre       First initialize the Ground IR sensors
//! \param     tick - Tick counter
//! \param     left - ADC value read on IR_SENSE_GROUND_LEFT_PIN
//! \param     right - ADC value read on IR_SENSE_GROUND_RIGHT_PIN
//! \return    None
extern void GroundIR_EmitPulses(uint16_t tick, uint16_t left, uint16_t right);

//! \brief     Shut down the ground IR sensors
//! \pre       First initialize the Ground IR sensors
//! \param     None
//! \return    None
extern void GroundIR_Shutdown(void);

#endif // GROUND_IR_H_
