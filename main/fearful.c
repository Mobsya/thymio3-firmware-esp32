//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    fearful.c
//! \brief   This module provides the useful functions to use the fearful mode
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

//static const char* Tag = "fearful";

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
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Fearful_Run(void)
{
  uint8_t brightness = Common_GetBodyColorPulse();
  static uint8_t play_state = 0; // 0 = not playing, 1 = play, 2 = wait play finish
  static T_SoundIndex sound = 0;
  static uint8_t play_timeout = 0;

  if (Accelerometer_IsFreeFallDetected())
  {
    //ESP_LOGE(Tag, "acc = %d", acc);
    //ESP_LOGE(Tag, "FREE FALL DETECTED");
    sound = E_SoundIndex_Fall;
    play_state = 1;
  }

  if (Accelerometer_IsTapDetected())
  {
    sound = E_SoundIndex_Alarm;
    play_state = 1;
  }

  // Moving part.
  if(Common_HandleTableEdgeDetection(brightness, 0u, 0u) == 0) { // No table edge detected
    // If all proximities "covered" then stop and play a sound
    if ((vmVariables.prox[1] > ACC_OBSTACLE) && (vmVariables.prox[2] > ACC_OBSTACLE) &&
        (vmVariables.prox[3] > ACC_OBSTACLE) &&
        ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))) //&&
      //(vmVariables.ground_delta[0] > 130 && vmVariables.ground_delta[1] > 130))
    {
      Common_SetTargetSpeed(0, 0);
      if(play_state == 0) { // If not already playing
        sound = E_SoundIndex_Alarm;
        play_state = 1;
      }
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

      Common_SetTargetSpeed((-(temp + temp2)), (temp2 - temp));
    }
    else if ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))
    {
      Common_SetTargetSpeed((vmVariables.prox[5] / 2), (vmVariables.prox[6] / 2));
    }
    else
    {
      Common_SetTargetSpeed(0, 0);
    }
  }

  Common_LimitSpeed(MIN_LIMIT_SPEED, MAX_LIMIT_SPEED);

  switch(play_state) {
    case 0: // not playing
      break;
    case 1: // start play
      Codec_Stop();
      Codec_PlayOnboardSound(sound);
      play_state = 2;
      play_timeout = 0;
      break;
    case 2: // wait play finish      
      if(Codec_IsSoundFinished()) {
        play_state = 0;
      }
      play_timeout++;
      if(play_timeout >= 75) { // Behaviors tasks run @ 25 hz, so if after 3 seconds the sound is still not finished, then restart anyway
        play_state = 0;
      }
  }
}
