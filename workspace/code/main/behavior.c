//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    behavior.c
//! \brief   This module provides the useful functions to handle the behavior
//!
//! \author  Vincent Gonet
//!
//! \version $Id: behavior.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <stdio.h>  // Only for debug

#include "behavior.h"

#include "aseba_esp32.h"
#include "ground_ir.h"
#include "leds.h"
#include "mode.h"
#include "prox_ir.h"
#include "stm32.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define IR_SENSOR_NUM           (PROX_IR_SENSORS_NUM + GROUND_IR_SENSORS_NUM)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static uint16_t behavior = 0u;

#define ENABLED(b)              (behavior & b)   //({behavior & b;})
#define ENABLE(b)               (behavior |= b)  //do {behavior |= b;} while(0)
#define DISABLE(b)              (behavior &= ~b)//do {behavior &= ~b;} while(0)  //(behavior &= ~b)

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Set the buttons LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetButtonsLeds(void);

//! \brief     Set the IR sensors LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetIRSensorsLeds(void);

//! \brief     Set the accelerometer LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetAccelerometerLeds(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_Run(void)
{
  if (ENABLED(B_LEDS_BUTTON))
  {
    SetButtonsLeds();
  }

  if (ENABLED(B_LEDS_PROX))
  {
    SetIRSensorsLeds();
  }
#if 0  // FIXME
  if (ENABLED(B_LEDS_ACC))
  {
    SetAccelerometerLeds();
  }
#endif
  if (ENABLED(B_MODE))
  {
    Mode_Run();
  }
}

//_____________________________________________________________________________

void Behavior_Start(uint16_t b)
{
  //ENABLE(b);

  behavior |= b;
}

//_____________________________________________________________________________

void Behavior_Stop(uint16_t b)
{
  DISABLE(b);

  //behavior &= ~b;
}

//_____________________________________________________________________________

static void SetButtonsLeds(void)
{
  static uint8_t brightness[BUTTON_NUM] = {MIN_BRIGHTNESS, MIN_BRIGHTNESS, MIN_BRIGHTNESS,
                                           MIN_BRIGHTNESS, MIN_BRIGHTNESS
                                          };
  uint8_t* buttonState;

  buttonState = STM32_GetButtonStatus();

  for (T_Button index = E_Button_Backward; index <= E_Button_Right; index++)
  {
    if (buttonState[index] != 0u)
    {
      brightness[index] += 3u;

      if (brightness[index] > MAX_BRIGHTNESS)
      {
        brightness[index] = MAX_BRIGHTNESS;
      }
    }
    else
    {
      brightness[index] = MIN_BRIGHTNESS;
    }
  }

  if (brightness[E_Button_Center] > MIN_BRIGHTNESS)
  {
    for (T_Led index = E_Led_Button_0; index <= E_Led_Button_1; index++)
    {
      Leds_SetSingleBrightness(index, brightness[E_Button_Center]);
    }
  }
  else
  {
    if (brightness[E_Button_Backward] != MIN_BRIGHTNESS)
    {
      Leds_SetSingleBrightness(E_Led_Button_2, brightness[E_Button_Backward]);
    }

    if (brightness[E_Button_Left] != MIN_BRIGHTNESS)
    {
      Leds_SetSingleBrightness(E_Led_Button_3, brightness[E_Button_Left]);
    }

    if (brightness[E_Button_Forward] != MIN_BRIGHTNESS)
    {
      Leds_SetSingleBrightness(E_Led_Button_0, brightness[E_Button_Forward]);
    }

    if (brightness[E_Button_Right] != MIN_BRIGHTNESS)
    {
      Leds_SetSingleBrightness(E_Led_Button_1, brightness[E_Button_Right]);
    }
  }

  if ((brightness[E_Button_Backward] == MIN_BRIGHTNESS) &&
      (brightness[E_Button_Center] == MIN_BRIGHTNESS))
  {
    Leds_SetSingleBrightness(E_Led_Button_2, MIN_BRIGHTNESS);
  }

  if ((brightness[E_Button_Left] == MIN_BRIGHTNESS) &&
      (brightness[E_Button_Center] == MIN_BRIGHTNESS))
  {
    Leds_SetSingleBrightness(E_Led_Button_3, MIN_BRIGHTNESS);
  }

  if ((brightness[E_Button_Forward] == MIN_BRIGHTNESS) &&
      (brightness[E_Button_Center] == MIN_BRIGHTNESS))
  {
    Leds_SetSingleBrightness(E_Led_Button_0, MIN_BRIGHTNESS);
  }

  if ((brightness[E_Button_Right] == MIN_BRIGHTNESS) &&
      (brightness[E_Button_Center] == MIN_BRIGHTNESS))
  {
    Leds_SetSingleBrightness(E_Led_Button_1, MIN_BRIGHTNESS);
  }
}

//_____________________________________________________________________________

void SetIRSensorsLeds(void)
{
  //static int16_t max[IR_SENSOR_NUM] = {4000, 4000, 4000, 4000, 4000, 4000, 4000, 900, 900};
  static int16_t max[IR_SENSOR_NUM] = {50, 50, 50, 50, 50, 50, 900, 900, 900};
  //static int16_t min[IR_SENSOR_NUM] = {1200, 1200, 1200, 1200, 1200, 1200, 1200, 0, 0};
  static int16_t min[IR_SENSOR_NUM] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

  static T_Led led[IR_SENSOR_NUM] = {E_Led_Front_IR_0, E_Led_Front_IR_1, E_Led_Front_IR_2A,
                                     E_Led_Front_IR_3, E_Led_Front_IR_4, E_Led_IR_Back_Left,
                                     E_Led_IR_Back_Right, E_Led_Ground_IR_0, E_Led_Ground_IR_1
                                    };

  int16_t s = 0;
  int16_t delta = 0;
  int16_t brightness = 0;

  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
    if (max[index] < vmVariables.prox[index])
    {
      max[index] = vmVariables.prox[index];
    }
#if 0
    else if ((vmVariables.prox[index] != 0) && (min[index] > vmVariables.prox[index]))
    {
      min[index] = vmVariables.prox[index];
    }
    else
    {
      // Do nothing
    }
#endif
  }

  for (uint8_t index = 0u; index < GROUND_IR_SENSORS_NUM; index++)
  {
    if (max[index + PROX_IR_SENSORS_NUM] < vmVariables.ground_delta[index])
    {
      max[index + PROX_IR_SENSORS_NUM] = vmVariables.ground_delta[index];
      // min is fixed to 0 ... this is _physical_
    }
  }

  // Do a linear transformation from min-max to led 0-31!
  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
#if 0
    // Because of the min&max calculation above, we cannot have a
    // Division by 0 here.
    s = vmVariables.prox[index] - min[index];
    delta = (max[index] - min[index]);

    if (s < 0)
    {
      s = 0;
    }

    brightness = ((int32_t)s * MAX_BRIGHTNESS) / delta;
#endif
    s = (vmVariables.prox[index] > 0) ? vmVariables.prox[index] : 0;
    brightness = ((int32_t)s * MAX_BRIGHTNESS) / max[index];

    //printf("s = %d, b = %d\n", s, brightness);

    Leds_SetSingleBrightness(led[index], brightness);

    // The Front IR sensor has 2 LEDs (E_Led_Front_IR_2A and E_Led_Front_IR_2B)
    if (index == 2u)
    {
      Leds_SetSingleBrightness(led[index] + 1, brightness);
    }
  }

  for (uint8_t index = 0u; index < GROUND_IR_SENSORS_NUM; index++)
  {
    s = (vmVariables.ground_delta[index] > 0) ? vmVariables.ground_delta[index] : 0;
    brightness = ((int32_t)s * MAX_BRIGHTNESS) / max[index + PROX_IR_SENSORS_NUM];

    Leds_SetSingleBrightness(led[index + PROX_IR_SENSORS_NUM], brightness);
  }
}

//_____________________________________________________________________________

int16_t aseba_atan2(int16_t y, int16_t x); // We use a function which should be private to aseba native ...

static void SetAccelerometerLeds(void)
{
  static int previous_led;

  int intensity;
  int led = -1;

  // FIXME: Use vmVariables ?!
  if (vmVariables.acc_bis[2] < 16800)  // 21
  {
    int ha = (aseba_atan2(vmVariables.acc_bis[0], vmVariables.acc_bis[1]) / 2);

    //printf("z = %d\n", vmVariables.acc_bis[2]);
    //printf("ha = %d\n", ha);

    if ((ha >= -2000) && (ha < 2000))
    {
	  led = E_Led_Circle_4;
    }
	else if ((ha < -2000) && (ha >= -6000))
	{
	  led = E_Led_Circle_3;
	}
	else if (ha < -6000 && ha >= -10000)
	{
	  led = E_Led_Circle_2;
	}
	else if ((ha < -10000) && (ha >= -14000))
	{
	  led = E_Led_Circle_1;
	}
	else if ((ha  < -14000) || (ha >= 14000))
	{
	  led = E_Led_Circle_0;
	}
	else if ((ha < 6000) && (ha >= 2000))
	{
	  led = E_Led_Circle_5;
	}
	else if ((ha < 10000) && (ha >= 6000))
	{
	  led = E_Led_Circle_6;
	}
	else if ((ha < 14000) && (ha >= 10000))
	{
	  led = E_Led_Circle_7;
	}
	else
	{
	  // Do nothing
	}

	//intensity = (40 - (abs(vmVariables.acc_bis[2]) * 2));  // TODO
    intensity = 32;

	if (intensity < 0)
	{
	  intensity = 0;
	}

	if (led >= 0)
	{
	  if (previous_led >= 0)
	  {
	    Leds_SetSingleBrightness(previous_led, 0);
	  }

	  Leds_SetSingleBrightness(led, intensity);
	}

	previous_led = led;

  }
  else
  {
    if (previous_led >= 0)
    {
      Leds_SetSingleBrightness(previous_led, 0);
    }

	previous_led = -1;
  }
}
