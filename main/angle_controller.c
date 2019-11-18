//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    angle_controller.c
//! \brief   This module provides the useful functions to control the angle
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "angle_controller.h"

#include "aseba_esp32.h"
#include "gyroscope.h"

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

  vmVariables.target[0] = -output;
  vmVariables.target[1] = output;

  //ESP_LOGI(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  return output;
}
#endif
//_____________________________________________________________________________

int16_t AngleController_Update(int16_t target, int16_t maxSpeed)
{
  static int16_t lastError = 0;
  int16_t measure = Gyroscope_GetAngleZ();
  int16_t error = (target - measure) / 182;
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

  vmVariables.target[0] = -output;
  vmVariables.target[1] = output;

  //ESP_LOGI(Tag, "error: %d, measure: %d, output: %d", error, measure, output);

  return output;
}
