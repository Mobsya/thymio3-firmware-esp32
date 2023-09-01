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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"
#include <math.h>

#include "color_sensor.h"

#include "aseba_esp32.h"
#include "behavior.h"
#include "bh1745nuc.h"
#include "leds.h"
#include "settings.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define HUE_DEGREE               1

#define MAX_HSV_COLOR          255

#define MIN_WHITE_BLACK_GAP    200

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

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//! \brief     Get the lowest value
//! \pre       None
//! \param     None
//! \return    Smallest value
static inline float fMin(float a, float b);

//! \brief     Get the highest value
//! \pre       None
//! \param     None
//! \return    Highest value
static inline float fMax(float a, float b);

//! \brief     Get the lowest value
//! \pre       None
//! \param     None
//! \return    Smallest value
static inline float fMin3(float a, float b, float c);

//! \brief     Get the highest value
//! \pre       None
//! \param     None
//! \return    Highest value
static inline float fMax3(float a, float b, float c);




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

T_HSV ColorSensor_GetHsv(void)
{
  return Hsv;
}

//_____________________________________________________________________________

T_RawColor ColorSensor_GetRaw(void)
{
  return RawColor;
}

//_____________________________________________________________________________

T_RawColor ColorSensor_GetWhiteCalibration(void)
{
  return White;
}

//_____________________________________________________________________________

T_RawColor ColorSensor_GetBlackCalibration(void)
{
  return Black;
}

//_____________________________________________________________________________

bool ColorSensor_Calibrate(uint8_t choice, uint8_t* calibrationStatus)
{
  static bool isWhiteCalibrationDone = false;
  static bool isBlackCalibrationDone = false;
  bool status = false;

  if (choice == 0)  // White calibration
  {
	// If black calibration is not already performed, white calibration
	// is accepted unconditionally. Otherwise, the black calibration
	// is taken into account to accept or reject the white calibration.
    if (!isBlackCalibrationDone)
    {
      White.Red   = RawColor.Red;
      White.Green = RawColor.Green;
      White.Blue  = RawColor.Blue;

      status = true;
      isWhiteCalibrationDone = true;
    }
    else
    {
      if ((RawColor.Red > (Black.Red + MIN_WHITE_BLACK_GAP)) && (RawColor.Green > (Black.Green + MIN_WHITE_BLACK_GAP)) &&
    	  (RawColor.Blue > (Black.Blue + MIN_WHITE_BLACK_GAP)))
      {
        White.Red   = RawColor.Red;
        White.Green = RawColor.Green;
        White.Blue  = RawColor.Blue;

        status = true;
        isWhiteCalibrationDone = true;
      }
      else
      {
        isWhiteCalibrationDone = false;
      }
    }

    ESP_LOGE(Tag, "White calibration: %d, %d, %d", RawColor.Red, RawColor.Green, RawColor.Blue);
  }
  else if (choice == 1)  // Black calibration
  {
    // If white calibration is not already performed, black calibration
    // is accepted unconditionally. Otherwise, the white calibration
	// is taken into account to accept or reject the black calibration.
    if (!isWhiteCalibrationDone)
	{
	  Black.Red   = RawColor.Red;
	  Black.Green = RawColor.Green;
	  Black.Blue  = RawColor.Blue;

	  status = true;
	  isBlackCalibrationDone = true;
	}
	else
	{
	  if ((RawColor.Red < (White.Red - MIN_WHITE_BLACK_GAP)) && (RawColor.Green < (White.Green - MIN_WHITE_BLACK_GAP)) &&
          (RawColor.Blue < (White.Blue - MIN_WHITE_BLACK_GAP)))
	  {
	    Black.Red   = RawColor.Red;
		Black.Green = RawColor.Green;
		Black.Blue  = RawColor.Blue;

	    status = true;
	    isBlackCalibrationDone = true;
	  }
	  else
	  {
	    isBlackCalibrationDone = false;
	  }
	}

	ESP_LOGE(Tag, "Black calibration: %d, %d, %d", RawColor.Red, RawColor.Green, RawColor.Blue);
  }
  else
  {
    // Do nothing
  }

  //ESP_LOGE(Tag, "%d, %d, %d", isWhiteCalibrationDone, isBlackCalibrationDone, status);

  if (!status)
  {
    *calibrationStatus = 0u;
  }
  else if ((isWhiteCalibrationDone && !isBlackCalibrationDone) || (!isWhiteCalibrationDone && isBlackCalibrationDone))
  {
    *calibrationStatus = 1u;
  }
  else if (isWhiteCalibrationDone && isBlackCalibrationDone)
  {
    Range.Red   = (White.Red - Black.Red);
    Range.Green = (White.Green - Black.Green);
    Range.Blue  = (White.Blue - Black.Blue);

    if ((Range.Red != 0) && (Range.Green != 0) && (Range.Blue != 0))
    {
      Settings_WriteWhiteRed(White.Red);
      Settings_WriteWhiteGreen(White.Green);
      Settings_WriteWhiteBlue(White.Blue);

      Settings_WriteBlackRed(Black.Red);
      Settings_WriteBlackGreen(Black.Green);
      Settings_WriteBlackBlue(Black.Blue);

      *calibrationStatus = 2u;
      ESP_LOGE(Tag, "Color calibration is OK");
    }
    else
    {
      ESP_LOGE(Tag, "Color calibration is bad");
    }

    isWhiteCalibrationDone = false;
    isBlackCalibrationDone = false;
  }
  else
  {
	// Do nothing
  }

  return status;
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
  float red   = (RawColor.Red - Black.Red);
  float green = (RawColor.Green - Black.Green);
  float blue  = (RawColor.Blue - Black.Blue);
  
  if(red > Range.Red) {
    red = Range.Red;
  }
  if(red < 0) {
    red = 0;
  }

  if(green > Range.Green) {
    green = Range.Green;
  }
  if(green < 0) {
    green = 0;
  }

  if(red > Range.Red) {
    red = Range.Red;
  }
  if(red < 0) {
    red = 0;
  }

  if(blue > Range.Blue) {
    blue = Range.Blue;
  }
  if(blue < 0) {
    blue = 0;
  }  

  float cmin;
  float cmax;
  float delta;

  // Check division by 0
  if ((Range.Red == 0) || (Range.Green == 0) || (Range.Blue == 0))
  {
    Hsv.Hue = -1;
    Hsv.Saturation = -1;
    Hsv.Value = -1;
    return;
  }

  // Make the RGB values between 0 and 1
  red   /= Range.Red;
  green /= Range.Green;
  blue  /= Range.Blue;
  
  cmin = fMin3(red, green, blue);
  cmax = fMax3(red, green, blue);
  delta = cmax - cmin;

  Hsv.Value = (int16_t)(cmax*100);

  if (cmin == cmax) {
    Hsv.Hue = 0;
    Hsv.Saturation = 0;
  } else if(cmax == red) {
    Hsv.Hue = (int16_t)(fmod((60 * ((green - blue) / delta) + 360), 360.0));
  } else if(cmax == green) {
    Hsv.Hue = (int16_t)(fmod((60 * ((blue - red) / delta) + 120), 360.0));
  } else if(cmax == blue) {
    Hsv.Hue = (int16_t)(fmod((60 * ((red - green) / delta) + 240), 360.0));
  }
  
  if (cmax == 0) {
    Hsv.Saturation = 0;
  } else {
    Hsv.Saturation = (int16_t)((delta/cmax)*100);
  }

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

//_____________________________________________________________________________

static inline float fMin(float a, float b)
{
  return ((a < b) ? a : b);
}

//_____________________________________________________________________________

static inline float fMax(float a, float b)
{
  return ((a > b) ? a : b);
}

//_____________________________________________________________________________

static inline float fMin3(float a, float b, float c)
{
  return fMin(a, fMin(b, c));
}

//_____________________________________________________________________________

static inline float fMax3(float a, float b, float c)
{
  return fMax(a, fMax(b, c));
}