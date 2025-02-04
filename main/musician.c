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

#define INITIAL_SPEED          100
#define MAX_SPEED              500
#define MIN_SPEED              100

#define SPEED_INCREMENT         50
#define NEW_COLOR_THR           10 // After 200 ms (behavior run @ 50 hz)
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
  static uint8_t colorCount = 0;
  static T_Color colorPrev = E_Color_Unknown;
  static T_Color showColor = E_Color_Unknown;

  T_Color color = ColorSensor_GetColor();
  T_HSV hsvTemp = ColorSensor_GetHsv();

  if(color != colorPrev) 
  {
    colorCount = 0;
  }
  colorPrev = color;

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

  if(color == E_Color_Red)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Red;        
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_C3);
        Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);        
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_Yellow)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Yellow;        
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_D3);
        Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);    
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_Green)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Green;        
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_E3);
        Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);  
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_Blue)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Blue;        
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_F3);
        Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_Purple)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Purple;        
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_G3);
        Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_White)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_White;
        Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if((color == E_Color_Black) || (color == E_Color_Unknown))
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Unknown;
        Leds_SetBodyBrightness(0u, 0u, 0u);
      }
    } 
    else 
    {
      colorCount++;
    }
  }

}
