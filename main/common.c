//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    common.c
//! \brief   This module provides the common mode functions
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "common.h"

#include "aseba_esp32.h"
#include "buttons.h"
#include "leds.h"
#include "stm32_spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define GROUND_IR_THRESHOLD    280 //130

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const char* Tag = "common";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

uint8_t Common_GetBodyColorPulse(void)
{
  static int16_t pulse = 0;
  int16_t brightness = 0;

  pulse++;

  if (pulse > 0)
  {
    brightness = pulse;

    if (pulse >= (int16_t)MAX_BRIGHTNESS)
    {
      pulse = -((int16_t)MAX_BRIGHTNESS * 4);
    }
  }
  else
  {
    brightness = -pulse / 4;
  }

  return (uint8_t)brightness;
}

//_____________________________________________________________________________

void Common_SetTargetSpeed(int16_t left, int16_t right)
{
  vmVariables.target[0] = left;
  vmVariables.target[1] = right;
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_IncrementTargetSpeed(int16_t left, int16_t right)
{
  vmVariables.target[0] += left;
  vmVariables.target[1] += right;
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_LimitSpeed(int16_t min, int16_t max)
{
  if (vmVariables.target[0] < min)
  {
    vmVariables.target[0] = min;
  }
  else if (vmVariables.target[0] > max)
  {
    vmVariables.target[0] = max;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < min)
  {
    vmVariables.target[1] = min;
  }
  else if (vmVariables.target[1] > max)
  {
    vmVariables.target[1] = max;
  }
  else
  {
    // Do nothing
  }
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_HandlePositiveSpeed(int16_t speed)
{
  int32_t temp1 = 0;
  int32_t temp2 = 0;

  temp1 += (int32_t)vmVariables.prox[0];
  temp1 += (int32_t)(vmVariables.prox[1] * 2);
  temp1 += (int32_t)(vmVariables.prox[2] * 3);
  temp1 += (int32_t)(vmVariables.prox[3] * 2);
  temp1 += (int32_t)vmVariables.prox[4];

  temp2 += (int32_t)(vmVariables.prox[0] * -4);
  temp2 += (int32_t)(vmVariables.prox[1] * -3);
  temp2 += (int32_t)(vmVariables.prox[3] * 3);
  temp2 += (int32_t)(vmVariables.prox[4] * 4);

  //ESP_LOGI(Tag, "speed = %d, temp1 = %d, temp2 = %d", speed, temp1, temp2);

  vmVariables.target[0] = speed - (((temp1 + temp2) * speed) / 200); //2000);
  vmVariables.target[1] = speed - (((temp1 - temp2) * speed) / 200); //2000);

  //ESP_LOGI(Tag, "target = %d %d", vmVariables.target[0], vmVariables.target[1]);
  Common_LimitSpeed(MIN_LIMIT_SPEED, MAX_LIMIT_SPEED);
}

//_____________________________________________________________________________

void Common_HandleNegativeSpeed(int16_t speed)
{
  int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
  vmVariables.target[0] = speed + (temp / -300);

  temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
  vmVariables.target[1] = speed  + (temp / -300);

  Common_LimitSpeed(MIN_LIMIT_SPEED, MAX_LIMIT_SPEED);
}

//_____________________________________________________________________________

void Common_SetSpeedUsingButtons(int16_t* speed, int16_t increment, int16_t max, int16_t min)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    *speed = (*speed + increment);

    if (*speed > max)
    {
      *speed = max;
    }
  }

  when(buttonState[E_Button_Backward])
  {
    *speed = (*speed - increment);

    if (*speed < min)
    {
      *speed = min;
    }
  }
}

//_____________________________________________________________________________

uint8_t Common_HandleTableEdgeDetection(uint8_t red, uint8_t green, uint8_t blue)
{
  if ((vmVariables.ground_delta[0] < GROUND_IR_THRESHOLD) ||
      (vmVariables.ground_delta[1] < GROUND_IR_THRESHOLD))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);

    Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, 0u, 0u);
    Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, 0u, 0u);
    Leds_SetBackLeftBrightness(red, green, blue);
    Leds_SetBackRightBrightness(red, green, blue);
    return 1;
  }
  else
  {
    // Yellow pulse
    Leds_SetBodyBrightness(red, green, blue);
    return 0;
  }
}
