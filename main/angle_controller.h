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
#define ROTATION_ANGLE_90 16384  //!< Rotation angle corresponding to 90 degrees

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------
extern int32_t rotation_angle_90_;

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
//! \param     angleDeg - Target of the angle on the Z-axis, this is relative to the current position. Positive values for counterclockwise rotations, negative values for clockwise rotations.
//! \param     max - Maximum speed used by the controller
//! \return    None
bool AngleController_Start(int16_t angleDeg, int16_t max);

//! \brief     Start the angle controller towards an absolute angle
//! \pre       First initialize the angle controller
//! \param     angleDeg - Target angle on the Z-axis, absolute, i.e. referred to
//!                       the origin set by the last Gyroscope_ResetAngle() call.
//!                       It may exceed +-360 degrees. Positive values for
//!                       counterclockwise rotations, negative values for
//!                       clockwise rotations.
//! \param     max - Maximum speed used by the controller
//! \return    None
bool AngleController_StartAbsolute(int16_t angleDeg, int16_t max);

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

//! \brief     Update the rotation factor
//! \pre       First initialize the angle controller
//! \param     None
//! \return    None
void AngleController_UpdateRotFactor(int16_t factor);

//! \brief     Get the rotation factor
//! \pre       First initialize the angle controller
//! \param     None
//! \return    Rotation factor
int32_t AngleController_GetRotFactor(void);

//! \brief     Start a relative rotation using an arbitrary speed pair.
//! \param     angleDeg - Rotation to perform, in degrees, relative to the current heading
//! \param     left - Left motor speed applied at full controller output
//! \param     right - Right motor speed applied at full controller output
//! \return    true if the rotation was armed, false if the request is invalid
bool AngleController_StartWithSpeeds(int16_t angleDeg, int16_t left, int16_t right);

//! \brief     Start an absolute rotation using an arbitrary speed pair.
//! \param     angleDeg - Target heading, in degrees, relative to the last angle reset
//! \param     left - Left motor speed applied at full controller output
//! \param     right - Right motor speed applied at full controller output
//! \return    true if the rotation was armed, false if the request is invalid
bool AngleController_StartAbsoluteWithSpeeds(int16_t angleDeg, int16_t left, int16_t right);

#endif // ANGLE_CONTROLLER_H_
