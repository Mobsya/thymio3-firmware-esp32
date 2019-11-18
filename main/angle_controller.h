//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    angle_controller.h
//! \brief   This module provides the useful functions to control the angle
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ANGLE_CONTROLLER_H_
#define ANGLE_CONTROLLER_H_

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
extern void AngleController_Init(void);

//! \brief     Update the angle
//! \pre       First initialize the angle controller
//! \param     target - Target of the angle on the Z-axis
//! \return    None
extern int16_t AngleController_Update(int16_t target, int16_t maxSpeed);

#endif // ANGLE_CONTROLLER_H_
