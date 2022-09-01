//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sensors.h
//! \brief   This module provides the useful functions to acquire sensors values
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SENSORS_H_
#define SENSORS_H_

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

//! \brief     Initialize the sensors
//! \pre       None
//! \param     None
//! \return    None
extern void Sensors_Init(void);

//! \brief     Start the sensors task
//! \pre       First initialize the sensors
//! \param     None
//! \return    None
extern void Sensors_Start(void);

//! \brief     Stop the sensors task
//! \pre       First initialize the sensors
//! \param     None
//! \return    None
extern void Sensors_Stop(void);

extern void Sensors_buttons_pause(void);
extern void Sensors_buttons_resume(void);

#endif // SENSORS_H_
