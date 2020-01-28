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

  //uint8_t brightness = Common_GetBodyColorPulse();

  // White pulse
  //Leds_SetBodyBrightness(brightness, brightness, brightness);

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

  when (color == E_Color_Red)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_C3);
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
  }

  when (color == E_Color_Orange)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_D3);
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS / 2u, 0u);
  }

  when (color == E_Color_Yellow)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_E3);
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
  }

  when (color == E_Color_Green)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_F3);
    Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);
  }

  when (color == E_Color_Cyan)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_G3);
    Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }

  when (color == E_Color_Blue)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_A3);
    Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
  }

  when (color == E_Color_Purple)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_B3);
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
  }

  when (color == E_Color_White)
  {
    //Codec_PlayMP3FileFromFlash(E_SystemSound_Startup);
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }

  when (color == E_Color_Unknown)
  {
    //Codec_PlayMP3FileFromFlash(E_SystemSound_Startup);
    Leds_SetBodyBrightness(0u, 0u, 0u);
  }
}
