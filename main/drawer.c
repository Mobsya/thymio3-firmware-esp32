//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    drawer.c
//! \brief   This module provides the useful functions to use the drawer mode
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

#include "drawer.h"

#include "aseba_esp32.h"
#include "buttons.h"
#include "common.h"
#include "leds.h"
#include "stm32_i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_DrawerState_Wait,
  E_DrawerState_Record,
  E_DrawerState_Play
} T_DrawerState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "drawer";

static T_DrawerState State = E_DrawerState_Wait;

static uint8_t CurrentStep = 0u;
static uint8_t CounterStep = 0u;
static uint8_t Counter = 0u;
static uint8_t Index = 0u;
static uint8_t Duration[4] = {0u, 0u, 0u, 0u};

static int16_t LeftSpeed[4] = {0, 0, 0, 0};
static int16_t RightSpeed[4] = {0, 0, 0, 0};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunWaitState(void);

static void RunRecordState(void);

static void RunPlayState(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Drawer_Init(void)
{

}

//_____________________________________________________________________________

void Drawer_Start(void)
{

}

//_____________________________________________________________________________

void Drawer_Stop(void)
{

}

//_____________________________________________________________________________

void Drawer_Run(void)
{
  switch (State)
  {
    case E_DrawerState_Wait:
      RunWaitState();
      break;

    case E_DrawerState_Record:
      RunRecordState();
      break;

    case E_DrawerState_Play:
      RunPlayState();
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void RunWaitState(void)
{
  uint8_t* buttonState;

  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);

  T_Vind vind = STM32_GetInducedVoltage();

  if (abs(vind.Left + vind.Right) > 400)
  {
    State = E_DrawerState_Record;

    CurrentStep = 1u;
    CounterStep = 0u;

    for (uint8_t index = 0u; index < 4u; index++)
    {
      LeftSpeed[index] = 0;
      RightSpeed[index] = 0;
    }
  }

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    State = E_DrawerState_Play;
    Counter = 0u;
    Index = 1u;
  }
}

//_____________________________________________________________________________

static void RunRecordState(void)
{
  uint8_t* buttonState;

  T_Vind vind = STM32_GetInducedVoltage();
  int16_t temp = 0;

  Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);

  if (CurrentStep == 1u)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  }
  else if (CurrentStep == 2u)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u);
  }
  else if (CurrentStep == 3u)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
  }
  else if (CurrentStep == 4u)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else
  {
    // Do nothing
  }

  if ((abs(vind.Left) > 50) || (abs(vind.Right) > 50))
  {
    CounterStep++;

    temp = LeftSpeed[CurrentStep - 1u] + vind.Left;

    if ((LeftSpeed[CurrentStep - 1u] > 0) && (vind.Left > 0) && (temp < 0))
    {
      LeftSpeed[CurrentStep - 1u] = 32767;
    }
    else if ((LeftSpeed[CurrentStep - 1u] < 0) && (vind.Left < 0) && (temp > 0))
    {
	  LeftSpeed[CurrentStep - 1u] = -32767;
    }
    else
    {
	  LeftSpeed[CurrentStep - 1u] = temp;
    }

    temp = RightSpeed[CurrentStep - 1u] + vind.Right;

    if ((RightSpeed[CurrentStep - 1u] > 0) && (vind.Right > 0) && (temp < 0))
    {
      RightSpeed[CurrentStep - 1u] = 32767;
    }
    else if ((RightSpeed[CurrentStep - 1u] < 0) && (vind.Right < 0) && (temp > 0))
    {
      RightSpeed[CurrentStep - 1u] = -32767;
    }
    else
    {
      RightSpeed[CurrentStep - 1u] = temp;
    }
  }

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    if (CounterStep > 0u)
    {
	  LeftSpeed[CurrentStep - 1u] = LeftSpeed[CurrentStep - 1u] / CounterStep;
      RightSpeed[CurrentStep - 1u] = RightSpeed[CurrentStep - 1u] / CounterStep;
      Duration[CurrentStep - 1u] = CounterStep;
    }
	else
	{
      LeftSpeed[CurrentStep - 1u] = 0;
      RightSpeed[CurrentStep - 1u] = 0;
      Duration[CurrentStep - 1u] = 0;
	}

    CurrentStep++;
	CounterStep = 0u;

	if ((CurrentStep - 1u) >= 4u)
	{
	  CurrentStep = 0u;
	  CounterStep = 0u;

	  State = E_DrawerState_Wait;
	}
  }
}

//_____________________________________________________________________________

static void RunPlayState(void)
{
  uint8_t* buttonState;

  Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);

  vmVariables.target[0] = LeftSpeed[Index - 1u];
  vmVariables.target[1] = RightSpeed[Index - 1u];
  Counter++;

  if (Counter >= Duration[Index - 1u])
  {
    Counter = 0u;
    Index++;

    if (Index > 4u)
    {
      Counter = 0u;
      Index = 1u;
    }
  }

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    vmVariables.target[0] = 0;
	vmVariables.target[1] = 0;

	State = E_DrawerState_Wait;
  }
}
