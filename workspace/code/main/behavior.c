//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    behavior.c
//! \brief   This module provides the useful functions to handle the behavior
//!
//! \author  Vincent Gonet
//!
//! \version $Id: behavior.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "behavior.h"

#include "aseba_esp32.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PROX_IR_SENSOR_NUM      7u  //!< Number of proximity IR sensors
#define GROUND_IR_SENSOR_NUM    2u  //!< Number of ground IR sensors

#define IR_SENSOR_NUM           (PROX_IR_SENSOR_NUM + GROUND_IR_SENSOR_NUM)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_SetIRSensorsLeds(void)
{
  static int16_t max[IR_SENSOR_NUM] = {4000, 4000, 4000, 4000, 4000, 4000, 4000, 900, 900};
  static int16_t min[IR_SENSOR_NUM] = {1200, 1200, 1200, 1200, 1200, 1200, 1200, 0, 0};

  static T_Led led[IR_SENSOR_NUM] = {E_Led_Front_IR_0, E_Led_Front_IR_1, E_Led_Front_IR_2A,
                                     E_Led_Front_IR_3, E_Led_Front_IR_4, E_Led_IR_Back_Left,
                                     E_Led_IR_Back_Right, E_Led_Ground_IR_0, E_Led_Ground_IR_1
                                    };

  int16_t s = 0;
  int16_t delta = 0;
  int16_t brightness = 0;

  for (uint8_t index = 0u; index < PROX_IR_SENSOR_NUM; index++)
  {
    if (max[index] < vmVariables.prox[index])
    {
      max[index] = vmVariables.prox[index];
    }
    else if ((vmVariables.prox[index] != 0) && (min[index] > vmVariables.prox[index]))
    {
      min[index] = vmVariables.prox[index];
    }
  }

  for (uint8_t index = 0u; index < GROUND_IR_SENSOR_NUM; index++)
  {
    if (max[index + PROX_IR_SENSOR_NUM] < vmVariables.ground_delta[index])
    {
      max[index + PROX_IR_SENSOR_NUM] = vmVariables.ground_delta[index];
      // min is fixed to 0 ... this is _physical_
    }
  }

  // Do a linear transformation from min-max to led 0-31!
  for (uint8_t index = 0u; index < PROX_IR_SENSOR_NUM; index++)
  {
    // Because of the min&max calculation above, we cannot have a
    // Division by 0 here.
    s = vmVariables.prox[index] - min[index];
    delta = (max[index] - min[index]);

    if (s < 0)
    {
      s = 0;
    }

    brightness = ((int32_t)s * 32) / delta;
    Leds_SetSingleBrightness(led[index], brightness);

    // The Front IR sensor has 2 LEDs (E_Led_Front_IR_2A and E_Led_Front_IR_2B)
    if (index == 2)
    {
      Leds_SetSingleBrightness(led[index] + 1, brightness);
    }
  }

  for (uint8_t index = 0u; index < GROUND_IR_SENSOR_NUM; index++)
  {
    s = (vmVariables.ground_delta[index] > 0) ? vmVariables.ground_delta[index] : 0;
    brightness = ((int32_t)s * 32) / max[index + PROX_IR_SENSOR_NUM];

    Leds_SetSingleBrightness(led[index + PROX_IR_SENSOR_NUM], brightness);
  }
}

//_____________________________________________________________________________

void xxx_Task(void)
{

}
