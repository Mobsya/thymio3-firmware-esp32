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

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define KP     8  //!< Proportional factor
#define KD     2  //!< Derivative factor

#define ROTATION_ANGLE_90 16383  //!< Rotation angle corresponding to 90 degrees (0x3FFF)

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
static int16_t maxSpeed = 500;
static bool rotationInProgress = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void AngleController_Init(void)
{
  ESP_LOGI(Tag, "Angle controller is initialized");
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
    return;
  }
  static int16_t lastError = 0;
  int16_t measure = Gyroscope_GetAngleZ();
  int16_t error = (targetAngle - measure) / 182;
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

  if(output == 0) {
    rotationInProgress = false;
  }
}

//_____________________________________________________________________________

void AngleController_Start(int16_t angleDeg, int16_t max) {
  Gyroscope_ResetAngle();
  targetAngle = ((int32_t)angleDeg)*ROTATION_ANGLE_90/90; // Convert to a range that is usable by the angle controller.
  maxSpeed = max;
  rotationInProgress = true;
}

//_____________________________________________________________________________

void AngleController_Stop() {
  Common_SetTargetSpeed(0, 0);
  rotationInProgress = false;
}

//_____________________________________________________________________________

bool AngleController_Completed(void) {
  return !rotationInProgress;
}
