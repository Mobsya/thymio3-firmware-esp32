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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "musician.h"

#include "behavior.h"
#include "codec.h"
#include "color_sensor.h"
#include "common.h"
#include "leds.h"
#include "settings.h"
#include "stm32_spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define INITIAL_SPEED          50
#define MAX_SPEED              500
#define MIN_SPEED              50
#define GROUND_EDGE_OFFSET 100
#define SPEED_INCREMENT         25
#define NEW_COLOR_THR           10 // After 200 ms (behavior run @ 50 hz)

#define C4_FREQ 261 // Do
#define D4_FREQ 293 // Re
#define E4_FREQ 329 // Mi
#define F4_FREQ 349 // Fa
#define G4_FREQ 392 // Sol
#define A4_FREQ 440 // La
#define B4_FREQ 493 // Si

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
int16_t groundThr[2];

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
  Settings_GetGroundBlackSettings(groundThr);
  groundThr[0] += GROUND_EDGE_OFFSET;
  groundThr[1] += GROUND_EDGE_OFFSET;  
  Behavior_Enable(B_LEDS_RGB);
  Codec_Stop();
}

//_____________________________________________________________________________

void Musician_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
  Behavior_Disable(B_LEDS_RGB);
  Leds_SetColorSensorBrightness(0, 0, 0);
  Codec_Stop();
}

//_____________________________________________________________________________

void Musician_Run(void)
{
  static int16_t speed = INITIAL_SPEED;
  static uint8_t colorCount = 0;
  static T_Color colorPrev = E_Color_Unknown;
  static T_Color showColor = E_Color_Unknown;

  T_Color color = ColorSensor_GetColor();
  //T_HSV hsvTemp = ColorSensor_GetHsv();

  if(color != colorPrev) 
  {
    colorCount = 0;
  }
  colorPrev = color;

  // Buttons management
  Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);

  if ((GetGroundValue(0) < groundThr[0]) || (GetGroundValue(1) < groundThr[1]))
  {
    Common_SetTargetSpeed(0, 0);
  }
  else
  {
    Common_SetTargetSpeed(speed, speed);
    /*
    if (speed >= 0)
    {
      Common_HandlePositiveSpeed(speed);
    }
    else
    {
      Common_HandleNegativeSpeed(speed);
    }
    */
  }

  if(color == E_Color_Red)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Red;        
        if(Codec_PlayTone(C4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
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
        if(Codec_PlayTone(D4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
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
        if(Codec_PlayTone(E4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
        Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);  
      }
    } 
    else 
    {
      colorCount++;
    }
  }

  if(color == E_Color_Cyan)
  {
    if(colorCount >= NEW_COLOR_THR)
    {
      if(showColor != color)
      {
        showColor = E_Color_Cyan;  
        if(Codec_PlayTone(F4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
        Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);  
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
        if(Codec_PlayTone(G4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
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
        if(Codec_PlayTone(A4_FREQ, 0) != ESP_OK)
        {
          showColor = E_Color_Unknown; // Just to try play again next time
          ESP_LOGE(Tag, "Error playing tone");

        }
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
        Codec_Stop();
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
        Codec_Stop();
        Leds_SetBodyBrightness(0u, 0u, 0u);
      }
    } 
    else 
    {
      colorCount++;
    }
  }

}
