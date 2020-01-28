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

#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

#define MIN3(a,b,c)   MIN((a), MIN((b), (c)))
#define MAX3(a,b,c)   MAX((a), MAX((b), (c)))

#define HUE_DEGREE     1

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int16_t Hue;
  int16_t Saturation;
  int16_t Value;
} T_HSV;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "color_sensor";

static T_RawColor RawColor;

static T_HSV Hsv;

static T_Color Color = E_Color_Unknown;

static uint16_t White[4] = {790, 1330, 1080, 220};
static uint16_t Black[4] = {460, 940, 690, 150};
static uint16_t Range[4] = {0, 0, 0, 0};
static uint16_t MaxColor[4] = {255, 255, 255, 255};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Convert the raw color to HSV color
//! \pre       First initialize the color sensor
//! \param     None
//! \return    None
static void ConvertToHSV(void);

//! \brief     Update the color according to the HSV color
//! \pre       First initialize the color sensor
//! \param     hsv - HSV color
//! \return    None
static void UpdateColor(T_HSV hsv);

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

  Range[0] = White[0] - Black[0];
  Range[1] = White[1] - Black[1];
  Range[2] = White[2] - Black[2];

  ESP_LOGI(Tag, "Color sensor is initialized");
}

//_____________________________________________________________________________

void ColorSensor_ReadColor(void)
{
  BH1745NUC_ReadColor(&RawColor);

  vmVariables.color_raw[0] = RawColor.Red;
  vmVariables.color_raw[1] = RawColor.Green;
  vmVariables.color_raw[2] = RawColor.Blue;
  vmVariables.color_raw[3] = RawColor.Clear;

  ConvertToHSV();
  UpdateColor(Hsv);
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

static void ConvertToHSV(void)
{
  int16_t red   = RawColor.Red - Black[0];
  int16_t green = RawColor.Green - Black[1];
  int16_t blue  = RawColor.Blue - Black[2];

  uint16_t min;
  uint16_t max;
  uint16_t delta;

  red   = (red * MaxColor[0]) / Range[0];
  green = (green * MaxColor[1]) / Range[1];
  blue  = (blue * MaxColor[2]) / Range[2];

  min = MIN3(red, green, blue);
  max = MAX3(red, green, blue);
  delta = max - min;

  if (delta == 0)
  {
    // Achromatic case (i.e. grayscale)
    Hsv.Hue = -1;  // Undefined
    Hsv.Saturation = 0;
  }
  else
  {
    int h;

    if (red == max)
    {
      h = ((green - blue) * 60 * HUE_DEGREE) / delta;
    }
    else if (green == max)
    {
      h = (((blue - red) * 60 * HUE_DEGREE) / delta) + (120 * HUE_DEGREE);
    }
    else  // blue == max
    {
      h = (((red - green) * 60 * HUE_DEGREE) / delta) + (240 * HUE_DEGREE);
    }

    if (h < 0)
    {
      h += 360 * HUE_DEGREE;
    }

    Hsv.Hue = h;

    if (max != 0)
    {
      Hsv.Saturation = (128 * delta) / max;
    }
    else
    {
      Hsv.Saturation = 0;
    }
  }

  Hsv.Value = max;

  vmVariables.color_hsv[0] = Hsv.Hue;
  vmVariables.color_hsv[1] = Hsv.Saturation;
  vmVariables.color_hsv[2] = Hsv.Value;

  //ESP_LOGI(Tag, "H: %d, S: %d, V: %d", Hsv.Hue, Hsv.Saturation, Hsv.Value);
}

//_____________________________________________________________________________

static void UpdateColor(T_HSV hsv)
{
  if ((hsv.Saturation < 50) && (hsv.Value > 200))
  {
    // White
    Color = E_Color_White;
  }
  else if (hsv.Value < 30)
  {
    // Black
    Color = E_Color_Unknown;
  }
  else if (hsv.Saturation > 50)  // Color
  {
    if (((hsv.Hue >= 0) && (hsv.Hue <= 10)) || ((hsv.Hue > 350) && (hsv.Hue < 360)))
    {
      // Red
      Color = E_Color_Red;
    }
    else if (hsv.Hue <= 25)
    {
      // Yellow
      Color = E_Color_Orange;
    }
    else if (hsv.Hue <= 80)
    {
      // Yellow
      Color = E_Color_Yellow;
    }
    else if (hsv.Hue <= 150)
    {
      // Green
      Color = E_Color_Green;
    }
    else if (hsv.Hue <= 200)  // 210
    {
      // Cyan
      Color = E_Color_Cyan;
    }
    else if (hsv.Hue <= 270)
    {
      // Blue
      Color = E_Color_Blue;
    }
    else if (hsv.Hue <= 350)
    {
      // Purple
      Color = E_Color_Purple;
    }
    else
    {
      Color = E_Color_Unknown;
    }
  }
  else
  {
    Color = E_Color_Unknown;
  }
}
