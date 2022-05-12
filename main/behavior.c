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
#include "color_sensor.h"
#include "common.h"
#include "gpio.h"
#include "gyroscope.h"
#include "leds.h"
#include "mode.h"
#include "rc5.h"
#include "settings.h"
#include "wifi_manager.h"

#include "aseba_esp32.h"  // TODO Add GetSpeed in common to remove this line

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BAT_HIGH        390
#define BAT_MIDDLE      360
#define BAT_LOW         340

#define SPEED_STEP      128

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Setting_Menu,
  E_Setting_Volume,
  E_Setting_Motor,
  E_Setting_Color,
  E_Reset_WiFi_Credentials,
  E_Setting_Max = E_Reset_WiFi_Credentials
};
typedef int16_t T_Setting;  // Setting selection

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

static T_Setting CurrentSetting = E_Setting_Menu;

static T_Settings Setting;

static bool IsColorCalibrationInProgress = false;

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

//! \brief     Play the sound buttons
//! \pre       None
//! \param     None
//! \return    None
static void PlaySoundButtons(void);

//! \brief     Update the settings
//! \pre       None
//! \param     None
//! \return    None
static void UpdateSettings(void);

static T_Setting SelectNextSetting(T_Setting setting, int16_t index);

static bool IsSettingEnabled(T_Setting setting);

static void SetSettingColor(T_Setting setting);

static void ExitSetting(T_Setting setting);

static void AdjustVolume(void);

static void TuneMotors(void);

static void CalibrateColor(void);

static void RunLegoLedAnimation(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_Init(void)
{
  Setting.Volume = Settings_ReadVolume();

  Setting.LeftMotor = Settings_ReadLeftMotor();
  Setting.RightMotor = Settings_ReadRightMotor();

  TaskIsStarted = false;
  Behavior = 0u;

  Codec_SetVolume(Setting.Volume);
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
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Tick);
  }
  else if (button == E_Button_Center)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Blop);
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

void Behavior_PlaySoundAlarm(uint8_t type)
{
  static bool playSound = true;

  if (playSound)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Alarm);
  }

  if (type == 0u)//E_AlarmType_Once)
  {
    playSound = false;
  }
  else if (type == 1u)//E_AlarmType_Continuous)
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

  if (ENABLED(B_SETTING))
  {
    UpdateSettings();
  }

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

static void PlaySoundButtons(void)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Backward] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Tick);
  }

  when(buttonState[E_Button_Left] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Tick);
  }

  when(buttonState[E_Button_Center] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Blop);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Tick);
  }

  when(buttonState[E_Button_Right] != 0u)
  {
    Codec_PlayMP3FileFromFlash(E_SoundIndex_Tick);
  }
}

//_____________________________________________________________________________

static void UpdateSettings(void)
{
  static uint8_t count = 0u;
  static uint8_t select = 0u;
  static bool start = false;

  uint8_t* buttonState = Buttons_GetStatus();
  bool sideState = Gpio_IsButtonPressed();

  if (start)
  {
    //RunLegoLedAnimation();

    // Enter into a setting
    when(buttonState[E_Button_Center])
    {
      if (select != CurrentSetting)
      {
        CurrentSetting = select;
      }
    }

    // Exit from a setting
    when(sideState)
    {
      ExitSetting(CurrentSetting);

      if (select == CurrentSetting)
      {
        CurrentSetting = E_Setting_Menu;
      }
    }

    switch (CurrentSetting)
    {
      case E_Setting_Menu:
        when(buttonState[E_Button_Backward])
        {
          select = SelectNextSetting(select, -1);
        }

        when(buttonState[E_Button_Left])
        {
          select = SelectNextSetting(select, -1);
        }

        when(buttonState[E_Button_Forward])
        {
          select = SelectNextSetting(select, 1);
        }

        when(buttonState[E_Button_Right])
        {
          select = SelectNextSetting(select, 1);
        }

        SetSettingColor(select);
        break;

      case E_Setting_Volume:  // Orange
        AdjustVolume();
        break;

      case E_Setting_Motor:   // Green-Yellow
        TuneMotors();
        break;

      case E_Setting_Color:   // Purple
        CalibrateColor();
        break;

      case E_Reset_WiFi_Credentials:
    	  wifi_manager_delete_sta_config();
    	  break;

      default:
        // Do nothing
        break;
    }
  }
  else if (buttonState[E_Button_Left] && buttonState[E_Button_Right])
  {
    count++;

    if (count > 75)  // 75 * 40 [ms] = 3 [s]
    {
      Behavior_Disable(B_MODE);
      count = 0u;
      start = true;
      CurrentSetting = E_Setting_Menu;
    }
  }
}

//_____________________________________________________________________________

static T_Setting SelectNextSetting(T_Setting setting, int16_t index)
{
  int16_t temp = (int16_t)setting;

  do
  {
    temp += index;

    while (temp > E_Setting_Max)
    {
      temp -= (E_Setting_Max + 1);
    }

    while (temp < 0)
    {
      temp += (E_Setting_Max + 1);
    }
  }
  while (!IsSettingEnabled(temp));

  return (T_Setting)temp;
}

//_____________________________________________________________________________

static bool IsSettingEnabled(T_Setting setting)
{
  bool result = true;

  if (setting == E_Setting_Menu)
  {
    result = false;
  }

  return result;
}

//_____________________________________________________________________________

static void SetSettingColor(T_Setting setting)
{
  switch (setting)
  {
    case E_Setting_Menu:
      Leds_SetBodyBrightness(0u, 0u, 0u);
      break;

    case E_Setting_Volume:  // Orange
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, (MAX_BRIGHTNESS / 2u), 0u);
      break;

    case E_Setting_Motor:  // Green-Yellow
      Leds_SetBodyBrightness((MAX_BRIGHTNESS / 2u), MAX_BRIGHTNESS, 0u);
      break;

    case E_Setting_Color:  // Purple
      Leds_SetBodyBrightness((MAX_BRIGHTNESS / 2u), 0u, MAX_BRIGHTNESS);
      break;

    case E_Reset_WiFi_Credentials: // Dark red
    	Leds_SetBodyBrightness((MAX_BRIGHTNESS / 4u), 0u, 0u);
    	break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void ExitSetting(T_Setting setting)
{
  Leds_SetBodyBrightness(0u, 0u, 0u);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

  switch (setting)
  {
    case E_Setting_Menu:

      break;

    case E_Setting_Volume:
      // Write to the settings file
      Settings_WriteVolume(Setting.Volume);

      ESP_LOGE(Tag, "Write volume: %d", Setting.Volume);
      break;

    case E_Setting_Motor:
      Common_SetTargetSpeed(0, 0);

      // Write to the settings file
      Settings_WriteLeftMotor(Setting.LeftMotor);
      Settings_WriteRightMotor(Setting.RightMotor);

      ESP_LOGE(Tag, "Write motor: %d %d", Setting.LeftMotor, Setting.RightMotor);
      break;

    case E_Setting_Color:
      IsColorCalibrationInProgress = false;
      break;

    case E_Reset_WiFi_Credentials:
    	break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void AdjustVolume(void)
{
  uint8_t* buttonState = Buttons_GetStatus();

  uint8_t brightness = Common_GetBodyColorPulse();

  // Orange pulse
  Leds_SetBodyBrightness(brightness, (brightness / 2u), 0u);

  when(buttonState[E_Button_Backward] != 0u)
  {
    Setting.Volume -= 8;
    //set_save_settings();
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Setting.Volume += 8;
    //set_save_settings();
  }

  if (Setting.Volume < 40)
  {
    Setting.Volume = 40;
  }
  else if (Setting.Volume > 100)
  {
    Setting.Volume = 100;
  }

  //ESP_LOGE(Tag, "volume: %d", Volume);

  //settings.sound_shift = volume;
  Codec_SetVolume(Setting.Volume);

  if (Setting.Volume <= 40)
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 48)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 56)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 64)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 72)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 80)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }
  else if (Setting.Volume <= 88)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS,
                             0u, 0u);
  }
  else if (Setting.Volume <= 96)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS,
                             MAX_BRIGHTNESS, 0u);
  }
  else if (Setting.Volume <= 104)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS,
                             MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }

#if 0
  int led_circle[8];

  for (uint8_t index = 0u; index < 8u; index++)
  {
    led_circle[index] = 2 * (1 + index - volume);

    if (index == 0u)
    {
      ESP_LOGE(Tag, "brightness: %d", led_circle[0]);
    }

    if (led_circle[index] < 0)
    {
      led_circle[index] = 0;
    }
  }

  Leds_SetCircleBrightness(led_circle[7], led_circle[6], led_circle[5], led_circle[4], led_circle[3], led_circle[2],
                           led_circle[1], led_circle[0]);

  when(buttonState[E_Button_Center] != 0u) // && !dbnc)
  {
    CurrentSetting = E_Setting_Menu;
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

    // Write to the settings file
    Settings_WriteVolume(volume);
  }
#endif
}

//_____________________________________________________________________________

static void TuneMotors(void)
{
  static int16_t correction = 0;
  uint8_t* buttonState = Buttons_GetStatus();

  uint8_t brightness = Common_GetBodyColorPulse();

  // Green-Yellow pulse
  Leds_SetBodyBrightness((brightness / 2u), brightness, 0u);

  if (vmVariables.target[0] != 0)
  {
    when(buttonState[E_Button_Right])
    {
      correction += 1;
      //set_save_settings();
    }

    when(buttonState[E_Button_Left])
    {
      correction -= 1;
      //set_save_settings();
    }

    if (correction >= 0)
    {
      if (correction > 15)
      {
        Leds_SetCircleBrightness(0, 15, correction - 15, 0, 0, 0, 0, 0);

        if (correction > 50)  // Bound correction
        {
          correction = 50;
        }
      }
      else
      {
        Leds_SetCircleBrightness(0, correction, 0, 0, 0, 0, 0, 0);
      }
    }
    else
    {
      if (correction < -15)
      {
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, (-correction - 15), 15);

        if (correction < -50)
        {
          correction = -50;
        }
      }
      else
      {
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, -correction);
      }
    }

    Setting.LeftMotor  = (256 + correction);
    Setting.RightMotor = (256 - correction);

    Settings_SetLeftMotorSettings(Setting.LeftMotor);    // Used to send the value to STM32
    Settings_SetRightMotorSettings(Setting.RightMotor);  // Used to send the value to STM32
  }

  when(buttonState[E_Button_Backward])
  {
    Common_IncrementTargetSpeed(-SPEED_STEP, -SPEED_STEP);

    if (vmVariables.target[0] <= (-3 * SPEED_STEP))
    {
      Common_SetTargetSpeed((-3 * SPEED_STEP), (-3 * SPEED_STEP));
    }
  }

  when(buttonState[E_Button_Forward])
  {
    Common_IncrementTargetSpeed(SPEED_STEP, SPEED_STEP);

    if (vmVariables.target[0] >= 3 * SPEED_STEP)
    {
      Common_SetTargetSpeed((3 * SPEED_STEP), (3 * SPEED_STEP));
    }
  }
}

//_____________________________________________________________________________

static void CalibrateColor(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  uint8_t brightness = Common_GetBodyColorPulse();
  uint8_t calibrationStatus = false;

  if (!IsColorCalibrationInProgress)  // Pulse the four body LEDs
  {
    // Purple pulse
    Leds_SetBodyBrightness((brightness / 2u), 0u, brightness);
  }
  else  // Pulse only the back body LEDs
  {
    Leds_SetBackLeftBrightness((brightness / 2u), 0u, brightness);
    Leds_SetBackRightBrightness((brightness / 2u), 0u, brightness);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    if (ColorSensor_Calibrate(0, &calibrationStatus))  // White calibration
    {
      Codec_PlayMP3FileFromFlash(E_SoundIndex_Good);
    }
    else
    {
      Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
    }

    if (calibrationStatus == 2)
    {
      Leds_SetLegoFrontBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS);
    }
    else if (calibrationStatus == 1)
    {
      Leds_SetLegoFrontBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u);
    }
    else
    {
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u);
    }

    IsColorCalibrationInProgress = true;

    Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
    Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }

  when(buttonState[E_Button_Backward] != 0u)
  {
    if (ColorSensor_Calibrate(1, &calibrationStatus))  // Black calibration
    {
      Codec_PlayMP3FileFromFlash(E_SoundIndex_Good);
    }
    else
    {
      Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
    }

    if (calibrationStatus == 2)  // Successful calibration
    {
      Leds_SetLegoFrontBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS);
    }
    else if (calibrationStatus == 1)  // Partial calibration
    {
      Leds_SetLegoFrontBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u);
    }
    else  // Bad calibration
    {
      Leds_SetLegoFrontBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u);
    }

    IsColorCalibrationInProgress = true;

    Leds_SetFrontLeftBrightness(0u, 0u, 0u);
    Leds_SetFrontRightBrightness(0u, 0u, 0u);
  }
}

//_____________________________________________________________________________

static void RunLegoLedAnimation(void)
{
  static uint8_t led_state;
  uint8_t l[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t fixed;

  led_state += 2;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7] = MAX_BRIGHTNESS;
  l[(fixed - 2) & 0x7] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1)));
  l[(fixed + 2) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));
  l[(fixed + 4) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));

  Leds_SetLegoFrontBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
  //Leds_SetLegoBackBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);

#if 0
  uint8_t l[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  static uint8_t count = 0u;

  if ((count == 0u) || (count == 14u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = 0u;
    l[2] = 0u;
    l[3] = 0u;
    l[4] = 0u;
    l[5] = 0u;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 1u) || (count == 13u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = 0u;
    l[3] = 0u;
    l[4] = 0u;
    l[5] = 0u;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 2u) || (count == 12u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = 0u;
    l[4] = 0u;
    l[5] = 0u;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 3u) || (count == 11u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = MAX_BRIGHTNESS;
    l[4] = 0u;
    l[5] = 0u;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 4u) || (count == 10u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = MAX_BRIGHTNESS;
    l[4] = MAX_BRIGHTNESS;
    l[5] = 0u;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 5u) || (count == 9u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = MAX_BRIGHTNESS;
    l[4] = MAX_BRIGHTNESS;
    l[5] = MAX_BRIGHTNESS;
    l[6] = 0u;
    l[7] = 0u;
  }
  else if ((count == 6u) || (count == 8u))
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = MAX_BRIGHTNESS;
    l[4] = MAX_BRIGHTNESS;
    l[5] = MAX_BRIGHTNESS;
    l[6] = MAX_BRIGHTNESS;
    l[7] = 0u;
  }
  else if (count == 7u)
  {
    l[0] = MAX_BRIGHTNESS;
    l[1] = MAX_BRIGHTNESS;
    l[2] = MAX_BRIGHTNESS;
    l[3] = MAX_BRIGHTNESS;
    l[4] = MAX_BRIGHTNESS;
    l[5] = MAX_BRIGHTNESS;
    l[6] = MAX_BRIGHTNESS;
    l[7] = MAX_BRIGHTNESS;
  }

  count++;

  if (count == 15u)
  {
    count = 0u;
  }

  Leds_SetLegoFrontBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
  Leds_SetLegoBackBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
#endif
}
