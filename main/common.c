//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    common.c
//! \brief   This module provides the useful functions to use the common mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "common.h"

#include "aseba_esp32.h"
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

static const char* Tag = "common";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

uint8_t Common_GetBodyColorPulse(void)
{
  static int16_t pulse = 0;
  int16_t brightness = 0;

  pulse++;

  if (pulse > 0)
  {
	brightness = pulse;

    if (pulse >= MAX_BRIGHTNESS)
    {
      pulse = -(MAX_BRIGHTNESS * 4);
    }
  }
  else
  {
    brightness = -pulse / 4;
  }

  return (uint8_t)brightness;
}

//_____________________________________________________________________________

void Common_LimitSpeed(int16_t min, int16_t max)
{
  if (vmVariables.target[0] < min)
  {
    vmVariables.target[0] = min;
  }
  else if (vmVariables.target[0] > max)
  {
    vmVariables.target[0] = max;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < min)
  {
    vmVariables.target[1] = min;
  }
  else if (vmVariables.target[1] > max)
  {
    vmVariables.target[1] = max;
  }
  else
  {
    // Do nothing
  }
}
