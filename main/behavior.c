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

#include "aseba_esp32.h"
#include "board.h"
#include "buttons.h"
#include "leds.h"
#include "mode.h"
#include "sound.h"
#include "stm32.h"

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

static uint16_t behavior = 0u;

#define ENABLED(b)     (behavior & b)    // ({behavior & b;})
#define ENABLE(b)      (behavior |= b)   // do {behavior |= b;} while(0)
#define DISABLE(b)     (behavior &= ~b)  // do {behavior &= ~b;} while(0)  //(behavior &= ~b)

const T_Note ButtonSound[1];
const T_Note CenterButtonSound[3];

static T_Melody MelodyButton;
static T_Melody MelodyCenterButton;

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

//! \brief     Update the settings
//! \pre       None
//! \param     None
//! \return    None
static void UpdateSettings(void);

static void PlaySoundButtons(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_Init(void)
{
  MelodyButton.Melody = ButtonSound;
  MelodyButton.Tempo  = E_Tempo_Allegro;
  MelodyButton.Loop   = 1;
  MelodyButton.Size   = 1;

  MelodyCenterButton.Melody = CenterButtonSound;
  MelodyCenterButton.Tempo  = E_Tempo_Allegro;
  MelodyCenterButton.Loop   = 1;
  MelodyCenterButton.Size   = 3;
}

//_____________________________________________________________________________

void Behavior_Start(void)
{
  xTaskCreatePinnedToCore(
    RunBehaviorTask,  // Function to implement the task
    "behavior",       // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    2,                // Priority of the task
    NULL,             // Task handle
    0);               // Core where the task should run
}

//_____________________________________________________________________________

void Behavior_Enable(uint16_t b)
{
  //ENABLE(b);

  behavior |= b;
}

//_____________________________________________________________________________

void Behavior_Disable(uint16_t b)
{
  DISABLE(b);

  //behavior &= ~b;
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
  if (ENABLED(B_LEDS_BUTTON))
  {
    SetButtonsLeds();
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

#if 0  // FIXME
  if (ENABLED(B_SOUND_BUTTON))
  {
    PlaySoundButtons();
  }
#endif

//#if 0
  if (ENABLED(B_SETTING))
  {
    UpdateSettings();
  }
//#endif
}

//_____________________________________________________________________________

static void SetButtonsLeds(void)
{
  static uint8_t brightness[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

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
  static int previous_led;

  int intensity;
  int led = -1;

  // FIXME: Use vmVariables ?!
  if (vmVariables.acc[2] < 16800)  // 21
  {
    int ha = (aseba_atan2(vmVariables.acc[0], vmVariables.acc[1]) / 2);

    //printf("z = %d\n", vmVariables.acc[2]);
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

    //intensity = (40 - (abs(vmVariables.acc[2]) * 2));  // TODO
    intensity = MAX_BRIGHTNESS;

    if (intensity < 0)
    {
      intensity = 0;
    }

    if (led >= 0)
    {
      if (previous_led >= 0)
      {
        Leds_SetSingleBrightness(previous_led, 0u);
      }

      Leds_SetSingleBrightness(led, intensity);
    }

    previous_led = led;

  }
  else
  {
    if (previous_led >= 0)
    {
      Leds_SetSingleBrightness(previous_led, 0u);
    }

    previous_led = -1;
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
    Sound_StartPlayer(&MelodyButton);
  }

  when(buttonState[E_Button_Left] != 0u)
  {
    Sound_StartPlayer(&MelodyButton);
  }

  when(buttonState[E_Button_Center] != 0u)
  {
    Sound_StartPlayer(&MelodyCenterButton);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Sound_StartPlayer(&MelodyButton);
  }

  when(buttonState[E_Button_Right] != 0u)
  {
    Sound_StartPlayer(&MelodyButton);
  }
}
