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
//! \author  Vincent Gonet, Stefano Morgani
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
#include "angle_controller.h"
#include "mp_component.h"
#include "python_handler.h"
#include "aseba_esp32.h"  // TODO Add GetSpeed in common to remove this line
#include "gyroscope.h"
#include "stm32_spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BAT_HIGH        390
#define BAT_MIDDLE      360
#define BAT_LOW         340

#define SPEED_STEP      128

#define GROUND_IR_THRESHOLD    280
#define MOVEMENT_SPEED 300

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
  E_Setting_Gyro,
  E_Setting_MotorFwBw,
  E_Setting_Max = E_Setting_MotorFwBw
};
typedef int16_t T_Setting;  // Setting selection

enum
{
  E_Python_Menu,
  E_Python_REPL,
  E_Python_Main1,
  E_Python_Main2,
  E_Python_Main3,
  E_Python_Main4,
  E_Python_Main5,
  E_Python_Main6,
  E_Python_Main7,
  E_Python_Max = E_Python_Main7
};
typedef int16_t T_Python_item;  // Python script selection

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
static T_Python_item CurrentPythonItem = E_Python_Menu;

static T_Settings Setting;

static bool IsColorCalibrationInProgress = false;

static bool start_settings_menu = false;
static bool start_python_menu = false;

static uint8_t updateSettingsState = 0;

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

static void CalibrateGyro(void);

static void CalibrateMotFwBw(void);

static void UpdatePythonMenu(void);

static T_Python_item SelectNextPythonItem(T_Python_item item, int16_t index);

static bool IsPythonItemEnabled(T_Python_item item);

static void SetPythonItemColor(T_Python_item item);

static void ExitPythonItem(T_Python_item item);

static void RunLegoLedAnimation(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Behavior_Init(void)
{
  Setting.Volume = Settings_GetVolumeSettings();

  Setting.LeftMotor = Settings_GetLeftMotorSettings();
  Setting.RightMotor = Settings_GetRightMotorSettings();

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
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_TICK);
  }
  else if (button == E_Button_Center)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_BLOP);
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
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_ALARM);
  }

  if (type == 0u)//E_AlarmType_Once)
  {
    playSound = false;
  }
  else if (type == 1u)//E_AlarmType_Continuous)
  {
    if (Codec_IsSoundFinished())
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
  int64_t time_start, time_end;

  while (1)
  {
    time_start = esp_timer_get_time();
    RunBehaviors();
    AngleController_Update();
		time_end = esp_timer_get_time();
		//printf("%lld usec\n", time_end - time_start);
		if((time_end - time_start) < 20000) { // Run behavior task @ 50 Hz like in T2
			vTaskDelay((20000 - (time_end - time_start))/1000 / portTICK_PERIOD_MS);
		}
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

  if (ENABLED(B_LEDS_LEGO_GYRO))
  {
    SetGyroscopeLeds();
  }

  if (ENABLED(B_SETTING))
  {
    UpdateSettings();
  }

  if (ENABLED(B_LEDS_LEGO_KITT))
  {
    RunLegoLedAnimation();
  }  

  if (ENABLED(B_PYTHON))
  {
    UpdatePythonMenu();
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
  uint8_t ind = 0;

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
      /*
      if (previousLed >= 0)
      {
        Leds_SetSingleBrightness(previousLed, 0u);
      }

      Leds_SetSingleBrightness(led, intensity);
      */

      for(ind=E_Led_Circle_N; ind<=E_Led_Circle_NW; ind++)
      {
        if(ind == led)
        {           
          Leds_SetSingleBrightness(ind, ((4-intensity)<0)?0:(4-intensity));
        }
        else
        {
          Leds_SetSingleBrightness(ind, MAX_BRIGHTNESS);
        }
      }

    }

    previousLed = led;

  }
  else
  {
    /*
    if (previousLed >= 0)
    {
      Leds_SetSingleBrightness(previousLed, 0u);
    }
    */
    previousLed = -1;

    for(ind=E_Led_Circle_N; ind<=E_Led_Circle_NW; ind++)
    {
      Leds_SetSingleBrightness(ind, MAX_BRIGHTNESS);
    }
  }
}

//_____________________________________________________________________________

static void SetGyroscopeLeds(void)
{
  int16_t gyro = Gyroscope_GetAngularVelocityZ();

  // Turn left
  if (gyro >= 9000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
  }
  else if (gyro >= 6000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
  }
  else if (gyro >= 3000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u);
  }
  else if (gyro >= 1500)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }
  // Turn right
  else if (gyro <= -9000)
  {
    Leds_SetLegoFrontBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -6000)
  {
    Leds_SetLegoFrontBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -3000)
  {
    Leds_SetLegoFrontBrightness(0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (gyro <= -1500)
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
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_TICK);
  }

  when(buttonState[E_Button_Left] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_TICK);
  }

  when(buttonState[E_Button_Center] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_BLOP);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_TICK);
  }

  when(buttonState[E_Button_Right] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_TICK);
  }
}

//_____________________________________________________________________________

static void UpdateSettings(void)
{
  static uint8_t count = 0u;
  static uint8_t select = 0u;
  static uint8_t settings_navigation_state = RUNNING_MENU;
  static uint8_t delayCount = 0;

  if(start_python_menu) { // Settings menu can be accessed only from the main menu
    return;
  }

  uint8_t* buttonState = Buttons_GetStatus();

  if (start_settings_menu)
  {
    //RunLegoLedAnimation();
    delayCount++; // Based on 50 Hz behaviors update rate
    if(delayCount == 25)
    {
      Leds_SetLegoFrontOdd(MAX_BRIGHTNESS/2);
    } 
    else if(delayCount == 50)
    {
      delayCount = 0;
      Leds_SetLegoFrontEven(MAX_BRIGHTNESS/2);
    }

    // Handle settings entering/exiting with the center button
    when(buttonState[E_Button_Center])
    {
      if(settings_navigation_state == RUNNING_MENU) {
        settings_navigation_state = RUNNING_BEHAVIOR;
        if (select != CurrentSetting) {
          CurrentSetting = select;
        }        
      } else {
        settings_navigation_state = RUNNING_MENU;
        ExitSetting(CurrentSetting);
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

      case E_Setting_Gyro:
        CalibrateGyro();
        break;

      case E_Setting_MotorFwBw:
        CalibrateMotFwBw();
        break;

      default:
        // Do nothing
        break;
    }

    if ((CurrentSetting==E_Setting_Menu) && (buttonState[E_Button_Left] && buttonState[E_Button_Right]))
    {
      count++;

      if (count > 75)  // 75 * 40 [ms] = 3 [s]
      {
        Behavior_Enable(B_MODE);
        count = 0u;
        start_settings_menu = false;
        CurrentSetting = E_Setting_Menu;
        Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
        Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);        
      }
    } else {
      count = 0;
    }

  } else { // If settings menu not entered
    // When in the "investigator mode" (line tracking) the left+right are used to calibrate the ground sensors in a white surface,
    // thus avoid to enter the settings menu while trying to calibrate on the white surface.
    // Also in the "obedient mode" the left+right combination is used (to stop the rotation), thus avoid entering the settings menu in this case.
    // The drawback is that you cannot enter the settings menu when the investigator mode is running, but this is not a big problem...
    if((Mode_get_current() != E_Mode_Investigator) && (Mode_get_current() != E_Mode_Obedient)) {
      if (buttonState[E_Button_Left] && buttonState[E_Button_Right]) {
        count++;
        if (count > 75)  // 75 * 40 [ms] = 3 [s]
        {
          Behavior_Disable(B_MODE);
          count = 0u;
          start_settings_menu = true;
          CurrentSetting = E_Setting_Menu;
          Leds_SetLegoFrontEven(MAX_BRIGHTNESS/2);
          Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
        }
      } else {
        count = 0;
      }
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
  updateSettingsState = 0;
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
      Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, (MAX_BRIGHTNESS / 2u), 0u);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, 0, 0);
      Leds_SetBackRightBrightness(0, 0, 0);
      break;

    case E_Setting_Motor:  // Green-Yellow
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness((MAX_BRIGHTNESS / 2u), MAX_BRIGHTNESS, 0u);
      Leds_SetBackLeftBrightness(0, 0, 0);
      Leds_SetBackRightBrightness(0, 0, 0);
      break;

    case E_Setting_Color:  // Purple
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness((MAX_BRIGHTNESS / 2u), 0u, MAX_BRIGHTNESS);
      Leds_SetBackRightBrightness(0, 0, 0);
      break;

    case E_Reset_WiFi_Credentials: // Dark red
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, 0, 0);
    	Leds_SetBackRightBrightness((MAX_BRIGHTNESS / 4u), 0u, 0u);
    	break;

    case E_Setting_Gyro: // Dark green
      Leds_SetFrontLeftBrightness(0, (MAX_BRIGHTNESS / 4u), 0u);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, 0, 0);
      Leds_SetBackRightBrightness(0, 0, 0);
      break;

    case E_Setting_MotorFwBw: // Dark blue
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, (MAX_BRIGHTNESS / 4u));
      Leds_SetBackLeftBrightness(0, 0, 0);
      Leds_SetBackRightBrightness(0, 0, 0);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void ExitSetting(T_Setting setting)
{
  static int16_t values[3];
  Leds_SetBodyBrightness(0u, 0u, 0u);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  //Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

  switch (setting)
  {
    case E_Setting_Menu:

      break;

    case E_Setting_Volume:
      // Write to the settings file
      Settings_WriteVolume(Setting.Volume);
      Settings_SetVolumeSettings(Setting.Volume);
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

    case E_Setting_Gyro: // Dark green      
      Gyroscope_GetCalibration(values);
      Settings_WriteZeroOffGyro(values);
      ESP_LOGI(Tag, "Write zero gyro: %d, %d, %d", values[0], values[1], values[2]);
      break;

    case E_Setting_MotorFwBw: // Dark blue
      Settings_WriteMotFwBwFactor(Setting.MotFwBw);
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
  Leds_SetFrontLeftBrightness(brightness, (brightness / 2u), 0u);

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
  Leds_SetFrontRightBrightness((brightness / 2u), brightness, 0u);

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

  // Purple pulse
  Leds_SetBackLeftBrightness((brightness / 2u), 0u, brightness);

  when(buttonState[E_Button_Forward] != 0u)
  {
    ColorSensor_CalibrateWhite();
  }

  when(buttonState[E_Button_Backward] != 0u)
  {
    ColorSensor_CalibrateBlack();
  }
  
}

//_____________________________________________________________________________

static void CalibrateGyro(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  uint8_t brightness = Common_GetBodyColorPulse();
  static uint8_t delayCount = 0;

  // Dark green pulse
  Leds_SetFrontLeftBrightness(0, (brightness / 4u), 0u);

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start calibration
      when(buttonState[E_Button_Forward] != 0u)
      {
        updateSettingsState = 1;
        delayCount = 0;
      }
      break;
    
    case 1: // Wait a bit before actually taking samples in order to avoid interference from the button press and in the meantime show led animation
      delayCount++;
      Leds_SetLegoBackProgress(delayCount/6);
      if(delayCount == 50) // Based on 50 Hz behaviors update rate
      {
        Gyroscope_DisableContinuousCalib();
        Gyroscope_Calibrate();
        updateSettingsState = 2;
      }
      break;

    case 2: // Wait for the center button press in order to save the calibration
      break;

  }
}

//_____________________________________________________________________________

static void CalibrateMotFwBw(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  uint8_t brightness = Common_GetBodyColorPulse();
  static uint8_t delayCount = 0;
  static uint8_t fwCount = 0;
  static uint8_t bwCount = 0;

  // Dark blue pulse
  Leds_SetFrontRightBrightness(0, 0, (brightness / 4u));  

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start calibration
      when(buttonState[E_Button_Forward] != 0u)
      {
        updateSettingsState = 1;
        Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED); 
        delayCount = 0;
      }
      break;
    
    case 1: // Go forward, the robot should be able to detect the first black line to start the time needed to travel to the second black line
      if ((GetGroundValue(0) < GROUND_IR_THRESHOLD) && (GetGroundValue(0) < GROUND_IR_THRESHOLD))
      {
        delayCount++;
        if(delayCount >= 3)
        {
          delayCount = 0;
          updateSettingsState = 2;
          fwCount = 0;
        }
      } else {
        delayCount = 0;
      }
      break;

    case 2: // Wait for the center button press in order to save the calibration
      fwCount++;
      if ((GetGroundValue(0) < GROUND_IR_THRESHOLD) && (GetGroundValue(0) < GROUND_IR_THRESHOLD))
      {
        delayCount++;
        if(delayCount >= 3)
        {
          Common_SetTargetSpeed(0, 0);
          updateSettingsState = 3;
          bwCount = 0;
        }
      }      
      break;
    case 3:
      break;

  }  
}

//_____________________________________________________________________________

static void UpdatePythonMenu(void)
{
  static uint8_t count = 0u;
  static uint8_t select = 0u;
  static uint8_t python_navigation_state = RUNNING_MENU;

  if(start_settings_menu) { // Python menu can be entered only from the main menu
    return;
  }

  uint8_t* buttonState = Buttons_GetStatus();

  if (start_python_menu)
  {

    // Handle python menu entering/exiting with the center button
    when(buttonState[E_Button_Center])
    {
      if(python_navigation_state == RUNNING_MENU) {
        python_navigation_state = RUNNING_BEHAVIOR;
        if (select != CurrentPythonItem) {
          CurrentPythonItem = select;
        }        
      } else {
        python_navigation_state = RUNNING_MENU;
        ExitPythonItem(CurrentPythonItem);
        CurrentPythonItem = E_Python_Menu;
      }

    }

    // Once you enter one of the user python scripts or the REPL, then you cannot exit normally but you need to power off the robot.
    switch (CurrentPythonItem)
    {
      case E_Python_Menu:
        when(buttonState[E_Button_Backward])
        {
          select = SelectNextPythonItem(select, -1);
        }

        when(buttonState[E_Button_Left])
        {
          select = SelectNextPythonItem(select, -1);
        }

        when(buttonState[E_Button_Forward])
        {
          select = SelectNextPythonItem(select, 1);
        }

        when(buttonState[E_Button_Right])
        {
          select = SelectNextPythonItem(select, 1);
        }

        SetPythonItemColor(select);
        break;

      case E_Python_REPL:   // Black + back lego leds
        PythonHandler_Run(0);
        break;

      case E_Python_Main1:  // Black + back lego leds
        PythonHandler_Run(1);
        break;

      case E_Python_Main2:  // Black + back lego leds
        PythonHandler_Run(2);
        break;

      case E_Python_Main3:  // Black + back lego leds
        PythonHandler_Run(3);
        break;

      case E_Python_Main4:  // Black + back lego leds
        PythonHandler_Run(4);
        break;

      case E_Python_Main5:  // Black + back lego leds
        PythonHandler_Run(5);
        break;

      case E_Python_Main6:  // Black + back lego leds
        PythonHandler_Run(6);
        break;

      case E_Python_Main7:  // Black + back lego leds
        PythonHandler_Run(7);
        break;                        

      default:
        // Do nothing
        break;
    }

    if (buttonState[E_Button_Forward] && buttonState[E_Button_Backward])
    {
      count++;

      if (count > 75)  // 75 * 40 [ms] = 3 [s]
      {
        Behavior_Enable(B_MODE);
        count = 0u;
        start_python_menu = false;
        CurrentPythonItem = E_Python_Menu;
      }
    } else {
      count = 0;
    }

  } else { // If micro Python menu not entered
    // When in the "investigator mode" (line tracking) the forward+backward are used to calibrate the ground sensors in a black surface,
    // thus avoid to enter the settings menu while trying to calibrate on the black surface.
    // Also in the "obedient mode" the forward+backward combination is used (to stop the robot), thus avoid entering the settings menu in this case.
    // The drawback is that you cannot enter the settings menu when the investigator or obedient modes are running, but this is not a big problem...
    if((Mode_get_current() != E_Mode_Investigator) && (Mode_get_current() != E_Mode_Obedient)) {
      if (buttonState[E_Button_Forward] && buttonState[E_Button_Backward]) {
        count++;
        if (count > 75)  // 75 * 40 [ms] = 3 [s]
        {
          Behavior_Disable(B_MODE);
          count = 0u;
          start_python_menu = true;
          CurrentPythonItem = E_Python_Menu;
        }
      } else {
        count = 0;
      }
    }
  }
}

//_____________________________________________________________________________

static T_Setting SelectNextPythonItem(T_Python_item item, int16_t index)
{
  int16_t temp = (int16_t)item;

  do
  {
    temp += index;

    while (temp > E_Python_Max)
    {
      temp -= (E_Python_Max + 1);
    }

    while (temp < 0)
    {
      temp += (E_Python_Max + 1);
    }
  }
  while (!IsPythonItemEnabled(temp));

  return (T_Python_item)temp;
}

//_____________________________________________________________________________

static bool IsPythonItemEnabled(T_Python_item item)
{
  bool result = true;

  if (item == E_Python_Menu)
  {
    result = false;
  }
  if((item==E_Python_Main1) && !script_is_present(1)) {
    return false;
  }
  if((item==E_Python_Main2) && !script_is_present(2)) {
    return false;
  }  
  if((item==E_Python_Main3) && !script_is_present(3)) {
    return false;
  }
  if((item==E_Python_Main4) && !script_is_present(4)) {
    return false;
  }
  if((item==E_Python_Main5) && !script_is_present(5)) {
    return false;
  }  
  if((item==E_Python_Main6) && !script_is_present(6)) {
    return false;
  }
  if((item==E_Python_Main7) && !script_is_present(7)) {
    return false;
  }
  return result;
}

//_____________________________________________________________________________

static void SetPythonItemColor(T_Python_item item)
{
  switch (item)
  {
    case E_Python_Menu:
      Leds_SetBodyBrightness(0u, 0u, 0u);
      break;

    case E_Python_REPL:   // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main1:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main2:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main3:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main4:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main5:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main6:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    case E_Python_Main7:  // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);    
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void ExitPythonItem(T_Python_item item)
{
  Leds_SetBodyBrightness(0u, 0u, 0u);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

  switch (item)
  {
    case E_Python_Menu:
      break;

    case E_Python_REPL:
      break;

    case E_Python_Main1:
      break;

    case E_Python_Main2: 
      break;

    case E_Python_Main3: 
      break;

    case E_Python_Main4: 
      break;

    case E_Python_Main5: 
      break;

    case E_Python_Main6: 
      break;

    case E_Python_Main7:
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void RunLegoLedAnimation(void)
{
  static uint8_t led_state;
  static uint8_t tick_count = 0;
  uint8_t l[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t fixed;

  tick_count++;
  if(tick_count > 5) { // Behaviors task run @ 50 Hz => change every 100 ms
    tick_count = 0;

    switch(led_state) {
      case 0:
        l[0] = 0;
        l[1] = 4;
        l[2] = 8;
        l[3] = 12;
        l[4] = 16;
        l[5] = 16;
        l[6] = 16;
        l[7] = 16;
        led_state = 1;
        break;

      case 1:
        l[0] = 4;
        l[1] = 8;
        l[2] = 12;
        l[3] = 16;
        l[4] = 16;
        l[5] = 16;
        l[6] = 16;
        l[7] = 12;
        led_state = 2;
        break;

      case 2:
        l[0] = 8;
        l[1] = 12;
        l[2] = 16;
        l[3] = 16;
        l[4] = 16;
        l[5] = 16;
        l[6] = 12;
        l[7] = 8;
        led_state = 3;
        break;        

      case 3:
        l[0] = 12;
        l[1] = 16;
        l[2] = 16;
        l[3] = 16;
        l[4] = 16;
        l[5] = 12;
        l[6] = 8;
        l[7] = 4;
        led_state = 4;
        break; 

      case 4:
        l[0] = 16;
        l[1] = 16;
        l[2] = 16;
        l[3] = 16;
        l[4] = 12;
        l[5] = 8;
        l[6] = 4;
        l[7] = 0;
        led_state = 5;
        break;

      case 5:
        l[0] = 12;
        l[1] = 16;
        l[2] = 16;
        l[3] = 16;
        l[4] = 16;
        l[5] = 12;
        l[6] = 8;
        l[7] = 4;
        led_state = 6;
        break;   

      case 6:
        l[0] = 8;
        l[1] = 12;
        l[2] = 16;
        l[3] = 16;
        l[4] = 16;
        l[5] = 16;
        l[6] = 12;
        l[7] = 8;
        led_state = 7;
        break;                   

      case 7:
        l[0] = 4;
        l[1] = 8;
        l[2] = 12;
        l[3] = 16;
        l[4] = 16;
        l[5] = 16;
        l[6] = 16;
        l[7] = 12;
        led_state = 0;
        break;       
    }   
/*
    led_state += 2;
    fixed = (led_state / MAX_BRIGHTNESS);

    l[fixed & 0x7] = MAX_BRIGHTNESS;
    l[(fixed - 2) & 0x7] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1)));
    l[(fixed + 2) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));
    l[(fixed + 4) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));
*/
    Leds_SetLegoFrontBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
    Leds_SetLegoBackBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
  }
}
