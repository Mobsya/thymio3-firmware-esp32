//_____________________________________________________________________________
//
// Copyright (C) 2026                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    straight_controller.h
//! \brief   This module holds the heading of the robot during a straight motion
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef STRAIGHT_CONTROLLER_H_
#define STRAIGHT_CONTROLLER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Start holding the current heading while moving straight
//! \details   Counterpart of the angle controller: that one turns the robot to a
//!            heading, this one keeps the robot on a heading while it travels.
//!            The controller only drives the motors while the timed motion
//!            started by the caller is running, see Behavior_IsMotionRunning(),
//!            and releases itself as soon as the motion timer expires, leaving
//!            the motors as the timer ISR set them.
//! \pre       The caller has already started the timed motion
//! \param     baseSpeed - Base speed of both motors, negative to go backward
//! \return    None
void StraightController_Start(int16_t baseSpeed);

//! \brief     Start holding an absolute heading while moving straight
//! \details   The heading is referred to the origin set by the last
//!            Gyroscope_ResetAngle() call, i.e. the same reference used by
//!            AngleController_StartAbsolute(). Holding the absolute heading
//!            makes the straight motion itself recover the drift accumulated by
//!            the previous steps, instead of leaving it to the next rotation.
//! \pre       The caller has already started the timed motion
//! \param     headingDeg - Heading to hold, in degrees
//! \param     baseSpeed - Base speed of both motors, negative to go backward
//! \return    None
void StraightController_StartAbsolute(int16_t headingDeg, int16_t baseSpeed);

//! \brief     Stop the controller without touching the motors
//! \pre       None
//! \param     None
//! \return    None
void StraightController_Stop(void);

//! \brief     Check if the controller is driving the motors
//! \pre       None
//! \param     None
//! \return    true while the controller is running
bool StraightController_IsRunning(void);

//! \brief     Run one iteration of the controller
//! \details   Called by the behavior task at 50 Hz, right after
//!            AngleController_Update(). Does nothing when no straight motion is
//!            running.
//! \pre       None
//! \param     None
//! \return    None
void StraightController_Update(void);

#endif // STRAIGHT_CONTROLLER_H_