//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    fearful.c
//! \brief   This module provides the useful functions to use the fearful mode
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

#include "fearful.h"

#include "accelerometer.h"
#include "aseba_esp32.h"
#include "codec.h"
#include "common.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ACC_OBSTACLE                175
#define ACC_FREE_FALL              1000

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "fearful";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Fearful_Init(void)
{

}

//_____________________________________________________________________________

void Fearful_Start(void)
{
  Accelerometer_ClearTapStatus();  // Clear any tap made before entering this mode
}

//_____________________________________________________________________________

void Fearful_Stop(void)
{
  vmVariables.target[0] = 0;
  vmVariables.target[1] = 0;
}

//_____________________________________________________________________________

void Fearful_Run(void)
{
  uint8_t brightness = Common_GetBodyColorPulse();
  bool play = false;
//  static unsigned int acc = 32;
//  static uint8_t counter = 0u;

  // Red pulse
  //Leds_SetBodyBrightness(brightness, 0u, 0u);

  //acc = acc + acc + acc + abs(vmVariables.acc[0]) + abs(vmVariables.acc[1]) + abs(vmVariables.acc[2]);
  //acc >>= 2;
  //acc = abs(vmVariables.acc[0]) + abs(vmVariables.acc[1]) + abs(vmVariables.acc[2]);

  //if (acc < ACC_FREE_FALL)
  if (Accelerometer_IsFreeFallDetected())
  {
    //ESP_LOGE(Tag, "acc = %d", acc);
    //ESP_LOGE(Tag, "FREE FALL DETECTED");
    play = true;
  }

#if 0
  when(acc > ACC_FREE_FALL)
  {
    Leds_SetBodyBrightness((MAX_BRIGHTNESS / 2), 0u, 0u);
  }

  if (acc < ACC_FREE_FALL)
  {
    counter++;

    if (counter > 5)
    {
      if (counter == 10)
      {
        counter = 0;
      }

      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
    }
    else
    {
      Leds_SetBodyBrightness(0u, 0u, 0u);
    }
  }
  else
  {
    // Red pulse
    Leds_SetBodyBrightness(brightness, 0u, 0u);
  }
#endif

#if 0
  if (Accelerometer_IsTapDetected())
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }
#endif

//#if 0
  // Moving part.
  if ((vmVariables.prox[1] > ACC_OBSTACLE) && (vmVariables.prox[2] > ACC_OBSTACLE)
      && (vmVariables.prox[3] > ACC_OBSTACLE) &&
      ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))) //&&
    //(vmVariables.ground_delta[0] > 130 && vmVariables.ground_delta[1] > 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    play = true;
  }
  else if ((vmVariables.prox[0] > ACC_OBSTACLE) || (vmVariables.prox[1] > ACC_OBSTACLE) ||
           (vmVariables.prox[2] > ACC_OBSTACLE) || (vmVariables.prox[3] > ACC_OBSTACLE) ||
           (vmVariables.prox[4] > ACC_OBSTACLE))
  {
    //int temp = vmVariables.prox[0]/5 + vmVariables.prox[1]/4 + vmVariables.prox[2]/4;
    //temp += vmVariables.prox[3]/4 + vmVariables.prox[4]/5;
    int16_t temp = (vmVariables.prox[0] / 3) + (vmVariables.prox[1] / 2) + (vmVariables.prox[2] / 2);
    temp += (vmVariables.prox[3] / 2) + (vmVariables.prox[4] / 3);

    int16_t temp2 = (vmVariables.prox[0] / 4) + (vmVariables.prox[1] / 3);
    temp2 -= (vmVariables.prox[3] / 3) + (vmVariables.prox[4] / 4);

    vmVariables.target[0] = -(temp + temp2);
    vmVariables.target[1] = temp2 - temp;

    //ESP_LOGE(Tag, "target_left = %d, target_right = %d", vmVariables.target[0], vmVariables.target[1]);
  }
  else if ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))
  {
    vmVariables.target[0] = vmVariables.prox[5] / 2;
    vmVariables.target[1] = vmVariables.prox[6] / 2;
  }
  else
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
  }
#if 0
  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    leds_set_br(32, 0, 0);
    leds_set_bl(32, 0, 0);
  }
  else
  {
    leds_set_br(0, 0, 0);
    leds_set_bl(0, 0, 0);
  }
#endif
  Common_LimitSpeed(MIN_SPEED, MAX_SPEED);
//#endif
  when(play)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Fall);
  }
}
