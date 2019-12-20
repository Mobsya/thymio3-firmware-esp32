//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "behavior.h"

#include "accelerometer.h"
#include "buttons.h"
#include "codec.h"
#include "common.h"
#include "gpio.h"
#include "gyroscope.h"
#include "leds.h"
#include "mode.h"
#include "rc5.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BAT_HIGH        390
#define BAT_MIDDLE      360
#define BAT_LOW         340

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Setting_Volume,
  E_Setting_Motor
} T_Setting;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "behavior";

static TaskHandle_t BehaviorTask = NULL;

static bool TaskIsStarted = false;

static uint16_t Behavior = 0u;

#define ENABLED(b)     (Behavior & b)    // ({behavior & b;})
#define ENABLE(b)      (Behavior |= b)   // do {behavior |= b;} while(0)
#define DISABLE(b)     (Behavior &= ~b)  // do {behavior &= ~b;} while(0)  //(behavior &= ~b)

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the behavior task
//! \pre       None
//! \param     None
//! \return    None
static void RunBehaviorTask(void* arg);

//! \brief     Run the behaviors
//! \pre       None
//! \param     None
//! \return    None
static void RunBehaviors(void);

//! \brief     Set the RC5 LED
//! \pre       None
//! \param     None
//! \return    None
static void SetRC5Led(void);

//! \brief     Set the buttons LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetButtonsLeds(void);

//! \brief     Set the accelerometer LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetAccelerometerLeds(void);

//! \brief     Set the gyroscope LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetGyroscopeLeds(void);

//! \brief     Update the settings
//! \pre       None
//! \param     None
//! \return    None
static void UpdateSettings(void);

//! \brief     Play the sound buttons
//! \pre       None
//! \param     None
//! \return    None
static void PlaySoundButtons(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_Init(void)
{
  TaskIsStarted = false;
  Behavior = 0u;
}

//_____________________________________________________________________________

void Behavior_Start(void)
{
  xTaskCreatePinnedToCore(
    RunBehaviorTask,  // Function to implement the task
    "behavior",       // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    4,                // Priority of the task
    &BehaviorTask,    // Task handle
    0);               // Core where the task should run

  TaskIsStarted = true;
}

//_____________________________________________________________________________

void Behavior_Stop(void)
{
  if (TaskIsStarted)
  {
    ESP_LOGW(Tag, "Behavior task is stopped");

    TaskIsStarted = false;
    vTaskDelete(BehaviorTask);
  }
}

//_____________________________________________________________________________

void Behavior_Enable(uint16_t b)
{
  ENABLE(b);
}

//_____________________________________________________________________________

void Behavior_Disable(uint16_t b)
{
  DISABLE(b);
}

//_____________________________________________________________________________

uint16_t Behavior_GetStatus(void)
{
  return Behavior;
}

//_____________________________________________________________________________

void Behavior_PlaySoundButtons(uint8_t button)
{
  if ((button == E_Button_Backward) ||
      (button == E_Button_Left)     ||
      (button == E_Button_Forward)  ||
      (button == E_Button_Right))
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }
  else if (button == E_Button_Center)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Blop);
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static void RunBehaviorTask(void* arg)
{
  ESP_LOGI(Tag, "Start Behavior Task");

  while (1)
  {
    RunBehaviors();
    vTaskDelay(40 / portTICK_PERIOD_MS);  // MAX_BRIGHTNESS = 16
  }
}

//_____________________________________________________________________________

static void RunBehaviors(void)
{
  if (ENABLED(B_LED_RC5))
  {
    SetRC5Led();
  }

  if (ENABLED(B_LEDS_BUTTON))
  {
    SetButtonsLeds();
  }

  if (ENABLED(B_SOUND_BUTTON))
  {
    PlaySoundButtons();
  }

  if (ENABLED(B_MODE))
  {
    Mode_Run();
  }

  if (ENABLED(B_LEDS_ACC))
  {
    SetAccelerometerLeds();
  }

  if (ENABLED(B_LEDS_LEGO))
  {
    SetGyroscopeLeds();
  }

//#if 0
  if (ENABLED(B_SETTING))
  {
    UpdateSettings();
  }
//#endif

  Gpio_ClearButtonStatus();
}

//_____________________________________________________________________________

static void SetRC5Led(void)
{
  if (RC5_IsFrameValid())
  {
    RC5_ClearFrameValidity();
    Leds_SetSingleBrightness(E_Led_RC5, MAX_BRIGHTNESS);
  }
  else
  {
    Leds_SetSingleBrightness(E_Led_RC5, 0u);
  }
}

//_____________________________________________________________________________

static void SetButtonsLeds(void)
{
  static uint8_t brightness[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
  uint8_t* buttonState = Buttons_GetStatus();

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
      brightness[index] = 0u;
    }
  }

  if (brightness[E_Button_Center] > 0u)
  {
    for (T_Led index = E_Led_Button_Forward; index <= E_Led_Button_Left; index++)
    {
      Leds_SetSingleBrightness(index, brightness[E_Button_Center]);
    }
  }
  else
  {
    if (brightness[E_Button_Backward] != 0u)
    {
      Leds_SetSingleBrightness(E_Led_Button_Backward, brightness[E_Button_Backward]);
    }

    if (brightness[E_Button_Left] != 0u)
    {
      Leds_SetSingleBrightness(E_Led_Button_Left, brightness[E_Button_Left]);
    }

    if (brightness[E_Button_Forward] != 0u)
    {
      Leds_SetSingleBrightness(E_Led_Button_Forward, brightness[E_Button_Forward]);
    }

    if (brightness[E_Button_Right] != 0u)
    {
      Leds_SetSingleBrightness(E_Led_Button_Right, brightness[E_Button_Right]);
    }
  }

  if ((brightness[E_Button_Backward] == 0u) &&
      (brightness[E_Button_Center] == 0u))
  {
    Leds_SetSingleBrightness(E_Led_Button_Backward, 0u);
  }

  if ((brightness[E_Button_Left] == 0u) &&
      (brightness[E_Button_Center] == 0u))
  {
    Leds_SetSingleBrightness(E_Led_Button_Left, 0u);
  }

  if ((brightness[E_Button_Forward] == 0u) &&
      (brightness[E_Button_Center] == 0u))
  {
    Leds_SetSingleBrightness(E_Led_Button_Forward, 0u);
  }

  if ((brightness[E_Button_Right] == 0u) &&
      (brightness[E_Button_Center] == 0u))
  {
    Leds_SetSingleBrightness(E_Led_Button_Right, 0u);
  }
}

//_____________________________________________________________________________

int16_t aseba_atan2(int16_t y, int16_t x); // We use a function which should be private to aseba native ...

static void SetAccelerometerLeds(void)
{
  static int16_t previousLed = 0;
  int16_t intensity = 0;
  int16_t led = -1;
  int16_t tilt = 0;

  T_Axis acc = Accelerometer_GetAcceleration();

  if (acc.Z < 16800)
  {
    tilt = (aseba_atan2(acc.Y, acc.X) / 2);

    if ((tilt >= -2000) && (tilt < 2000))
    {
      led = E_Led_Circle_N;
    }
    else if ((tilt < -2000) && (tilt >= -6000))
    {
      led = E_Led_Circle_NE;
    }
    else if ((tilt < -6000) && (tilt >= -10000))
    {
      led = E_Led_Circle_E;
    }
    else if ((tilt < -10000) && (tilt >= -14000))
    {
      led = E_Led_Circle_SE;
    }
    else if ((tilt < -14000) || (tilt >= 14000))
    {
      led = E_Led_Circle_S;
    }
    else if ((tilt < 6000) && (tilt >= 2000))
    {
      led = E_Led_Circle_NW;
    }
    else if ((tilt < 10000) && (tilt >= 6000))
    {
      led = E_Led_Circle_W;
    }
    else if ((tilt < 14000) && (tilt >= 10000))
    {
      led = E_Led_Circle_SW;
    }
    else
    {
      // Do nothing
    }

    intensity = (16 - (abs(acc.Z) >> 10));

    if ((intensity < 0) || ((abs(acc.X) + abs(acc.Y)) <= 2500))
    {
      intensity = 0;
    }

    if (led >= 0)
    {
      if (previousLed >= 0)
      {
        Leds_SetSingleBrightness(previousLed, 0u);
      }

      Leds_SetSingleBrightness(led, intensity);
    }

    previousLed = led;

  }
  else
  {
    if (previousLed >= 0)
    {
      Leds_SetSingleBrightness(previousLed, 0u);
    }

    previousLed = -1;
  }
}

//_____________________________________________________________________________

static void SetGyroscopeLeds(void)
{
  int16_t gyro = Gyroscope_GetAngularVelocityZ();

  // Turn left
  if (gyro >= 20000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }
  else if (gyro >= 10000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
  }
  else if (gyro >= 5000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u);
  }
  else if (gyro >= 2500)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }
  // Turn right
  else if (gyro <= -20000)
  {
    Leds_SetLegoFrontBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -10000)
  {
    Leds_SetLegoFrontBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -5000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -2500)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  }
}

//_____________________________________________________________________________

static void UpdateSettings(void)
{
  T_Setting setting = E_Setting_Motor;

  switch (setting)
  {
    case E_Setting_Volume:
      break;

    case E_Setting_Motor:
      //Leds_SetBodyBrightness(15u, MAX_BRIGHTNESS, 0u);
      //Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void PlaySoundButtons(void)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Backward] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }

  when(buttonState[E_Button_Left] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }

  when(buttonState[E_Button_Center] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Blop);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }

  when(buttonState[E_Button_Right] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }
}
