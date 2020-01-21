//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    musician.c
//! \brief   This module provides the useful functions to use the musician mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "musician.h"

#include "codec.h"
#include "color_sensor.h"
#include "common.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define INITIAL_SPEED          150
#define MAX_SPEED              500
#define MIN_SPEED             -300

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

static const char* Tag = "musician";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Musician_Init(void)
{

}

//_____________________________________________________________________________

void Musician_Start(void)
{

}

//_____________________________________________________________________________

void Musician_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Musician_Run(void)
{
  static int16_t speed = INITIAL_SPEED;

  T_Color color = ColorSensor_GetColor();

  uint8_t brightness = Common_GetBodyColorPulse();

  // White pulse
  Leds_SetBodyBrightness(brightness, brightness, brightness);

  //RunCircleLedRotation();

  // Buttons management
  Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);

  //ESP_LOGE(Tag, "Color: %d", color);

  when (color == E_Color_Red)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Startup);
  }

  if (speed >= 0)
  {
    Common_HandlePositiveSpeed(speed);
  }
  else
  {
    Common_HandleNegativeSpeed(speed);
  }
}
