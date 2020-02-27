//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    friendly.c
//! \brief   This module provides the useful functions to use the friendly mode
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

#include "aseba_esp32.h"
#include "codec.h"
#include "common.h"
#include "leds.h"

#include "friendly.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DETECT                  85

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "friendly";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Friendly_Init(void)
{

}

//_____________________________________________________________________________

void Friendly_Start(void)
{

}

//_____________________________________________________________________________

void Friendly_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Friendly_Run(void)
{
  int16_t max = vmVariables.prox[0];
  int16_t min = 0;
  int16_t t;
  uint8_t brightness = Common_GetBodyColorPulse();
  int16_t speedDiff;
  int16_t speed_l = 0;

  static int16_t speed = 300;

  for (uint8_t index = 1u; index < 5u; index++)
  {
    if (vmVariables.prox[index] > max)
    {
      max = vmVariables.prox[index];
      min = index;
    }
  }

  t = 2 - min;
  speedDiff = t * (speed / 2);

  if (max > 600)
  {
    speed_l = (600 - max) / 2;
  }

  if (max > 700)
  {
    speed_l = -speed;
  }

  if (max < 520)
  {
    t = 52 - ((max - 175) / 7);
    speed_l = t;
  }

  if (max < 350)
  {
    speed_l = speed;
  }

  if (speed_l > speed)
  {
    speed_l = speed;
  }

  if (speed_l < -speed)
  {
    speed_l = -speed;
  }

  if (max < DETECT)
  {
#if 0  // FIXME
    if (does_see_friend)
    {
      Common_SetTargetSpeed(speed, speed);
    }
    else
#endif
    {
      Common_SetTargetSpeed(0, 0);
    }
  }
  else
  {
    Common_SetTargetSpeed((speed_l - speedDiff), (speedDiff + speed_l));
  }

  when(max > DETECT)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Detection);
  }

  Common_HandleTableEdgeDetection(0u, brightness, 0u);

#if 0
  static char sound_done;
  static char does_see_friend = 1;  // FIXME
  static unsigned char led_state;
  static char led_delta = 1;
  static int16_t speed = 300;

#define DETECT 500

  int i;
  int speed_diff;
  int speed_l = 0;
  int max, mi, t;

  max = vmVariables.prox[0];
  mi = 0;

  for (i = 1; i < 5; i++)
  {
    if (vmVariables.prox[i] > max)
    {
      max = vmVariables.prox[i];
      mi = i;
    }
  }

  t = (2 - mi);
  speed_diff = t * (speed / 2);

  if (max > 3500)
  {
    speed_l = (3500 - max) / 2;
  }

  if (max > 4000)
  {
    speed_l = -speed;
  }

  if (max < 3000)
  {
    t = 300 - ((max - 1000) / 7);
    speed_l = t;
  }

  if (max < 2000)
  {
    speed_l = speed;
  }

  if (speed_l > speed)
  {
    speed_l = speed;
  }

  if (speed_l < -speed)
  {
    speed_l = -speed;
  }

  if (max < DETECT)
  {
    //if (does_see_friend)
    {
      Common_SetTargetSpeed(speed, speed);
    }
#if 0  // FIXME
    else
    {
      Common_SetTargetSpeed(0, 0);
    }
#endif
  }
  else
  {
    Common_SetTargetSpeed((speed_l - speed_diff), (speed_diff + speed_l));
  }

  if ((does_see_friend > 0) && sound_done)
  {
    unsigned char rgb[3];

    GetRainbow(rgb);

    // FIXME Leds_SetTopBrightness(rgb[0], rgb[1], rgb[2]);
    // FIXME Leds_SetBottomLeftBrightness(rgb[2], rgb[0], rgb[1]);
    // FIXME Leds_SetBottomRightBrightness(rgb[1], rgb[2], rgb[0]);
  }
  else
  {
    Leds_SetBodyBrightness(0, Common_GetBodyColorPulse(), 0);
  }

  if (does_see_friend)
  {
    led_state += led_delta;

    if (led_state >= 31)
    {
      led_delta = -1;
    }
    else if (led_state == 0)
    {
      led_delta = 1;
    }
    else
    {
      // Do nothing
    }

    Leds_SetCircleBrightness(0, (led_state >> 4), (led_state >> 3), led_state, MAX_BRIGHTNESS, led_state, (led_state >> 3),
                             (led_state >> 4));
  }
  else
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u);
  }

  // Buttons management
  SetSpeedUsingButtons(&speed);

  when(max > DETECT)
  {
    // play_sound(SOUND_F_DETECT);  // FIXME
  }

  if (speed_diff == 0 && speed_l == 0 && sound_done == 0 && max > DETECT)
  {
    sound_done = 1;
    //play_sound(SOUND_F_OK);  // FIXME
  }

  if ((speed_diff != 0) || (max < DETECT))
  {
    sound_done = 0;
  }

  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    Common_SetTargetSpeed(0, 0);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, MAX_BRIGHTNESS);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, MAX_BRIGHTNESS);
  }
  else
  {
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0u);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0u);
  }

  if (does_see_friend)
  {
    does_see_friend--;
  }
#if 0  // FIXME
  if (IS_EVENT(EVENT_DATA))
  {
    CLEAR_EVENT(EVENT_DATA);
    does_see_friend = 0;
    mi = 0;
    max = vmVariables.intensity[0];
    vmVariables.intensity[0] = 0;

    for (i = 1; i < 7; i++)
    {
      if (vmVariables.intensity[i] > max)
      {
        mi = i;
        max = vmVariables.intensity[i];
      }

      vmVariables.intensity[i] = 0;
    }

    if (max > 3000)
    {
      vmVariables.ir_tx_data = mi;

      if ((vmVariables.rx_data > 0) && (vmVariables.rx_data < 4))
      {
        when(mi == 2)
        {
          play_sound(SOUND_F_OK);
        }

        if (mi == 2)
        {
          does_see_friend = 6;
        }
      }
    }
  }
#endif
#endif
}

//_____________________________________________________________________________
