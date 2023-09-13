//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    angle_controller.h
//! \brief   This module provides the useful functions to control the angle
//!
//! \author  Vincent Gonet, Stefano Morgani
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

//! \brief     Activate the angle controller
//! \pre       First initialize the angle controller
//! \param     target - Target of the angle on the Z-axis
//! \return    None
extern void AngleController_Update();

//! \brief     Start the angle controller with the given parameters
//! \pre       First initialize the angle controller
//! \param     angleDeg - Target of the angle on the Z-axis
//! \param     max - Maximum speed used by the controller
//! \return    None
void AngleController_Start(int16_t angleDeg, int16_t max);

//! \brief     Stop the angle controller
//! \pre       First initialize the angle controller
//! \param     None
//! \return    None
void AngleController_Stop(void);

//! \brief     Check if angle controller reached target angle
//! \pre       First initialize the angle controller
//! \param     None
//! \return    true if target angle reached
bool AngleController_Completed(void);

#endif // ANGLE_CONTROLLER_H_
