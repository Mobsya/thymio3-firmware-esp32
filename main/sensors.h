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

//! \brief     Initialize the communication
//! \pre       None
//! \param     None
//! \return    None
extern void Sensors_Init(void);

//! \brief     Start the communication task
//! \pre       First initialize the communication
//! \param     None
//! \return    None
extern void Sensors_Start(void);

//! \brief     Check that the I2C bus is available
//! \pre       First initialize the communication
//! \param     None
//! \return    True if the I2C bus is available, false otherwise
extern bool Sensors_IsBusAvailable(void);

#endif // SENSORS_H_
