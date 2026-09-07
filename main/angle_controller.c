//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    angle_controller.c
//! \brief   This module provides the useful functions to control the angle
//!
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "angle_controller.h"

#include "common.h"
#include "gyroscope.h"
#include "settings.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define KP     8  //!< Proportional factor
#define KD     2  //!< Derivative factor

#define MOT_FW_BW_CAL_SPEED  300  //!< Speed at which the forward/backward factor is calibrated
#define MOT_FW_BW_MAX_BOOST  4    //!< Multiplier applied to the factor at zero speed

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "angle_controller";
static int32_t targetAngle = 0;
static float targetAngleFloat = 0.0;
static int16_t maxSpeed = 500;
static volatile bool rotationInProgress = false;
static int32_t lastError = 0;
int32_t rotation_angle_90_ = 0;
//static float lastErrorFloat = 0.0;
static bool stopPending = false;   // rotation was running on the previous call

// Speed pair applied when the controller output is at its maximum. The default
// is the spin in place, i.e. the behaviour of the module before speed pairs
// were introduced.
static int16_t speedLeft = -500;   //!< Left motor speed at full controller output
static int16_t speedRight = 500;   //!< Right motor speed at full controller output
static int8_t turnSign = 1;        //!< Direction the speed pair turns to: +1 counterclockwise, -1 clockwise

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static bool StartRotation(int32_t targetTicks, int16_t left, int16_t right);
static void ApplyFwBwCorrection(int16_t* left, int16_t* right);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void AngleController_Init(void)
{
  ESP_LOGI(Tag, "Angle controller is initialized");
  rotation_angle_90_ = ROTATION_ANGLE_90 + Settings_GetGyroRotFactorSettings();
}

//_____________________________________________________________________________
#if 0
int16_t AngleController_Update(int16_t target_deg, int16_t maxSpeed)
{
  static int16_t lastError = 0;
  int16_t measure = Gyroscope_GetAngleZ_deg();
  int16_t error = (target_deg - measure);
  int16_t proportional = (KP * error);
  int16_t derivative = KD * (error - lastError);

  int16_t output = proportional + derivative;

  ESP_LOGE(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  lastError = error;

  if (output > maxSpeed)
  {
    output = maxSpeed;
  }
  else if (output < -maxSpeed)
  {
    output = -maxSpeed;
  }

  Common_SetTargetSpeed(-output, output);

  //ESP_LOGI(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  return output;
}
#endif
//_____________________________________________________________________________

void AngleController_Update()
{
  if(!rotationInProgress) {
    if (stopPending)
    {
      // The rotation ended or was aborted: make sure no stale output survives a
      // race with the SPI task, then release the motors.
      stopPending = false;
      Common_SetTargetSpeed(0, 0);
    }
    return;
  }
  stopPending = true;
  
  int32_t measure = Gyroscope_GetAngleZ();
  // The PD loop works in the frame of the speed pair: the error is positive
  // while the robot still has to travel in the direction the pair turns to.
  // This makes a positive output always reduce the error, whichever pair is in
  // use, so the pair itself can be applied exactly as the caller gave it.
  int32_t error = turnSign * ((targetAngle - measure) / 182);
  int32_t proportional = (KP * error);
  int32_t derivative = KD * (error - lastError);

  int16_t output = proportional + derivative;

  ESP_LOGE(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  lastError = error;

  if (output > maxSpeed)
  {
    output = maxSpeed;
  }
  else if (output < -maxSpeed)
  {
    output = -maxSpeed;
  }

  // The controller output is a signed magnitude; the requested speed pair gives
  // it its shape. With (-max, +max) this is equivalent to the former
  // Common_SetTargetSpeed(-output, output), i.e. a spin in place.
  Common_SetTargetSpeed((int16_t)(((int32_t)output * speedLeft) / maxSpeed),
                        (int16_t)(((int32_t)output * speedRight) / maxSpeed));

  //ESP_LOGI(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  if(output == 0) {
    rotationInProgress = false;
  }
}

/*
// Same angle controller but using float instead of integers (for testing purposes).
void AngleController_Update()
{
  if(!rotationInProgress) {
    return;
  }
  float measure = Gyroscope_GetAngleZFloat();
  float error = (targetAngleFloat - measure) / 182;
  float proportional = (KP * error);
  float derivative = KD * (error - lastErrorFloat);

  float output = (proportional + derivative);

  //ESP_LOGE(Tag, "error: %f, measure: %f, output: %d", error, measure, output);

  lastErrorFloat = error;

  if (output > maxSpeed)
  {
    output = maxSpeed;
  }
  else if (output < -maxSpeed)
  {
    output = -maxSpeed;
  }

  if((error < 0.5) && (error > -0.5)) {
    output = 0;
  }

  //Common_SetTargetSpeed(-output, output);
  if(output > 0)
  {
    Common_SetTargetSpeed(-output*1.15, output);
  } else {
    Common_SetTargetSpeed(-output, output*1.15);
  }
  
  //ESP_LOGI(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  if(output == 0) {
    rotationInProgress = false;
  }
}
*/

//_____________________________________________________________________________

//! \brief     Compensate the different behaviour of the motors when running
//!            forward and running backward.
//! \details   Settings_GetMotFwBwSettings() gives the ratio between the two
//!            directions, as measured by the 15 cm calibration: a value of 1.04
//!            means the backward motion is 4% slower than the forward one for
//!            the same command. The mismatch is shared between the two motors,
//!            each one taking a part proportional to its own speed: it is
//!            removed from the motor running forward and added to the one
//!            running backward, so equal speeds get half of it each. Below the
//!            speed at which the factor was calibrated the motors get non
//!            linear and the mismatch grows, so the factor is boosted
//!            accordingly, see MOT_FW_BW_MAX_BOOST. The
//!            resulting pair may be slightly faster than the one requested;
//!            this is accepted, as the ratio between the two speeds, i.e. the
//!            shape of the trajectory, is what matters here.
//! \param     left - Left motor speed, corrected in place
//! \param     right - Right motor speed, corrected in place
static void ApplyFwBwCorrection(int16_t* left, int16_t* right)
{
  float mismatch = 0.0;
  float absLeft = 0.0;
  float absRight = 0.0;
  float backwardSpeed = 0.0;
  float shareLeft = 0.0;
  float shareRight = 0.0;
  float correctedLeft = 0.0;
  float correctedRight = 0.0;

  // The mismatch only shows up when one motor runs forward and the other one
  // backward. A pair running both ways forward, or both ways backward, carries
  // the same error on both sides and has nothing to compensate. A motor stopped
  // on one side is left untouched as well: there is no pair to balance.
  if (((*left >= 0) && (*right >= 0)) || ((*left <= 0) && (*right <= 0)))
  {
    return;
  }

  // Read at every rotation rather than cached, so that a new calibration is
  // taken into account as soon as it is saved.
  mismatch = Settings_GetMotFwBwSettings() - 1.0;

  absLeft = (*left < 0) ? (float)(-(int32_t)(*left)) : (float)(*left);
  absRight = (*right < 0) ? (float)(-(int32_t)(*right)) : (float)(*right);

  // The motor running backward is the one suffering the mismatch, so its own
  // speed sets how much the factor has to be boosted.
  backwardSpeed = (*left < 0) ? absLeft : absRight;

  if (backwardSpeed < (float)MOT_FW_BW_CAL_SPEED)
  {
    // The motors get increasingly non linear below the speed at which the
    // factor was calibrated. The boost grows linearly from 1 at the calibration
    // speed up to MOT_FW_BW_MAX_BOOST at standstill: with 300 and 4 this gives
    // x2 at 200 and x3 at 100. Above the calibration speed the factor is used
    // as it is.
    mismatch = mismatch * ((float)MOT_FW_BW_MAX_BOOST -
                           (((float)(MOT_FW_BW_MAX_BOOST - 1) * backwardSpeed) /
                            (float)MOT_FW_BW_CAL_SPEED));
  }

  shareLeft = (mismatch * absLeft) / (absLeft + absRight);
  shareRight = (mismatch * absRight) / (absLeft + absRight);

  correctedLeft = (*left < 0) ? ((float)(*left) * (1.0 + shareLeft))
                              : ((float)(*left) * (1.0 - shareLeft));
  correctedRight = (*right < 0) ? ((float)(*right) * (1.0 + shareRight))
                                : ((float)(*right) * (1.0 - shareRight));

  *left = (int16_t)((correctedLeft < 0.0) ? (correctedLeft - 0.5) : (correctedLeft + 0.5));
  *right = (int16_t)((correctedRight < 0.0) ? (correctedRight - 0.5) : (correctedRight + 0.5));
}

//_____________________________________________________________________________

//! \brief     Arm the controller for a new rotation.
//! \param     targetTicks - Target angle, in gyroscope ticks
//! \param     left - Left motor speed applied at full controller output
//! \param     right - Right motor speed applied at full controller output
//! \return    true if the rotation was armed, false if the request is invalid
static bool StartRotation(int32_t targetTicks, int16_t left, int16_t right)
{
  int32_t absLeft = 0;
  int32_t absRight = 0;
  int32_t max = 0;
  int32_t currentAngle = 0;
  int32_t rotationNeeded = 0;
  int32_t fullTurn = 0;
  int8_t pairSign = 0;

  if (left == right)
  {
    // Equal speeds produce no angular velocity: the target could never be
    // reached and the controller would spin forever. Do not arm, and make sure
    // any rotation still running is released by AngleController_Update().
    rotationInProgress = false;
    return false;
  }

  // The angular velocity of a differential drive is proportional to
  // (right - left), so its sign alone gives the direction of the turn,
  // independently of whether the robot travels forward or backward.
  pairSign = (right > left) ? 1 : -1;

  currentAngle = Gyroscope_GetAngleZ();
  rotationNeeded = targetTicks - currentAngle;
  fullTurn = rotation_angle_90_ * 4;

  if (((rotationNeeded > 0) && (pairSign < 0)) ||
      ((rotationNeeded < 0) && (pairSign > 0)))
  {
    // The speed pair turns away from the target. The pair is what the caller
    // commanded to the motors, so it wins: the target is moved by whole turns
    // until it lies in the direction the pair turns to. Requesting -90 degrees
    // with a pair that turns counterclockwise makes the robot travel +270
    // degrees instead, reaching the same final heading along the arc the caller
    // asked for.
    // '%' truncates toward zero, so the sign of the remainder is preserved.
    rotationNeeded = rotationNeeded % fullTurn;
    if (rotationNeeded == 0)
    {
      // The request was an exact number of turns: keep one full turn rather
      // than collapsing the whole motion to nothing.
      rotationNeeded = pairSign * fullTurn;
    }
    else
    {
      rotationNeeded += pairSign * fullTurn;
    }
    targetTicks = currentAngle + rotationNeeded;
  }

  // Compensate the forward/backward motor mismatch here, once per rotation, so
  // that AngleController_Update() keeps working on a plain speed pair. The
  // signs are preserved, hence the direction of the turn computed above still
  // holds.
  ApplyFwBwCorrection(&left, &right);

  absLeft = (left < 0) ? -(int32_t)left : (int32_t)left;
  absRight = (right < 0) ? -(int32_t)right : (int32_t)right;
  max = (absLeft > absRight) ? absLeft : absRight;

  targetAngle = targetTicks;
  lastError = 0;
  //lastErrorFloat = 0.0;
  maxSpeed = (int16_t)max;
  speedLeft = left;
  speedRight = right;
  turnSign = pairSign;

  // Written last: the update task only reads the parameters above once this
  // flag is set.
  rotationInProgress = true;
  return true;
}

//_____________________________________________________________________________

bool AngleController_Start(int16_t angleDeg, int16_t max) {
  // Legacy semantics: "max" is a magnitude and the direction is given by the
  // sign of the angle, so the spin-in-place pair is built accordingly.
  // Do NOT replace this with a fixed (-max, +max) pair: StartRotation() would
  // then see every negative angle as turning away from the pair and would send
  // the robot the long way around, e.g. +270 instead of -90.
  if (angleDeg < 0)
  {
    return AngleController_StartWithSpeeds(angleDeg, max, -max);
  }
  return AngleController_StartWithSpeeds(angleDeg, -max, max);
}

//_____________________________________________________________________________

bool AngleController_StartWithSpeeds(int16_t angleDeg, int16_t left, int16_t right) {
  int32_t relativeAngle = 0;

  // Keep the requested rotation within a single turn. The C '%' operator
  // truncates toward zero, so the sign of the request is preserved:
  // 450 -> 90, -450 -> -90, 720 -> 0. Exactly +-360 is left untouched so that
  // a full turn can still be requested explicitly.
  if ((angleDeg > 360) || (angleDeg < -360))
  {
    angleDeg = (int16_t)(angleDeg % 360);
  }

  // Convert to a range that is usable by the angle controller.
  relativeAngle = ((int32_t)angleDeg) * rotation_angle_90_ / 90;

  // The target is relative to the current heading: the gyroscope accumulator is
  // deliberately NOT reset, so the angle reported to the user stays continuous
  // across rotations and any reference set by the user survives.
  // The accumulator is free-running (no wrap-around), which is what allows the
  // error in AngleController_Update() to stay monotonic even for a full turn.
  return StartRotation(Gyroscope_GetAngleZ() + relativeAngle, left, right);
}

//_____________________________________________________________________________

bool AngleController_StartAbsolute(int16_t angleDeg, int16_t max) {
  // The target is absolute, i.e. referred to the origin set by the last
  // Gyroscope_ResetAngle() call. No modulo is applied: an absolute heading may
  // legitimately exceed one turn (a 7-pointed star accumulates 1078 degrees),
  // and folding it would send the robot the wrong way.
  int32_t targetTicks = ((int32_t)angleDeg) * rotation_angle_90_ / 90;

  // Unlike the relative case, the direction cannot be read from the parameters
  // alone: it depends on where the robot currently is.
  if ((targetTicks - Gyroscope_GetAngleZ()) < 0)
  {
    return StartRotation(targetTicks, max, -max);
  }
  return StartRotation(targetTicks, -max, max);
}

//_____________________________________________________________________________

bool AngleController_StartAbsoluteWithSpeeds(int16_t angleDeg, int16_t left, int16_t right) {
  return StartRotation(((int32_t)angleDeg) * rotation_angle_90_ / 90, left, right);
}

//_____________________________________________________________________________

void AngleController_Stop() {
  rotationInProgress = false;
}

//_____________________________________________________________________________

bool AngleController_Completed(void) {
  return !rotationInProgress;
}

//_____________________________________________________________________________

void AngleController_UpdateRotFactor(int16_t factor)
{
  rotation_angle_90_ = ROTATION_ANGLE_90 + factor;
}

//_____________________________________________________________________________

int32_t AngleController_GetRotFactor(void)
{
  return rotation_angle_90_;
}
