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

static int16_t LeftSpeeds[100] = {0}; // Record 10 seconds at 10 hz
static int16_t RightSpeeds[100] = {0};
static uint8_t Counter10Hz = 0;
static uint8_t SpeedsIndex = 0;
static uint8_t InputMode = 0; // 0 = manually moving motors, 1 = obstacle avoidance, 2 = tv remote, ...

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
  static uint8_t brightness = 0;

  brightness = Common_GetBodyColorPulse();
  Leds_SetFrontBrightness(0, 0, brightness);
  Leds_SetBackBrightness(brightness, brightness, 0);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

  T_Motor vind = STM32_GetInducedVoltage();

  if (abs(vind.Left + vind.Right) > 200)
  {
    State = E_PainterState_Record;

    CurrentStep = 1u;
    CounterStep = 0u;

    for (uint8_t index = 0u; index < 4u; index++)
    {
      LeftSpeed[index] = 0;
      RightSpeed[index] = 0;
    }

    SpeedsIndex = 0;
    Counter10Hz = 0;
    InputMode = 0;
  }

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {

    State = E_PainterState_Record;

    CurrentStep = 1u;
    CounterStep = 0u;

    for (uint8_t index = 0u; index < 4u; index++)
    {
      LeftSpeed[index] = 0;
      RightSpeed[index] = 0;
    }

    SpeedsIndex = 0;
    Counter10Hz = 0;
    InputMode = 1;  
  }

  when(buttonState[E_Button_Right])
  {
    State = E_PainterState_Play;
    Counter = 0u;
    Index = 1u;

    Counter10Hz = 0;
    SpeedsIndex = 0;
  }
}

//_____________________________________________________________________________

static void RunRecordState(void)
{
  uint8_t* buttonState;

  T_Motor vind = STM32_GetInducedVoltage();
  int16_t temp = 0;

  Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);

  // Handle circle leds animation
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

  if(SpeedsIndex < 100) {
    if(InputMode == 1) {
      Common_HandlePositiveSpeed(250);  
    }
    Counter10Hz++;
    if(Counter10Hz == 5) {  // Behaviors loop runs at 50 Hz
      Counter10Hz = 0;
      LeftSpeeds[SpeedsIndex] = vind.Left;
      RightSpeeds[SpeedsIndex] = vind.Right;
      SpeedsIndex++;
      if(SpeedsIndex == 100) {
        Common_SetTargetSpeed(0, 0);              
      }
    }    
  } else {
    if((vind.Left == 0) && (vind.Right == 0)) { // Wait for the speed to be zero before exiting the recording state otherwise this state will be entered again immediately
      State = E_PainterState_Wait;
    }
  }

/*
  if ((abs(vind.Left) > 50) || (abs(vind.Right) > 50))
  {
    CounterStep++;

    temp = LeftSpeed[CurrentStep - 1u] + vind.Left;

    if ((LeftSpeed[CurrentStep - 1u] > 0) && (vind.Left > 0) && (temp < 0)) // handle overflow
    {
      LeftSpeed[CurrentStep - 1u] = 32767;
    }
    else if ((LeftSpeed[CurrentStep - 1u] < 0) && (vind.Left < 0) && (temp > 0)) // handle underflow
    {
      LeftSpeed[CurrentStep - 1u] = -32767;
    }
    else
    {
      LeftSpeed[CurrentStep - 1u] = temp;
    }

    temp = RightSpeed[CurrentStep - 1u] + vind.Right;

    if ((RightSpeed[CurrentStep - 1u] > 0) && (vind.Right > 0) && (temp < 0)) // handle overflow
    {
      RightSpeed[CurrentStep - 1u] = 32767;
    }
    else if ((RightSpeed[CurrentStep - 1u] < 0) && (vind.Right < 0) && (temp > 0)) // handle underflow
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
*/

}

//_____________________________________________________________________________

static void RunPlayState(void)
{
  uint8_t* buttonState;

  Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);

  T_Motor vind = STM32_GetInducedVoltage();

  if(SpeedsIndex < 100) {
    Counter10Hz++;
    if(Counter10Hz == 5) {  // Behaviors loop runs at 50 Hz
      Counter10Hz = 0;
      Common_SetTargetSpeed(LeftSpeeds[SpeedsIndex] , RightSpeeds[SpeedsIndex]);
      SpeedsIndex++;
      if(SpeedsIndex == 100) {
        Common_SetTargetSpeed(0, 0);
      }
    }
  } else {
    if((vind.Left == 0) && (vind.Right == 0)) { // Wait for the speed to be zero before exiting the recording state otherwise this state will be entered again immediately
      State = E_PainterState_Wait;
    }
  }

/*
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
*/  
}
