//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    line_tracker.c
//! \brief   This module provides the useful functions to use the line tracker mode
//!
//! \author  Vincent Gonet, Stefano Morgani
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
#include "settings.h"

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

#define GROUND_THR 150
//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------
//static const char* Tag = "line_tracker";
static int16_t ground_left_black = 300; // Calibrated ground values range is 0..1023, thus 300 is a good threshold to detect black surfaces
static int16_t ground_left_white = 700; // Calibrated ground values range is 0..1023, thus 700 is a good threshold to detect white surfaces
static int16_t ground_right_black = 300;
static int16_t ground_right_white = 700;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void GetLineSensorsState(uint8_t* state);

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
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void LineTracker_Run(void)
{
  static uint8_t state[2] = {STATE_WHITE, STATE_WHITE};
  static int16_t dir = DIR_LOST;
  uint8_t brightness = Common_GetBodyColorPulse();

  // Cyan pulse
  Leds_SetBodyBrightness(0u, brightness, brightness);

  GetLineSensorsState(state);

  GetLineDirection(state, &dir);

  SetTargetAccordingToDirection(&dir);

}

//_____________________________________________________________________________

static void GetLineSensorsState(uint8_t* state)
{
  if (vmVariables.ground_delta[0] < ground_left_black)
  {
    state[0] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[0] > ground_left_white)
  {
    state[0] = STATE_WHITE;
  }

  if (vmVariables.ground_delta[1] < ground_right_black)
  {
    state[1] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[1] > ground_right_white)
  {
    state[1] = STATE_WHITE;
  }
  //ESP_LOGD(Tag, "state: l=%d, r=%d", state[0], state[1]);
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
  static int16_t speed = 0;
  speed = (vmVariables.ground_delta[0] - vmVariables.ground_delta[1])>>3;
  //ESP_LOGD(Tag, "ground: l=%d, r=%d", vmVariables.ground_delta[0], vmVariables.ground_delta[1]);
  if (*direction == DIR_FRONT)
  {
    Common_SetTargetSpeed(SPEED_LINE+speed, SPEED_LINE-speed);
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
    //ESP_LOGD(Tag, "[DIR_FRONT] speed: l=%d, r=%d", SPEED_LINE+speed, SPEED_LINE-speed);
  }
  else if (*direction == DIR_RIGHT)
  {
    Common_SetTargetSpeed(SPEED_LINE, 0);
    Leds_SetCircleBrightness(0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
    //ESP_LOGD(Tag, "[DIR_RIGHT] speed: l=%d, r=%d", SPEED_LINE, 0);
  }
  else if (*direction == DIR_LEFT)
  {
    Common_SetTargetSpeed(0, SPEED_LINE);
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
    //ESP_LOGD(Tag, "[DIR_LEFT] speed: l=%d, r=%d", 0, SPEED_LINE);
  }
  else if (*direction == DIR_L_LEFT)
  {
    Common_SetTargetSpeed(-SPEED_LINE, SPEED_LINE);
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
    //ESP_LOGD(Tag, "[DIR_L_LEFT] speed: l=%d, r=%d", -SPEED_LINE, SPEED_LINE);
  }
  else if (*direction == DIR_L_RIGHT)
  {
    Common_SetTargetSpeed(SPEED_LINE, -SPEED_LINE);
    Leds_SetCircleBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
    //ESP_LOGD(Tag, "[DIR_L_RIGHT] speed: l=%d, r=%d", SPEED_LINE, -SPEED_LINE);
  }
  else if (*direction == DIR_LOST)
  {
    Common_SetTargetSpeed(SPEED_LINE, -SPEED_LINE);
    //leds_set_circle(MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS);
    //ESP_LOGD(Tag, "[DIR_LOST] speed: l=%d, r=%d", SPEED_LINE, -SPEED_LINE);
  }
  else
  {
    // Do nothing
  }
}
