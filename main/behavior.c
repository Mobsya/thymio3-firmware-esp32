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
#include "timer_hw.h"
#include "timer_sw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BAT_HIGH        390
#define BAT_MIDDLE      360
#define BAT_LOW         340

#define SPEED_STEP      128

#define GROUND_IR_THRESHOLD    280
#define MOVEMENT_SPEED 300

#define LED_FRONT_LEFT 0
#define LED_FRONT_RIGHT 1
#define LED_BACK_RIGHT 2
#define LED_BACK_LEFT 3

#define SETTINGS_BRIGHTNESS  (MAX_BRIGHTNESS / 3u)

#define MOT_FW_BW_CALIB_STEP (DEFAULT_MOT15CM/100) //90000

#define STOP_DURATION_US 500000u //!< Delay at the end of a movement

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Setting_Menu,
  E_Setting_Volume,
  E_Setting_Motor,
  E_Setting_MotorFwBw,  
  E_Setting_Color,
  E_Reset_WiFi_Credentials,
  E_Setting_Gyro,
  E_Setting_Ground,
  E_Setting_Max = E_Setting_Ground
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
static uint8_t ledUpdateCount = 0;
static uint8_t ledUpdatePos = 0;
bool motionInProgress = false;
T_TimerSw *StopTimer = NULL; //!< Timer used to add a delay at the end of a movement

// Motors left/right calibration
static int16_t correction = 0; // motors right/left correction

// Motors forward/backware 15 cm calibration
static uint8_t motFwBwCalibType = 0; // 0=no calibration done, 1=calibration done, 2=reset calibration
static int8_t motFwCorrCount = 0;
static int8_t motBwCorrCount = 0;

// Gyro calibration
bool gyroRotCalibrated = false;

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

static void HandleWifiCredentials(void);

static void CalibrateGround(void);

static void CalibrateMotFwBw(void);

static void UpdatePythonMenu(void);

static T_Python_item SelectNextPythonItem(T_Python_item item, int16_t index);

static bool IsPythonItemEnabled(T_Python_item item);

static void SetPythonItemColor(T_Python_item item);

static void ExitPythonItem(T_Python_item item);

static void RunLegoLedAnimation(void);

static void SetColorSensorLed(void);

//! \brief     Interrupt called at the end of a movement
//! \pre       First initialize the mode
//! \param     arg - Not used
//! \return    None
void IRAM_ATTR ISR_TimerExpired(void *arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

static void Callback_TimerStop(void *arg)
{
  motionInProgress = false;
}

void IRAM_ATTR ISR_TimerExpired(void *para)
{
    timer_spinlock_take(TIMER_GROUP_1);
    //int timer_idx = (int) para;

    /* Retrieve the interrupt status and the counter value
       from the timer that reported the interrupt */
    uint32_t timer_intr = timer_group_get_intr_status_in_isr(TIMER_GROUP_1);

    /* Clear the interrupt
       and update the alarm time for the timer with without reload */
    if (timer_intr & TIMER_INTR_T1) {
        timer_group_clr_intr_status_in_isr(TIMER_GROUP_1, TIMER_1);
        Common_SetTargetSpeed(0, 0);
        TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_US);
        TimerHw_Stop(1, 1);
    }

    /* After the alarm has been triggered
      we need enable it again, so it is triggered the next time */
    timer_group_enable_alarm_in_isr(TIMER_GROUP_1, 1);

    timer_spinlock_give(TIMER_GROUP_1);
}

void Behavior_Init(void)
{
  Setting.Volume = Settings_GetVolumeSettings();

  Settings_GetMotorsSettings(Setting.Motors);

  TaskIsStarted = false;
  Behavior = 0u;

  Codec_SetVolume(Setting.Volume);

  StopTimer = TimerSw_Create(STOP_DURATION_US, Callback_TimerStop);
  TimerHw_Init(1, 1, true, DEFAULT_MOT15CM, ISR_TimerExpired);
}

//_____________________________________________________________________________

void Behavior_Start(void)
{
  xTaskCreatePinnedToCore(
    RunBehaviorTask,  // Function to implement the task
    "behavior",       // Name of the task
    2560,             // Stack size in words
    NULL,             // Task input parameter
    6,                // Priority of the task
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
    Codec_PlayOnboardSound(TONE_TYPE_FLECHES);
  }
  else if (button == E_Button_Center)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_ROND);
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

  if (ENABLED(B_LEDS_RGB))
  {
    SetColorSensorLed();
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

    if ((intensity < 0) || ((abs(acc.X) + abs(acc.Y)) <= 1700))
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
          //Leds_SetSingleBrightness(ind, ((6-intensity*3)<0)?0:(6-intensity*3));
          Leds_SetSingleBrightness(ind, ((4-intensity*2)<0)?0:(4-intensity*2));
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
    Codec_PlayOnboardSound(TONE_TYPE_FLECHES);
  }

  when(buttonState[E_Button_Left] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_FLECHES);
  }

  when(buttonState[E_Button_Center] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_ROND);
  }

  when(buttonState[E_Button_Forward] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_FLECHES);
  }

  when(buttonState[E_Button_Right] != 0u)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_FLECHES);
  }
}

//_____________________________________________________________________________

static void UpdateSettings(void)
{
  static uint8_t count = 0u;
  static uint8_t select = 0u;
  static uint8_t settings_navigation_state = RUNNING_MENU;
  static uint8_t delayCount = 0;
  static bool onSettingStart = true;

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
        updateSettingsState = 0;
        if (select != CurrentSetting) {
          CurrentSetting = select;
          onSettingStart = true;
        }        
      } else {
        settings_navigation_state = RUNNING_MENU;
        ExitSetting(CurrentSetting);
        CurrentSetting = E_Setting_Menu;
        updateSettingsState = 0;
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

      case E_Setting_Volume:
        AdjustVolume();
        break;

      case E_Setting_Motor:
        if(onSettingStart)
        {
          onSettingStart = false;
          correction = 0;
          Setting.Motors[0]  = (256 + correction);
          Setting.Motors[1] = (256 - correction);          
        }      
        TuneMotors();
        break;

      case E_Setting_MotorFwBw:
        if(onSettingStart)
        {
          onSettingStart = false;
          Setting.Mot15cm[0] = DEFAULT_MOT15CM;
          Setting.Mot15cm[1] = DEFAULT_MOT15CM;
          motFwCorrCount = 0;
          motBwCorrCount = 0;
          Setting.MotFwBw = DEFAULT_MOT_FW_TO_BW;
        }
        CalibrateMotFwBw();
        break;

      case E_Setting_Color:
        CalibrateColor();
        break;

      case E_Reset_WiFi_Credentials:
        HandleWifiCredentials();
    	  break;

      case E_Setting_Gyro:
        if(onSettingStart)
        {
          onSettingStart = false;
          gyroRotCalibrated = false;
          Setting.ZeroOffGyro[0] = DEFAULT_OFFSET_GYRO_X;
          Setting.ZeroOffGyro[1] = DEFAULT_OFFSET_GYRO_Y;
          Setting.ZeroOffGyro[2] = DEFAULT_OFFSET_GYRO_Z;
          Setting.GyroRotFactor = DEFAULT_GYRO_ROT_FACTOR;
          Gyroscope_ResetAngle();
        }
        CalibrateGyro();
        break;

      case E_Setting_Ground:
        if(onSettingStart)
        {
          onSettingStart = false;
          Setting.GroundWhite[0] = DEFAULT_GROUND_WHITE;
          Setting.GroundWhite[1] = DEFAULT_GROUND_WHITE;
          Setting.GroundBlack[0] = DEFAULT_GROUND_BLACK;
          Setting.GroundBlack[1] = DEFAULT_GROUND_BLACK;
        }      
        CalibrateGround();
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
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
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

    case E_Setting_Volume:  // Red
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, 0, 0);
      Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, 0, 0);
      break;

    case E_Setting_Motor:  // Green
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, SETTINGS_BRIGHTNESS, 0);
      Leds_SetBackRightBrightness(0, SETTINGS_BRIGHTNESS, 0);
      break;

    case E_Setting_MotorFwBw:  // Blue
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, 0, SETTINGS_BRIGHTNESS);
      Leds_SetBackRightBrightness(0, 0, SETTINGS_BRIGHTNESS);
      break;

    case E_Setting_Color: // Cyan
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
      Leds_SetBackRightBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
    	break;

    case E_Reset_WiFi_Credentials: // Magenta
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
      Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
      break;

    case E_Setting_Gyro: // Yellow
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
      Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
      break;      

    case E_Setting_Ground: // White
      Leds_SetFrontLeftBrightness(0, 0, 0);
      Leds_SetFrontRightBrightness(0, 0, 0);
      Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
      Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
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
  //Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

  switch (setting)
  {
    case E_Setting_Menu:

      break;

    case E_Setting_Volume:
      // Write to the settings file
      if(Settings_WriteVolume(Setting.Volume) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }
      Settings_SetVolumeSettings(Setting.Volume);
      ESP_LOGE(Tag, "Write volume: %d", Setting.Volume);
      break;

    case E_Setting_Motor:
      Common_SetTargetSpeed(0, 0);
      // Write to the settings file
      if(Settings_WriteMotors(Setting.Motors) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }      
      ESP_LOGE(Tag, "Write motor: %d %d", Setting.Motors[0], Setting.Motors[1]);
      break;

    case E_Setting_MotorFwBw:
      // Save data based on actual calibration done
      //if(motFwBwCalibType == 1)
      //{
        Settings_SetMot15cmSettings(Setting.Mot15cm);
        if(Settings_WriteMot15cm(Setting.Mot15cm) < 0)
        {
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BAD);
        }        
        Setting.MotFwBw = (float)Setting.Mot15cm[1]/(float)Setting.Mot15cm[0];
        Settings_SetMotFwBwSettings(Setting.MotFwBw);
        if(Settings_WriteMotFwBwFactor(Setting.MotFwBw) < 0)
        {
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BAD);
        }        
        ESP_LOGE(Tag, "Write mot fw,bw,factor: %lld, %lld, %f", Setting.Mot15cm[0], Setting.Mot15cm[1], Setting.MotFwBw);
        //printf("Write mot fw,bw,factor: %lld, %lld, %f", Setting.Mot15cm[0], Setting.Mot15cm[1], Setting.MotFwBw);
        //fflush(stdout);
      //}
      //else if(motFwBwCalibType == 2)
      //{
      //  WriteFactoryMot15cm();
      //  WriteFactoryMotFwBwFactor();
      //}
      //motFwBwCalibType = 0;
      break;

    case E_Setting_Color:
      IsColorCalibrationInProgress = false;
      break;

    case E_Reset_WiFi_Credentials:
    	break;

    case E_Setting_Gyro:      
      Settings_SetZeroOffGyroSettings(Setting.ZeroOffGyro);
      if(Settings_WriteZeroOffGyro(Setting.ZeroOffGyro) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }      
      ESP_LOGI(Tag, "Write zero gyro: %d, %d, %d", Setting.ZeroOffGyro[0], Setting.ZeroOffGyro[1], Setting.ZeroOffGyro[2]);
      if(gyroRotCalibrated)
      {
        Setting.GyroRotFactor = -Gyroscope_GetAngleZ()/4;
      }
      Settings_SetGyroRotFactorSettings(Setting.GyroRotFactor);
      if(Settings_WriteGyroRotFactor(Setting.GyroRotFactor) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }      
      AngleController_UpdateRotFactor(Setting.GyroRotFactor);
      ESP_LOGI(Tag, "Gyro rot factor, controller: %d, %d", Setting.GyroRotFactor, AngleController_GetRotFactor());               
      break;

    case E_Setting_Ground:  
      if(Settings_WriteGroundWhite(Setting.GroundWhite) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }      
      Settings_SetGroundWhiteSettings(Setting.GroundWhite);  
      if(Settings_WriteGroundBlack(Setting.GroundBlack) < 0)
      {
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BAD);
      }      
      Settings_SetGroundBlackSettings(Setting.GroundBlack);
      Common_SetGroundThr(Setting.GroundBlack);
      STM32_SetGroundThr(Setting.GroundWhite, Setting.GroundBlack);
      ESP_LOGI(Tag, "Ground calib white: %d, %d black: %d, %d", Setting.GroundWhite[0], Setting.GroundWhite[1], Setting.GroundBlack[0], Setting.GroundBlack[1]);
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

  // Red
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(SETTINGS_BRIGHTNESS, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(SETTINGS_BRIGHTNESS, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, 0, 0);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

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
  uint8_t* buttonState = Buttons_GetStatus();

  // Green
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(0, SETTINGS_BRIGHTNESS, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, SETTINGS_BRIGHTNESS, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, SETTINGS_BRIGHTNESS, 0);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, SETTINGS_BRIGHTNESS, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

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

    Setting.Motors[0]  = (256 + correction);
    Setting.Motors[1] = (256 - correction);

    Settings_SetMotorsSettings(Setting.Motors);    // Used to send the value to STM32
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

  // Cyan
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

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

static void HandleWifiCredentials(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  static uint8_t delayCount = 0;

  // Magenta
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }    	  

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to reset wifi credentials
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
        wifi_manager_delete_sta_config();
        updateSettingsState = 2;
      }
      break;

    case 2: // Wait for the center button press in order to exit
      break;
  }

}

//_____________________________________________________________________________

static void CalibrateGyro(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  static uint8_t delayCount = 0;
  static uint8_t rotCount = 0;

  // Yellow
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start offsets calibration or right button press to start rotation calibration
      when(buttonState[E_Button_Forward] != 0u)
      {
        updateSettingsState = 1;
        delayCount = 0;
      }
      when(buttonState[E_Button_Right] != 0u)
      {
        AngleController_UpdateRotFactor(DEFAULT_GYRO_ROT_FACTOR);
        updateSettingsState = 2;
        AngleController_Start(-360, MOVEMENT_SPEED);
      }     
      break;
    
    case 1: // Wait a bit before actually taking samples in order to avoid interference from the button press and in the meantime show led animation
      delayCount++;
      Leds_SetLegoBackProgress(delayCount/6);
      if(delayCount == 50) // Based on 50 Hz behaviors update rate
      {
        Gyroscope_DisableContinuousCalib();
        Gyroscope_Calibrate();
        Gyroscope_GetCalibration(Setting.ZeroOffGyro);
        Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
        updateSettingsState = 0;
      }
      break;

    case 2: // Wait rotation is terminated
      if(AngleController_Completed())
      {
        updateSettingsState = 3;
        delayCount = 0;
      }
      break;

    case 3: // Wait a bit to be sure all is still
      delayCount++;
      if(delayCount == 20)
      {
        updateSettingsState = 4;
        delayCount = 0;
        Gyroscope_ResetAngle();
        gyroRotCalibrated = true;
      }
      break;

    case 4: // Wait for rotation tuning (left/right) and calibration end (center button)
      when(buttonState[E_Button_Right] != 0u)
      {
        rotCount = 0;        
        updateSettingsState = 5;
        Common_SetTargetSpeed(MOVEMENT_SPEED/2, -MOVEMENT_SPEED/2);
      }
      when(buttonState[E_Button_Left] != 0u)
      {
        rotCount = 0;
        updateSettingsState = 5;
        Common_SetTargetSpeed(-MOVEMENT_SPEED/2, MOVEMENT_SPEED/2);
      }
      break;

    case 5: // Wait tuning rotation end
      rotCount++;
      if(rotCount == 6)
      {
        Common_SetTargetSpeed(0, 0);
        updateSettingsState = 4;        
      }    
      break;   

  }

}

//_____________________________________________________________________________

static void CalibrateGround(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  static uint8_t delayCount = 0;
  static int16_t tempAmbient[2];
  static int16_t tempReflected[2];

  // White
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS, SETTINGS_BRIGHTNESS);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start ground white calibration or back button press to start ground black calibration
      when(buttonState[E_Button_Forward] != 0u)
      {
        updateSettingsState = 1;
        delayCount = 0;
      }
      when(buttonState[E_Button_Backward] != 0u)
      {
        updateSettingsState = 2;
        delayCount = 0;
      }      
      break;
    
    case 1: // Wait a bit before actually taking samples in order to avoid interference from the button press and in the meantime show led animation
      delayCount++;
      Leds_SetLegoBackProgress(delayCount/6);
      if(delayCount == 50) // Based on 50 Hz behaviors update rate
      {
        GetGroundAmbients(tempAmbient);
        GetGroundReflecteds(tempReflected);
        Setting.GroundWhite[0] = tempReflected[0] - tempAmbient[0];
        if(Setting.GroundWhite[0] < 0)
        {
          Setting.GroundWhite[0] = 0;
        }
        Setting.GroundWhite[1] = tempReflected[1] - tempAmbient[1];
        if(Setting.GroundWhite[1] < 0)
        {
          Setting.GroundWhite[1] = 0;
        }        
        updateSettingsState = 0;
      }
      break;

    case 2: // Wait a bit before actually taking samples in order to avoid interference from the button press and in the meantime show led animation
      delayCount++;
      Leds_SetLegoBackProgress(delayCount/6);
      if(delayCount == 50) // Based on 50 Hz behaviors update rate
      {
        GetGroundAmbients(tempAmbient);
        GetGroundReflecteds(tempReflected);
        Setting.GroundBlack[0] = tempReflected[0] - tempAmbient[0];
        if(Setting.GroundBlack[0] < 0)
        {
          Setting.GroundBlack[0] = 0;
        }
        Setting.GroundBlack[1] = tempReflected[1] - tempAmbient[1];
        if(Setting.GroundBlack[1] < 0)
        {
          Setting.GroundBlack[1] = 0;
        }
        updateSettingsState = 0;
      }
      break;
  
  }
}

//_____________________________________________________________________________

static void CalibrateMotFwBw(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  static uint8_t currentCalib = 0; // 0=nothing, 1=forward, 2=backward

  // Blue
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, SETTINGS_BRIGHTNESS);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  }

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start calibration
      when((buttonState[E_Button_Forward] != 0u) && !motionInProgress)
      {        
        motionInProgress = true;
        Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
        TimerHw_Set_Alarm_Ticks(1, 1, Setting.Mot15cm[0]);
        TimerHw_Reset_Counter(1, 1);  
        TimerHw_Start(1, 1);
        currentCalib = 1;
        Leds_SetCircleProgress(motFwCorrCount/2);
      }
      when((buttonState[E_Button_Backward] != 0u) && !motionInProgress)
      {
        motionInProgress = true;
        Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);
        TimerHw_Set_Alarm_Ticks(1, 1, Setting.Mot15cm[1]);
        TimerHw_Reset_Counter(1, 1);  
        TimerHw_Start(1, 1);
        currentCalib = 2;
        Leds_SetCircleProgress(motBwCorrCount/2);
      }      
      when(buttonState[E_Button_Right] != 0u)
      {
        if(currentCalib == 1)
        {
          Setting.Mot15cm[0] += MOT_FW_BW_CALIB_STEP;
          motFwCorrCount++;
          Leds_SetCircleProgress(motFwCorrCount/2);
        }
        if(currentCalib == 2)
        {
          Setting.Mot15cm[1] += MOT_FW_BW_CALIB_STEP;
          motBwCorrCount++;
          Leds_SetCircleProgress(motBwCorrCount/2);
        }
      }
      when(buttonState[E_Button_Left] != 0u)
      {
        if(currentCalib == 1)
        {
          Setting.Mot15cm[0] -= MOT_FW_BW_CALIB_STEP;
          motFwCorrCount--;
          Leds_SetCircleProgress(motFwCorrCount/2);
        }
        if(currentCalib == 2)
        {
          Setting.Mot15cm[1] -= MOT_FW_BW_CALIB_STEP;
          motBwCorrCount--;
          Leds_SetCircleProgress(motBwCorrCount/2);
        }
      }
      break;
  }
}

//_____________________________________________________________________________

static void CalibrateMotFwBw2(void)
{
  uint8_t* buttonState = Buttons_GetStatus();
  static uint8_t delayCount = 0;
  static int16_t groundsBlack[2];
  static int16_t groundsWhite[2];
  static bool timerInitialized = false;
  static int16_t hysteresisLeft = 0;
  static int16_t hysteresisRight = 0;

  // Blue
  ledUpdateCount++;
  if(ledUpdateCount == 10) // Based on 50 Hz behaviors update rate
  {
    ledUpdateCount = 0;
    switch(ledUpdatePos)
    {
      case LED_FRONT_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break; 
      case LED_FRONT_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos++;
        break;  
      case LED_BACK_RIGHT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, 0);
        Leds_SetBackRightBrightness(0, 0, SETTINGS_BRIGHTNESS);
        ledUpdatePos++;
        break;
      case LED_BACK_LEFT:
        Leds_SetFrontLeftBrightness(0, 0, 0);
        Leds_SetFrontRightBrightness(0, 0, 0);
        Leds_SetBackLeftBrightness(0, 0, SETTINGS_BRIGHTNESS);
        Leds_SetBackRightBrightness(0, 0, 0);
        ledUpdatePos= 0 ;
        break;                      
    }
  } 

  switch(updateSettingsState)
  {
    case 0: // Wait forward button press to start calibration
      when(buttonState[E_Button_Forward] != 0u)
      {
        motFwBwCalibType = 1;
        updateSettingsState = 1;
        Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
        Settings_GetGroundBlackSettings(groundsBlack);
        Settings_GetGroundWhiteSettings(groundsWhite);
        hysteresisLeft = (groundsWhite[0] - groundsBlack[0])/3;
        hysteresisRight = (groundsWhite[1] - groundsBlack[1])/3;
        ESP_LOGI(Tag, "ground hyst = %d, %d", hysteresisLeft, hysteresisRight);
      }
      when(buttonState[E_Button_Backward] != 0u)
      {
        motFwBwCalibType = 2;
        updateSettingsState = 12;
      }
      if(!timerInitialized)
      {
        timerInitialized = true;
        TimerHw_Init(1, 1, true, 0, NULL);
      }
      break;
    
    case 1: // Go forward until the start of the first black line is detected, then stop since the time measurement need to be started from still
      if ((GetGroundValue(0) < (groundsBlack[0]+hysteresisLeft)) && (GetGroundValue(1) < (groundsBlack[1]+hysteresisRight)))
      {
        delayCount = 0;
        Common_SetTargetSpeed(0, 0);
        ESP_LOGI(Tag, "detected start first line = %d, %d", GetGroundValue(0), GetGroundValue(1));
        updateSettingsState = 2;
      }
      break;

    case 2: // Wait a bit to be stopped, then start the forward time needed to travel to the second black line
      delayCount++;
      if(delayCount >= 20) // 400 ms
      {
        TimerHw_Reset_Counter(1, 1);  
        TimerHw_Start(1, 1);
        Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
        updateSettingsState = 3;
      }
      break;

    case 3: // During the travel the robot should be able to detect the end of the first black line
      if ((GetGroundValue(0) > (groundsWhite[0]-hysteresisLeft)) && (GetGroundValue(1) > (groundsWhite[1]-hysteresisRight)))
      {
        updateSettingsState = 4;
        //Common_SetTargetSpeed(0, 0); 
        ESP_LOGI(Tag, "detected end first line = %d, %d", GetGroundValue(0), GetGroundValue(1));
      }
      break;

    case 4: // Wait for the start of the second black line detection, then compute the forward time
      if ((GetGroundValue(0) < (groundsBlack[0]+hysteresisLeft)) && (GetGroundValue(1) < (groundsBlack[1]+hysteresisRight)))
      {      
        Setting.Mot15cm[0] = TimerHw_Get_Counter(1, 1);
        TimerHw_Stop(1, 1);        
        //Common_SetTargetSpeed(0, 0);     
        ESP_LOGI(Tag, "detected start 2nd line = %d, %d", GetGroundValue(0), GetGroundValue(1));  
        ESP_LOGI(Tag, "fwCounter = %lld", Setting.Mot15cm[0]);
        delayCount = 0;
        updateSettingsState = 5;
      }      
      break;

    case 5: // // Wait a bit before stopping the robot
      delayCount++;
      if(delayCount >= 20) // 400 ms
      {
        Common_SetTargetSpeed(0, 0);
        delayCount = 0;
        updateSettingsState = 6;
      } 
      break;     

    case 6: // // Wait a bit to be stopped, then start going backward
      delayCount++;
      if(delayCount >= 20) // 400 ms
      {
        Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);
        updateSettingsState = 7;
      } 
      break;      

    case 7: // The robot should be able to detect the end of the second black line
      if ((GetGroundValue(0) < (groundsBlack[0]+hysteresisLeft)) && (GetGroundValue(1) < (groundsBlack[1]+hysteresisRight)))
      {   
        updateSettingsState = 8;
        ESP_LOGI(Tag, "detected end 2nd line = %d, %d", GetGroundValue(0), GetGroundValue(1));
      }    
      break;

    case 8: // Continue until the second black line is crossed, then stop since the time measurement need to be started from still
      if ((GetGroundValue(0) > (groundsWhite[0]-hysteresisLeft)) && (GetGroundValue(1) > (groundsWhite[1]-hysteresisRight)))
      {
        updateSettingsState = 9;
        Common_SetTargetSpeed(0, 0); 
        ESP_LOGI(Tag, "detected start 2nd line = %d, %d", GetGroundValue(0), GetGroundValue(1));
        delayCount = 0;
      }    
      break;

    case 9: // // Wait a bit to be stopped, then start the backward time needed to travel to the first black line
      delayCount++;
      if(delayCount >= 20) // 400 ms
      {
        TimerHw_Reset_Counter(1, 1);  
        TimerHw_Start(1, 1);
        Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);
        updateSettingsState = 10;
      } 
      break;

    case 10: // During the travel the robot should be able to detect the end of the first black line
      if ((GetGroundValue(0) < (groundsBlack[0]+hysteresisLeft)) && (GetGroundValue(1) < (groundsBlack[1]+hysteresisRight)))
      {
        updateSettingsState = 11;
        ESP_LOGI(Tag, "detected end first line = %d, %d", GetGroundValue(0), GetGroundValue(1));
      }
      break;

    case 11: // Wait for the first black line to be crossed, then stop the robot and compute the backward time
      if ((GetGroundValue(0) > (groundsWhite[0]-hysteresisLeft)) && (GetGroundValue(1) > (groundsWhite[1]-hysteresisRight)))
      {
        Setting.Mot15cm[1] = TimerHw_Get_Counter(1, 1);
        TimerHw_Stop(1, 1);        
        Common_SetTargetSpeed(0, 0);
        ESP_LOGI(Tag, "detected start first line = %d, %d", GetGroundValue(0), GetGroundValue(1));
        ESP_LOGI(Tag, "bwCounter = %lld", Setting.Mot15cm[1]);
        updateSettingsState = 12;
      }      
      break;

    case 12: // Wait for the center buttons to save settings
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

//_____________________________________________________________________________

static void SetColorSensorLed(void)
{
  static T_Color color = E_Color_Unknown;
  if(color != ColorSensor_GetColor())
  {
    color = ColorSensor_GetColor();
    switch(color)
    {
      case E_Color_Red:
        Leds_SetColorSensorBrightness(MAX_BRIGHTNESS, 0, 0);
        break;

      case E_Color_Green:
        Leds_SetColorSensorBrightness(0, MAX_BRIGHTNESS, 0);
        break;

      case E_Color_Cyan:
        Leds_SetColorSensorBrightness(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Blue:
        Leds_SetColorSensorBrightness(0, 0, MAX_BRIGHTNESS);
        break;

      case E_Color_Yellow:
        Leds_SetColorSensorBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0);
        break;

      case E_Color_Purple:
        Leds_SetColorSensorBrightness(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS);
        break;

      case E_Color_White:
        Leds_SetColorSensorBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Black:
      case E_Color_Unknown:
      default:
        Leds_SetColorSensorBrightness(0, 0, 0);
        break;
    }
  }
}

bool Behavior_IsMotionInProgress(void)
{
  return motionInProgress;
}

void Behavior_SetMotionInProgress(bool value)
{
  motionInProgress = value;
}
