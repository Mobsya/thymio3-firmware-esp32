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
static float targetAngleFloat = 0.0;
static int16_t maxSpeed = 500;
static bool rotationInProgress = false;
static int16_t lastError = 0;
//static float lastErrorFloat = 0.0;

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

void AngleController_Start(int16_t angleDeg, int16_t max) {
  Gyroscope_ResetAngle();
  lastError = 0;
  //lastErrorFloat = 0.0;
  targetAngle = ((int32_t)angleDeg)*ROTATION_ANGLE_90/90; // Convert to a range that is usable by the angle controller.
  //targetAngleFloat = ((float)angleDeg)*(ROTATION_ANGLE_90*1.0)/90.0; // Instead of dividing by 2 "AngleFloat" we can multiply by 2 this value when using 250 dps for the gyro?
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
