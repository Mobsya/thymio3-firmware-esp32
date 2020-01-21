//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    painter.c
//! \brief   This module provides the useful functions to use the painter mode
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

#include "aseba_esp32.h"
#include "buttons.h"
#include "common.h"
#include "leds.h"
#include "stm32_spi.h"

#include "painter.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_PainterState_Wait,
  E_PainterState_Record,
  E_PainterState_Play
} T_PainterState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "painter";

static T_PainterState State = E_PainterState_Wait;

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

void Painter_Init(void)
{

}

//_____________________________________________________________________________

void Painter_Start(void)
{

}

//_____________________________________________________________________________

void Painter_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Painter_Run(void)
{
  switch (State)
  {
    case E_PainterState_Wait:
      RunWaitState();
      break;

    case E_PainterState_Record:
      RunRecordState();
      break;

    case E_PainterState_Play:
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

  T_Motor vind = STM32_GetInducedVoltage();

  if (abs(vind.Left + vind.Right) > 400)
  {
    State = E_PainterState_Record;

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
    State = E_PainterState_Play;
    Counter = 0u;
    Index = 1u;
  }
}

//_____________________________________________________________________________

static void RunRecordState(void)
{
  uint8_t* buttonState;

  T_Motor vind = STM32_GetInducedVoltage();
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

      State = E_PainterState_Wait;
    }
  }
}

//_____________________________________________________________________________

static void RunPlayState(void)
{
  uint8_t* buttonState;

  Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);

  Common_SetTargetSpeed(LeftSpeed[Index - 1u], RightSpeed[Index - 1u]);
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
    Common_SetTargetSpeed(0, 0);

    State = E_PainterState_Wait;
  }
}
