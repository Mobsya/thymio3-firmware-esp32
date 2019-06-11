//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    color_sensor.c
//! \brief   This module provides the useful functions to use the color sensor
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "color_sensor.h"

#include "aseba_esp32.h"
#include "bh1745nuc.h"
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

static const char* Tag = "color_sensor";

static T_Illuminance Illuminance;

static T_Color Color = E_Color_Unknown;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void DetectColor(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void ColorSensor_Init(void)
{
  BH1745NUC_Init();
  Leds_SetSingleBrightness(E_Led_White_Sensor, MAX_BRIGHTNESS);

  ESP_LOGI(Tag, "Color sensor is initialized");
}

//_____________________________________________________________________________

void ColorSensor_ReadColor(void)
{
  BH1745NUC_ReadIlluminance(&Illuminance);

  vmVariables.color[0] = Illuminance.Red;
  vmVariables.color[1] = Illuminance.Green;
  vmVariables.color[2] = Illuminance.Blue;
  vmVariables.color[3] = Illuminance.Clear;

  SET_EVENT(EVENT_COLOR);

  DetectColor();
}

//_____________________________________________________________________________

T_Color ColorSensor_GetColor(void)
{
  return Color;
}

//_____________________________________________________________________________

T_Error ColorSensor_CheckManufacturerId(void)
{
  T_Error err = E_Error_None;

  if (BH1745NUC_CheckManufacturerId() != E_Error_None)
  {
    err = E_Error_Color_InvalidID;
  }

  ESP_LOGI(Tag, "Color sensor test is done");

  return err;
}

//_____________________________________________________________________________

static void DetectColor(void)
{
  int16_t deltaRedBlue    = abs(Illuminance.Red -  Illuminance.Blue);
  int16_t deltaRedClear   = abs(Illuminance.Red -  Illuminance.Clear);
  int16_t deltaGreenBlue  = abs(Illuminance.Green -  Illuminance.Blue);
  int16_t deltaGreenClear = abs(Illuminance.Green -  Illuminance.Clear);

  if (deltaRedBlue < 3000)
  {
    if (deltaGreenBlue < 1000)
    {
      Color = E_Color_Red;
    }
    else if (deltaGreenBlue < 2000)
    {
      Color = E_Color_Orange;
    }
    else
    {
      Color = E_Color_Yellow;
    }
  }
  else if (deltaRedClear < 800)
  {
    if (deltaGreenBlue > 2000)
    {
      Color = E_Color_Green;
    }
    else if (deltaGreenClear > 7000)
    {
      Color = E_Color_Cyan;
    }
    else if (deltaGreenClear > 5200)
    {
      Color = E_Color_Blue;
    }
    else
    {
      Color = E_Color_Unknown;
    }
  }
  else if (deltaGreenBlue < 200)
  {
    Color = E_Color_Purple;
  }
  else if (deltaGreenClear > 9000)
  {
    Color = E_Color_White;
  }
  else
  {
    Color = E_Color_Unknown;
  }
}
