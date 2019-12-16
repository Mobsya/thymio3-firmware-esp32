//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    explorer.c
//! \brief   This module provides the useful functions to use the explorer mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "explorer.h"

#include "aseba_esp32.h"
#include "buttons.h"
#include "common.h"
#include "leds.h"

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
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "explorer";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunCircleLedRotation(void);

static void SetSpeedUsingButtons(int16_t* speed);

static void HandlePositiveSpeed(int16_t speed);

static void HandleNegativeSpeed(int16_t speed);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Explorer_Init(void)
{

}

//_____________________________________________________________________________

void Explorer_Start(void)
{

}

//_____________________________________________________________________________

void Explorer_Stop(void)
{
  vmVariables.target[0] = 0;
  vmVariables.target[1] = 0;
}

//_____________________________________________________________________________

void Explorer_Run(void)
{
  static int16_t speed = 150;

  uint8_t brightness = Common_GetBodyColorPulse();

  // Yellow pulse
  Leds_SetBodyBrightness(brightness, brightness, 0u);

  RunCircleLedRotation();

  // Buttons management
  SetSpeedUsingButtons(&speed);

  if (speed >= 0)
  {
    HandlePositiveSpeed(speed);
  }
  else
  {
    HandleNegativeSpeed(speed);
  }

  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
// FIXME    vmVariables.target[0] = 0;
// FIXME    vmVariables.target[1] = 0;
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, MAX_BRIGHTNESS);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, MAX_BRIGHTNESS);
  }
  else
  {
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0u);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0u);
  }
}

//_____________________________________________________________________________

static void RunCircleLedRotation(void)
{
  static uint8_t led_state = 0u;
  uint8_t l[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint8_t fixed;

  led_state += 2u;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7u] = MAX_BRIGHTNESS;
  l[(fixed - 1u) & 0x7u] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1u)));
  l[(fixed + 1u) & 0x7u] = (led_state & (MAX_BRIGHTNESS - 1u));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}

//_____________________________________________________________________________

static void SetSpeedUsingButtons(int16_t* speed)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    *speed = (*speed + 50);

    if (*speed > 500)
    {
      *speed = 500;
    }
  }

  when(buttonState[E_Button_Backward])
  {
    *speed = (*speed - 50);

    if (*speed < -300)
    {
      *speed = -300;
    }
  }
}

//_____________________________________________________________________________

static void HandlePositiveSpeed(int16_t speed)
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
  Common_LimitSpeed(MIN_SPEED, MAX_SPEED);
}

//_____________________________________________________________________________

static void HandleNegativeSpeed(int16_t speed)
{
  int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
  vmVariables.target[0] = speed + (temp / -300);

  temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
  vmVariables.target[1] = speed  + (temp / -300);

  Common_LimitSpeed(MIN_SPEED, MAX_SPEED);
}
