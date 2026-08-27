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
static int32_t rotation_angle_90_ = 0;
//static float lastErrorFloat = 0.0;
static bool stopPending = false;   // rotation was running on the previous call

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void StartRotation(int32_t targetTicks, int16_t max);

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
  int32_t error = (targetAngle - measure) / 182;
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

  Common_SetTargetSpeed(-output, output);
  //if(output > 0)
  //{
  //  Common_SetTargetSpeed(-output*1.15, output);
  //} else {
  //  Common_SetTargetSpeed(-output, output*1.15);
  //}
  
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

//! \brief     Arm the controller for a new rotation.
//! \param     targetTicks - Target angle, in gyroscope ticks
//! \param     max - Maximum speed used by the controller
//! \return    None
static void StartRotation(int32_t targetTicks, int16_t max)
{
  targetAngle = targetTicks;
  lastError = 0;
  //lastErrorFloat = 0.0;
  maxSpeed = max;

  // Written last: the update task only reads the parameters above once this
  // flag is set.
  rotationInProgress = true;
}

//_____________________________________________________________________________

void AngleController_Start(int16_t angleDeg, int16_t max) {
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
  StartRotation(Gyroscope_GetAngleZ() + relativeAngle, max);
}

//_____________________________________________________________________________

void AngleController_StartAbsolute(int16_t angleDeg, int16_t max) {
  // The target is absolute, i.e. referred to the origin set by the last
  // Gyroscope_ResetAngle() call. No modulo is applied: an absolute heading may
  // legitimately exceed one turn (a 7-pointed star accumulates 1078 degrees),
  // and folding it would send the robot the wrong way.
  StartRotation(((int32_t)angleDeg) * rotation_angle_90_ / 90, max);
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
