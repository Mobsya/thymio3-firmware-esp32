//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
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

#include "common.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define INITIAL_SPEED          150
#define MAX_SPEED              500
#define MIN_SPEED            (-300)

#define SPEED_INCREMENT         50

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const char* Tag = "explorer";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunCircleLedRotation(void);

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
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Explorer_Run(void)
{
  static int16_t speed = INITIAL_SPEED;

  uint8_t brightness = Common_GetBodyColorPulse();

  RunCircleLedRotation();

  // Buttons management
  Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);

  if (speed >= 0)
  {
    Common_HandlePositiveSpeed(speed);
  }
  else
  {
    Common_HandleNegativeSpeed(speed);
  }

  Common_HandleTableEdgeDetection(brightness, brightness, 0u);
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
