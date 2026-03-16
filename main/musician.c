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
#include "buttons.h"

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

#define PLAY_TONE 0
#define PLAY_PIANO 1
#define PLAY_FLUTE 2
#define PLAY_ORCHESTRE 3
#define PLAY_GUITARE 4
#define PLAY_VIOLON 5
#define PLAY_BALAFON 6
#define NUM_INSTRUMENTS 7

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
static int8_t current_instrument = PLAY_TONE;
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
  current_instrument = PLAY_TONE;
  Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS);
}

//_____________________________________________________________________________

void Musician_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
  Behavior_Disable(B_LEDS_RGB);
  Leds_SetColorSensorBrightness(0, 0, 0);
  Codec_Stop();
  Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0);
}

//_____________________________________________________________________________

void Musician_update_leds(void)
{
  switch(current_instrument)
  {
    case PLAY_TONE:
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS);
      break;
    case PLAY_PIANO:
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
      break;
    case PLAY_FLUTE:
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u);
      break;
    case PLAY_ORCHESTRE:
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
      break;
    case PLAY_GUITARE:
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
      break;
    case PLAY_VIOLON:
      Leds_SetLegoFrontBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
      break;
    case PLAY_BALAFON:
      Leds_SetLegoFrontBrightness(0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u);
      break;
  }
}

void Musician_Run(void)
{
  static int16_t speed = INITIAL_SPEED;
  static uint8_t colorCount = 0;
  static T_Color colorPrev = E_Color_Unknown;
  static T_Color showColor = E_Color_Unknown;
  uint8_t* buttonState;

  T_Color color = ColorSensor_GetColor();
  //T_HSV hsvTemp = ColorSensor_GetHsv();

  if(color != colorPrev) 
  {
    colorCount = 0;
  }
  colorPrev = color;

  // Buttons management
  Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);
  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Right])
  {
    current_instrument++;
    if(current_instrument >= NUM_INSTRUMENTS)
    {
      current_instrument = PLAY_TONE;
    }
    Musician_update_leds();
  }
  when(buttonState[E_Button_Left])
  {
    current_instrument--;
    if(current_instrument < 0)
    {
      current_instrument = (NUM_INSTRUMENTS-1);
    }
    Musician_update_leds();
  }

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
        switch(current_instrument)
        {
          case PLAY_TONE:
            if(Codec_PlayTone(C4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_00) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
        switch(current_instrument)
        {
          case PLAY_TONE:            
            if(Codec_PlayTone(D4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_01) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
        switch(current_instrument)
        {
          case PLAY_TONE:
            if(Codec_PlayTone(E4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_02) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
        switch(current_instrument)
        {
          case PLAY_TONE:
            if(Codec_PlayTone(F4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_03) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
        switch(current_instrument)
        {
          case PLAY_TONE:
            if(Codec_PlayTone(G4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_04) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
        switch(current_instrument)
        {
          case PLAY_TONE:
            if(Codec_PlayTone(A4_FREQ, 0) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing tone");

            }
            break;
          case PLAY_BALAFON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_BALAFON_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing bafalon");

            }
            break;
          case PLAY_FLUTE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_FLUTE_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing flute");

            }
            break;
          case PLAY_GUITARE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_GUITARE_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing guitare");

            }
            break;
          case PLAY_ORCHESTRE:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_ORCHESTRE_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing orchestre");

            }
            break;
          case PLAY_PIANO:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_PIANO_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing piano");

            }
            break;
          case PLAY_VIOLON:
            Codec_Stop();
            if(Codec_PlayOnboardSound(TONE_TYPE_INSTRU_VIOLON_05) != ESP_OK)
            {
              showColor = E_Color_Unknown; // Just to try play again next time
              ESP_LOGE(Tag, "Error playing violon");

            }
            break;
          default:
            current_instrument = PLAY_TONE;
            break;
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
