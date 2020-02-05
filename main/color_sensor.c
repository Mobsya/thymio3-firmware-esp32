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
#include "codec.h"
#include "leds.h"
#include "settings.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MIN(a,b)      ((a) < (b) ? (a) : (b))
#define MAX(a,b)      ((a) > (b) ? (a) : (b))

#define MIN3(a,b,c)   MIN((a), MIN((b), (c)))
#define MAX3(a,b,c)   MAX((a), MAX((b), (c)))

#define HUE_DEGREE         1

#define MAX_HSV_COLOR    255

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

static T_RawColor White;
static T_RawColor Black;
static T_RawColor Range;
static T_RawColor MaxColor;

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

static void PlayAlarmSound(uint8_t type);

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

  White.Red   = Settings_ReadWhiteRed();
  White.Green = Settings_ReadWhiteGreen();
  White.Blue  = Settings_ReadWhiteBlue();
  White.Clear = 220;

  ESP_LOGI(Tag, "White values: %d, %d, %d", White.Red, White.Green, White.Blue);

  Black.Red   = Settings_ReadBlackRed();
  Black.Green = Settings_ReadBlackGreen();
  Black.Blue  = Settings_ReadBlackBlue();
  Black.Clear = 150;

  ESP_LOGI(Tag, "Black values: %d, %d, %d", Black.Red, Black.Green, Black.Blue);

  Range.Red   = (White.Red - Black.Red);
  Range.Green = (White.Green - Black.Green);
  Range.Blue  = (White.Blue - Black.Blue);
  Range.Clear = (White.Clear - Black.Clear);

  MaxColor.Red   = MAX_HSV_COLOR;
  MaxColor.Green = MAX_HSV_COLOR;
  MaxColor.Blue  = MAX_HSV_COLOR;
  MaxColor.Clear = MAX_HSV_COLOR;

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

void ColorSensor_Calibrate(uint8_t choice)
{
  static bool isWhiteCalibrationDone = false;
  static bool isBlackCalibrationDone = false;

  if (choice == 0)
  {
    White.Red   = RawColor.Red;
    White.Green = RawColor.Green;
    White.Blue  = RawColor.Blue;

    ESP_LOGE(Tag, "White calibration done: %d, %d, %d", White.Red, White.Green, White.Blue);

    isWhiteCalibrationDone = true;
  }
  else if (choice == 1)
  {
    Black.Red   = RawColor.Red;
    Black.Green = RawColor.Green;
    Black.Blue  = RawColor.Blue;

    ESP_LOGE(Tag, "Black calibration done: %d, %d, %d", Black.Red, Black.Green, Black.Blue);

    isBlackCalibrationDone = true;
  }
  else
  {
    // Do nothing
  }

  if (isWhiteCalibrationDone && isBlackCalibrationDone)
  {
    Range.Red   = (White.Red - Black.Red);
    Range.Green = (White.Green - Black.Green);
    Range.Blue  = (White.Blue - Black.Blue);

    if ((Range.Red != 0u) && (Range.Green != 0u) && (Range.Blue != 0u))
    {
      Settings_WriteWhiteRed(White.Red);
      Settings_WriteWhiteGreen(White.Green);
      Settings_WriteWhiteBlue(White.Blue);

      Settings_WriteBlackRed(Black.Red);
      Settings_WriteBlackGreen(Black.Green);
      Settings_WriteBlackBlue(Black.Blue);

      ESP_LOGI(Tag, "Color calibration is OK");
    }
    else
    {
      ESP_LOGE(Tag, "Color calibration is bad");
    }

    isWhiteCalibrationDone = false;
    isBlackCalibrationDone = false;
  }
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
  static bool alarm = false;

  int16_t red   = (RawColor.Red - Black.Red);
  int16_t green = (RawColor.Green - Black.Green);
  int16_t blue  = (RawColor.Blue - Black.Blue);

  uint16_t min;
  uint16_t max;
  uint16_t delta;

  // Check division by 0
  if ((Range.Red != 0u) && (Range.Green != 0u) && (Range.Blue != 0u))
  {
    alarm = false;

    red   = (red * MaxColor.Red) / Range.Red;
    green = (green * MaxColor.Green) / Range.Green;
    blue  = (blue * MaxColor.Blue) / Range.Blue;

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
  }
  else
  {
    // TODO Warn the user in case of division by 0
    alarm = true;
  }

  if(alarm)
  {
    //ESP_LOGE(Tag, "SOUND ALARM");
    //Codec_PlayMP3FileFromFlash(E_SystemSound_Alarm);
	PlayAlarmSound(0);
  }

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

//_____________________________________________________________________________

static void PlayAlarmSound(uint8_t type)
{
  static bool playSound = true;

  if (playSound)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Alarm);
  }

  if (type == 0)//E_AlarmType_Once)
  {
    playSound = false;
  }
  else if (type == 1)//E_AlarmType_Continuous)
  {
    if (Codec_IsSoundFinished(E_SoundIndex_Alarm))
    {
      playSound = true;
    }
    else
    {
      playSound = false;
    }
  }
  else
  {
    // Do nothing
  }
}
