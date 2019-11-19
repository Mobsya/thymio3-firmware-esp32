//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    line_tracker.c
//! \brief   This module provides the useful functions to use the line tracker mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

#include "esp_log.h"

#include "line_tracker.h"

#include "aseba_esp32.h"
#include "buttons.h"
#include "common.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SPEED_LINE    300

#define STATE_BLACK     0
#define STATE_WHITE     1

#define DIR_LEFT     (-1)
#define DIR_L_LEFT   (-2)
#define DIR_RIGHT     (1)
#define DIR_L_RIGHT   (2)
#define DIR_LOST     (10)
#define DIR_FRONT     (0)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "line_tracker";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static bool CalibrateLevelUsingButtons(uint16_t* blackLevel, uint16_t* whiteLevel);

static void GetLineSensorsState(uint16_t* blackLevel, uint16_t* whiteLevel, uint8_t* state);

static void GetLineDirection(uint8_t* state, int16_t* direction);

static void SetTargetAccordingToDirection(int16_t* direction);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void LineTracker_Init(void)
{

}

//_____________________________________________________________________________

void LineTracker_Start(void)
{

}

//_____________________________________________________________________________

void LineTracker_Stop(void)
{
  vmVariables.target[0] = 0;
  vmVariables.target[1] = 0;
}

//_____________________________________________________________________________

void LineTracker_Run(void)
{
  static uint8_t state[2] = {STATE_WHITE, STATE_WHITE};
  static int16_t dir = DIR_LOST;
  static uint16_t bs_black_level = 650; //400;
  static uint16_t bs_white_level = 700; //450;

  uint8_t brightness = Common_GetBodyColorPulse();

  // Cyan pulse
  Leds_SetBodyBrightness(0u, brightness, brightness);

#if 0
  if (!CalibrateLevelUsingButtons(&bs_black_level, &bs_white_level))
  {
    // Calibration is not in progress

    GetLineSensorsState(&bs_black_level, &bs_white_level, state);
#if 0
    T_Color color = ColorSensor_GetColor();

    switch (color)
    {
      case E_Color_Red:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, 0u);
        break;

      case E_Color_Orange:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 20u, 0u);
        break;

      case E_Color_Yellow:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
        break;

      case E_Color_Green:
        Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, 0u);
        break;

      case E_Color_Cyan:
        Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Blue:
        Leds_SetTopBrightness(0u, 0u, MAX_BRIGHTNESS);
        break;

      case E_Color_Purple:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
        break;

      case E_Color_White:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Unknown:
        Leds_SetTopBrightness(0u, 0u, 0u);
        break;

      default:
        Leds_SetTopBrightness(0u, 0u, 0u);
        break;
    }
#endif

    GetLineDirection(state, &dir);

    SetTargetAccordingToDirection(&dir);
  }
#endif
}

//_____________________________________________________________________________

static bool CalibrateLevelUsingButtons(uint16_t* blackLevel, uint16_t* whiteLevel)
{
  uint8_t* buttonState;
  bool calibrationIsInProgress = false;

  buttonState = Buttons_GetStatus();

  //ESP_LOGI(Tag, "left = %d, right = %d", vmVariables.ground_delta[0], vmVariables.ground_delta[1]);

  // Calibration feature
  if (buttonState[E_Button_Backward] && buttonState[E_Button_Forward])
  {
    *blackLevel = (vmVariables.ground_delta[0] + vmVariables.ground_delta[1]) / 2;
    *blackLevel += 150u;
    calibrationIsInProgress = true;
  }

  if (buttonState[E_Button_Left] && buttonState[E_Button_Right])
  {
    *whiteLevel = (vmVariables.ground_delta[0] + vmVariables.ground_delta[1]) / 2;

    if (*whiteLevel < 150u)
    {
      *whiteLevel = 200u;
    }

    *whiteLevel -= 150u;
    calibrationIsInProgress = true;
  }

  // if the user is trying to calibrate, then don't try to move
  if (calibrationIsInProgress)
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
  }

  return calibrationIsInProgress;
}

//_____________________________________________________________________________

static void GetLineSensorsState(uint16_t* blackLevel, uint16_t* whiteLevel, uint8_t* state)
{
  if (vmVariables.ground_delta[0] < *blackLevel)
  {
    state[0] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[0] > *whiteLevel)
  {
    state[0] = STATE_WHITE;
  }

  if (vmVariables.ground_delta[1] < *blackLevel)
  {
    state[1] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[1] > *whiteLevel)
  {
    state[1] = STATE_WHITE;
  }
}

//_____________________________________________________________________________

static void GetLineDirection(uint8_t* state, int16_t* direction)
{
  if ((state[0] == STATE_BLACK) && (state[1] == STATE_BLACK))
  {
    // Black line right under us
    *direction = DIR_FRONT;
  }
  else if ((state[0] == STATE_WHITE) && (state[1] == STATE_BLACK))
  {
    *direction = DIR_RIGHT;
  }
  else if ((state[1] == STATE_WHITE) && (state[0] == STATE_BLACK))
  {
    *direction = DIR_LEFT;
  }
  else
  {
    // Lost the line
    if (*direction > 0)
    {
      *direction = DIR_L_RIGHT;
    }
    else if (*direction < 0)
    {
      *direction = DIR_L_LEFT;
    }
    else
    {
      *direction = DIR_LOST;
    }
  }
}

//_____________________________________________________________________________

static void SetTargetAccordingToDirection(int16_t* direction)
{
  if (*direction == DIR_FRONT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }
  else if (*direction == DIR_RIGHT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = 0;
    Leds_SetCircleBrightness(0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (*direction == DIR_LEFT)
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
  }
  else if (*direction == DIR_L_LEFT)
  {
    vmVariables.target[0] = -SPEED_LINE;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
  }
  else if (*direction == DIR_L_RIGHT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = -SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
  }
  else if (*direction == DIR_LOST)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = -SPEED_LINE;
    //leds_set_circle(MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS);
  }
  else
  {
    // Do nothing
  }
}
