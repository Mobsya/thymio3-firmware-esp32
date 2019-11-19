//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    attentive.c
//! \brief   This module provides the useful functions to use the attentive mode
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

#include "attentive.h"

#include "accelerometer.h"
#include "codec.h"
#include "common.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "attentive";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Attentive_Init(void)
{

}

//_____________________________________________________________________________

void Attentive_Start(void)
{

}

//_____________________________________________________________________________

void Attentive_Stop(void)
{

}

//_____________________________________________________________________________

// When the Thymio is placed on the left side, the WAV recorder is activated.
// When the Thymio is placed on the right side, the WAV player is activated (replay).
void Attentive_Run(void)
{
  uint8_t brightness = Common_GetBodyColorPulse();
  int16_t acceleration = Accelerometer_GetAccelerationY();

  // Dark blue pulse
  Leds_SetBodyBrightness(0u, 0u, brightness);

  when(acceleration >= 15000)  // Left side
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Startup);
  }

  when(acceleration <= -15000)  // Right side
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Bye);
  }
}
