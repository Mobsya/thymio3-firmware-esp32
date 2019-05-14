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

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "behavior.h"

#include "aseba_esp32.h"
#include "ground_ir.h"
#include "leds.h"
#include "mode.h"
#include "prox_ir.h"
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

//! \brief     Set the proximity IR sensors LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetProxIRSensorsLeds(void);

//! \brief     Set the ground IR sensors LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetGroundIRSensorsLeds(void);

//! \brief     Set the accelerometer LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetAccelerometerLeds(void);

//! \brief     Set the battery LEDs
//! \pre       None
//! \param     None
//! \return    None
static void SetBatteryLeds(void);

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
    3,                // Priority of the task
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
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

static void RunBehaviors(void)
{
#if 0
  if (ENABLED(B_LEDS_BATTERY))
  {
    SetBatteryLeds();
  }
#endif
  if (ENABLED(B_LEDS_BUTTON))
  {
    SetButtonsLeds();
  }

  if (ENABLED(B_LEDS_PROX))
  {
    SetProxIRSensorsLeds();
    SetGroundIRSensorsLeds();
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
  static uint8_t brightness[BUTTONS_NUM] = {MIN_BRIGHTNESS, MIN_BRIGHTNESS, MIN_BRIGHTNESS,
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

void SetProxIRSensorsLeds(void)
{
  static int16_t max[PROX_IR_SENSORS_NUM] = {4200, 4200, 4200, 4200, 4200, 4200, 4200};
  static int16_t min[PROX_IR_SENSORS_NUM] = {1000, 1000, 1000, 1000, 1000, 1000, 100};

  static T_Led led[PROX_IR_SENSORS_NUM] = {E_Led_Front_IR_0, E_Led_Front_IR_1, E_Led_Front_IR_2A,
                                           E_Led_Front_IR_3, E_Led_Front_IR_4, E_Led_IR_Back_Left,
                                           E_Led_IR_Back_Right
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

    if ((vmVariables.prox[index] != 0) && (min[index] > vmVariables.prox[index]))
    {
      min[index] = vmVariables.prox[index];
    }
  }

  // Do a linear transformation from min-max to led 0-31!
  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
    // Because of the min&max calculation above, we cannot have a
    // Division by 0 here.
    s = vmVariables.prox[index] - min[index];
    delta = (max[index] - min[index]);

    if (s < 0)
    {
      s = 0;
    }

    brightness = ((int32_t)s * MAX_BRIGHTNESS) / delta;

    Leds_SetSingleBrightness(led[index], brightness);

    // The Front IR sensor has 2 LEDs (E_Led_Front_IR_2A and E_Led_Front_IR_2B)
    if (index == 2u)
    {
      Leds_SetSingleBrightness(led[index] + 1, brightness);
    }
  }
}

//_____________________________________________________________________________

void SetGroundIRSensorsLeds(void)
{
  static int16_t max[GROUND_IR_SENSORS_NUM] = {900, 900};

  static T_Led led[GROUND_IR_SENSORS_NUM] = {E_Led_Ground_IR_0, E_Led_Ground_IR_1};

  int16_t s = 0;
  int16_t brightness = 0;

  for (uint8_t index = 0u; index < GROUND_IR_SENSORS_NUM; index++)
  {
    if (max[index] < vmVariables.ground_delta[index])
    {
      max[index] = vmVariables.ground_delta[index];
      // min is fixed to 0 ... this is _physical_
    }
  }

  for (uint8_t index = 0u; index < GROUND_IR_SENSORS_NUM; index++)
  {
    s = (vmVariables.ground_delta[index] > 0) ? vmVariables.ground_delta[index] : 0;
    brightness = ((int32_t)s * MAX_BRIGHTNESS) / max[index];

    Leds_SetSingleBrightness(led[index], brightness);
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

static void SetBatteryLeds(void)
{
  static uint8_t counter = 0u;
  static bool wasCharging = false;

  int16_t vbat = STM32_GetBatteryVoltage();

  if (STM32_IsUSBCablePresent())
  {
    static uint8_t state = 0u;

#if 0
    int16_t v = STM32_GetBatteryVoltage();
    int32_t temp = v * 1000;
    v = (temp / 3978);

    ESP_LOGE(Tag, "v: %d", v);
#endif

    if (!wasCharging)
    {
      // switch off everything.
      Leds_SetSingleBrightness(E_Led_Battery_0, 0u);
      Leds_SetSingleBrightness(E_Led_Battery_1, 0u);
      Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
      wasCharging = true;
    }

    // On 5V
    //int i = counter ? counter : 1;
    //counter += i > 10 ? 7 : i/2 + 1;
    int16_t i;

    if (counter >= 1u)
    {
      i = counter;
    }
    else
    {
      i = 1;
    }

    if (i > 10)
    {
      counter += 7;
    }
    else
    {
      counter += ((i / 2) + 1);
    }

    if (counter > 100u)
    {
      state++;
      counter = 1u;

      if (state == 3u)
      {
        state = 0u;
      }

      switch (state)
      {
        case 0:
          Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
          Leds_SetSingleBrightness(E_Led_Battery_1, 0u);
          break;
      }
    }

    //ESP_LOGE(Tag, "i: %d, counter: %d, state: %d", i, counter, state);

    Leds_SetSingleBrightness(E_Led_Battery_0 + state, counter);
  }
  else
  {
    vbat = STM32_GetBatteryVoltage();
    int32_t temp = vbat * 1000;
    vbat = (temp / 3978);

    if (wasCharging)
    {
      wasCharging = false;

      if (vbat >= BAT_HIGH)
      {
        Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
        Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
        Leds_SetSingleBrightness(E_Led_Battery_2, MAX_BRIGHTNESS);
      }
      else if (vbat > BAT_MIDDLE)
      {
        Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
        Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
        Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
      }
      else if (vbat > BAT_LOW)
      {
        Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
        Leds_SetSingleBrightness(E_Led_Battery_1, 0u);
        Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
      }
      else
      {
        // Do nothing
      }
    }

    when(vbat >= BAT_HIGH)
    {
      Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
      Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
      Leds_SetSingleBrightness(E_Led_Battery_2, MAX_BRIGHTNESS);
    }

    when((vbat > BAT_MIDDLE) && (vbat < (BAT_HIGH - 5)))
    {
      Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
      Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
      Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
    }

    when((vbat > BAT_LOW) && (vbat <= (BAT_MIDDLE - 5)))
    {
      Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
      Leds_SetSingleBrightness(E_Led_Battery_1, 0u);
      Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
    }

    when(vbat <= BAT_LOW)
    {
      Leds_SetSingleBrightness(E_Led_Battery_1, 0u);
      Leds_SetSingleBrightness(E_Led_Battery_2, 0u);
    }


    if (vbat <= BAT_LOW)
    {
      counter++;

      if (counter == 3u)
      {
        Leds_SetSingleBrightness(E_Led_Battery_0, MAX_BRIGHTNESS);
      }

      if (counter > 5u)
      {
        Leds_SetSingleBrightness(E_Led_Battery_0, 0u);
        counter = 0u;
      }
    }
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

  buttonState = STM32_GetButtonStatus();

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
