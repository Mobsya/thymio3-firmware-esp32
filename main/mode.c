//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    mode.c
//! \brief   This module provides the useful functions to use the modes
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>  // Only for debug

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "mode.h"

#include "accelerometer.h"
#include "aseba_esp32.h"
#include "behavior.h"
#include "buttons.h"
#include "fifo.h"
#include "leds.h"
#include "tcp_server.h"
#include "timer_sw.h"

#include "color_sensor.h"

#include "sound.h"
#include "mp3.h"
#include "stm32_i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUTTONS_SEQ_BUFFER_SIZE        30u

#define MOVEMENT_DURATION_us      2000000u
#define STOP_DURATION_us           500000u

#define SPEED_LINE    300
#define STATE_BLACK     0
#define STATE_WHITE     1
#define DIR_LEFT     (-1)
#define DIR_L_LEFT   (-2)
#define DIR_RIGHT     (1)
#define DIR_L_RIGHT   (2)
#define DIR_LOST     (10)
#define DIR_FRONT     (0)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "mode";

static T_Mode CurrentMode;
static T_Mode SelectMode;

static T_FifoBytes* ButtonsSeqFifo = NULL;
static uint8_t ButtonsSeqBuffer[BUTTONS_SEQ_BUFFER_SIZE];

static bool RecordSequenceIsFinished = false;
static bool MovementIsStarted = false;
static bool MovementIsInProgress = false;
static bool MovementTimerIsRunning = false;
static bool StopTimerIsRunning = false;
static uint16_t Position = 0u;

static T_TimerSw* MovementTimer = NULL;  //!< Used to move the robot in obedient mode
static T_TimerSw* StopTimer = NULL;      //!< Used to stop the robot in obedient mode

const T_Note Accord[4];

const T_Note LeLionEstMort[15];

const T_Note JamesBond[21];

static T_Melody MelodyInvestigator;
static T_Melody MelodyObedient;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void StartMode(T_Mode mode);

static void ExitMode(T_Mode mode);

static T_Mode SelectNextMode(T_Mode mode, int16_t index);

static bool IsModeEnabled(T_Mode mode);

static void SetModeColor(T_Mode mode);

static void RunExplorer(void);

static void RunFollower(void);

static void RunAttentive(void);

static void RunLineTracker(void);

static void RunObedient(void);

static void HandlePositiveSpeed(int16_t speed);

static void HandleNegativeSpeed(int16_t speed);

static int16_t GetBodyColorPulse(void);

static void GetRainbow(uint8_t* rgb);

static uint8_t GetRainbowBrightness(uint8_t index);

static void RunCircleLedRotation(void);

static void RunCircleLedCross(void);

static void SetSpeedUsingButtons(int16_t* speed);

static bool CalibrateLevelUsingButtons(uint16_t* blackLevel, uint16_t* whiteLevel);

static void GetLineSensorsState(uint16_t* blackLevel, uint16_t* whiteLevel, uint8_t* state);

static void GetLineDirection(uint8_t* state, int16_t* direction);

static void SetTargetAccordingToDirection(int16_t* direction);

static void RecordButtonsSequence(void);

static void PlayMovementSequence(void);

static void Callback_TimerMovement(void* arg);

static void Callback_TimerStop(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Mode_Init(void)
{
#if 0  // TODO when the mode are defined
  // Init the defaults behaviors + our behavior.

  vm_active = vm_enabled;

  init_mode(MODE_MENU);

  if (vm_active)
  {
    _selecting = 0;
  }
  else
  {
    _selecting = MODE_FOLLOW; // Bypass the "VM exit mode"
    set_mode_color(_selecting);
  }
#endif

#if 0
  MelodyInvestigator.Melody = LeLionEstMort;
  MelodyInvestigator.Tempo  = E_Tempo_Allegro;
  MelodyInvestigator.Loop   = 1;
  MelodyInvestigator.Size   = 15;

  MelodyObedient.Melody = JamesBond;
  MelodyObedient.Tempo  = E_Tempo_Vivace;
  MelodyObedient.Loop   = 1;
  MelodyObedient.Size   = 21;
#endif

  Behavior_Enable(B_ALWAYS | B_MODE);

  ButtonsSeqFifo = Fifo8bits_Create(ButtonsSeqBuffer, BUTTONS_SEQ_BUFFER_SIZE);
  RecordSequenceIsFinished = false;
  MovementIsStarted = false;
  MovementIsInProgress = false;
  MovementTimerIsRunning = false;
  StopTimerIsRunning = false;
  MovementTimer = TimerSw_Create(MOVEMENT_DURATION_us, Callback_TimerMovement);
  StopTimer = TimerSw_Create(STOP_DURATION_us, Callback_TimerStop);

  ESP_LOGI(Tag, "Mode is initialized");
}

//_____________________________________________________________________________

void Mode_InitVM(void)
{
  Behavior_Enable(B_LEDS_ACC);
  Behavior_Enable(B_LEDS_PROX);
  Behavior_Enable(B_SOUND_BUTTON);

  ESP_LOGI(Tag, "VM Mode is initialized");
}

//_____________________________________________________________________________

void Mode_Run(void)
{
  uint8_t* buttonState;

  static uint8_t ignore;

  buttonState = Buttons_GetStatus();

  ignore++;

  // As the user mode disable the "mode menu thing"...
  if (ignore > 100)
  {
    ignore = 101;

    when(buttonState[E_Button_Center])
    {
      ExitMode(CurrentMode);
//#if 0  // FIXME
      if (SelectMode == E_Mode_Menu)
      {
        // Special case, if we select the mode menu stuff
        Behavior_Disable(B_MODE | B_SETTING);
        Mode_InitVM();
        return;
      }
//#endif

      if (SelectMode == CurrentMode)
      {
        StartMode(E_Mode_Menu);
        CurrentMode = E_Mode_Menu;
      }
      else
      {
        StartMode(SelectMode);
        CurrentMode = SelectMode;
      }
    }
  }

  if (STM32_IsUSBPortOpen() || TCPServer_IsSocketAccepted())
  {
    ExitMode(CurrentMode);
    Behavior_Disable(B_MODE);
    Mode_InitVM();
    return;
  }

  switch (CurrentMode)
  {
    case E_Mode_Menu:
      when(buttonState[E_Button_Backward])
      {
        SelectMode = SelectNextMode(SelectMode, -1);
      }

      when(buttonState[E_Button_Left])
      {
        SelectMode = SelectNextMode(SelectMode, -1);
      }

      when(buttonState[E_Button_Forward])
      {
        SelectMode = SelectNextMode(SelectMode, 1);
      }

      when(buttonState[E_Button_Right])
      {
        SelectMode = SelectNextMode(SelectMode, 1);
      }

      SetModeColor(SelectMode);
      break;

    case E_Mode_Follower:
      RunFollower();
      break;

    case E_Mode_Explorer:
      RunExplorer();
      break;

    case E_Mode_Attentive:
      RunAttentive();
      break;

    case E_Mode_LineTracker:
      RunLineTracker();
      break;

    case E_Mode_Obedient:
      RunObedient();
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void StartMode(T_Mode mode)
{
  SetModeColor(mode);

  switch (mode)
  {
    case E_Mode_Menu:
      ESP_LOGE(Tag, "Mode MENU");
      Behavior_Enable(B_SETTING);
      break;

    case E_Mode_Follower:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Explorer:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Attentive:
      Behavior_Enable(B_LEDS_PROX);

#if 0
      xTaskCreatePinnedToCore(
        Sound_RunPlayerTask,
        "sound",       // Name of the task
        2048,          // Stack size in words
        &MelodyInvestigator,  // Task input parameter
        2,             // Priority of the task
        NULL,          // Task handle
        0);            // Core where the task should run
#endif
      break;

    case E_Mode_LineTracker:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      Behavior_Enable(B_LEDS_PROX);
      RecordSequenceIsFinished = false;
#if 0
      xTaskCreatePinnedToCore(
        Sound_RunPlayerTask,
        "sound",       // Name of the task
        2048,          // Stack size in words
        &MelodyObedient,  // Task input parameter
        2,             // Priority of the task
        NULL,          // Task handle
        0);            // Core where the task should run
#endif
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void ExitMode(T_Mode mode)
{
  Leds_SetBodyBrightness(0u, 0u, 0u);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
#if 0  // FIXME
  Leds_SetSingleBrightness(E_Led_Front_IR_0, 0u);
  Leds_SetSingleBrightness(E_Led_Front_IR_1, 0u);
  Leds_SetSingleBrightness(E_Led_Front_IR_2A, 0u);
  Leds_SetSingleBrightness(E_Led_Front_IR_2B, 0u);
  Leds_SetSingleBrightness(E_Led_Front_IR_3, 0u);
  Leds_SetSingleBrightness(E_Led_Front_IR_4, 0u);
  Leds_SetSingleBrightness(E_Led_Ground_IR_0, 0u);
  Leds_SetSingleBrightness(E_Led_Ground_IR_1, 0u);
  Leds_SetSingleBrightness(E_Led_IR_Back_Left, 0u);
  Leds_SetSingleBrightness(E_Led_IR_Back_Right, 0u);
#endif

  switch (mode)
  {
    case E_Mode_Menu:
      Behavior_Disable(B_SETTING);
      break;

    case E_Mode_Follower:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Explorer:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Attentive:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_LineTracker:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static T_Mode SelectNextMode(T_Mode mode, int16_t index)
{
  int16_t temp = mode;

  do
  {
    temp += index;

    while (temp > E_Mode_Max)
    {
      temp -= (E_Mode_Max + 1);
    }

    while (temp < 0)
    {
      temp += (E_Mode_Max + 1);
    }
  }
  while (!IsModeEnabled(temp));

  return (T_Mode)temp;
}

//_____________________________________________________________________________

static bool IsModeEnabled(T_Mode mode)
{
  bool result = true;

#if 0  // FIXME
  if ((temp == MODE_DRAW) || (temp == MODE_SIDE))
  {
    result = false;
  }
  else if ((mode == E_Mode_Menu) && !VMActive) // Here mode menu == VM mode
  {
    result = false;
  }
#endif

  if (mode == E_Mode_Menu)
  {
    result = false;
  }

  return result;
}

//_____________________________________________________________________________

static void SetModeColor(T_Mode mode)
{
  switch (mode)
  {
    case E_Mode_Menu:
      Leds_SetFrontLeftBrightness(0u, 0u, 0u);
      Leds_SetFrontRightBrightness(0u, 0u, 0u);
      Leds_SetBackLeftBrightness(0u, 0u, 0u);
      Leds_SetBackRightBrightness(0u, 0u, 0u);
      break;

    case E_Mode_Follower:  // Green
      Leds_SetFrontLeftBrightness(0u, MAX_BRIGHTNESS, 0u);
      Leds_SetFrontRightBrightness(0u, MAX_BRIGHTNESS, 0u);
      Leds_SetBackLeftBrightness(0u, MAX_BRIGHTNESS, 0u);
      Leds_SetBackRightBrightness(0u, MAX_BRIGHTNESS, 0u);
      break;

    case E_Mode_Explorer:  // Yellow
      Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
      Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
      Leds_SetBackLeftBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
      Leds_SetBackRightBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
      break;

    case E_Mode_Attentive:  // Dark blue
      Leds_SetFrontLeftBrightness(0u, 0u, MAX_BRIGHTNESS);
      Leds_SetFrontRightBrightness(0u, 0u, MAX_BRIGHTNESS);
      Leds_SetBackLeftBrightness(0u, 0u, MAX_BRIGHTNESS);
      Leds_SetBackRightBrightness(0u, 0u, MAX_BRIGHTNESS);
      break;

    case E_Mode_LineTracker:  // Cyan
      Leds_SetFrontLeftBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetFrontRightBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetBackLeftBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetBackRightBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Obedient:  // Magenta
      Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      Leds_SetBackLeftBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      Leds_SetBackRightBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void RunExplorer(void)
{
  static int16_t speed = 150;

  int16_t brightness = GetBodyColorPulse();

  // Yellow pulse
  Leds_SetFrontLeftBrightness(brightness, brightness, 0u);
  Leds_SetFrontRightBrightness(brightness, brightness, 0u);
  Leds_SetBackLeftBrightness(brightness, brightness, 0u);
  Leds_SetBackRightBrightness(brightness, brightness, 0u);

  RunCircleLedRotation();

  // Buttons management
  SetSpeedUsingButtons(&speed);

  if (speed >= 0)
  {
    HandlePositiveSpeed(speed);
  }
  else
  {
    HandleNegativeSpeed(speed);
  }

  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, MAX_BRIGHTNESS);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, MAX_BRIGHTNESS);
  }
  else
  {
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0u);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0u);
  }
}

//_____________________________________________________________________________

static void RunAttentive(void)
{
  int16_t brightness = GetBodyColorPulse();

  // Dark blue pulse
  Leds_SetFrontLeftBrightness(0u, 0u, brightness);
  Leds_SetFrontRightBrightness(0u, 0u, brightness);
  Leds_SetBackLeftBrightness(0u, 0u, brightness);
  Leds_SetBackRightBrightness(0u, 0u, brightness);
}

//_____________________________________________________________________________

static void RunLineTracker(void)
{
  static uint8_t state[2] = {STATE_WHITE, STATE_WHITE};
  static int16_t dir = DIR_LOST;
  static uint16_t bs_black_level = 650; //400;
  static uint16_t bs_white_level = 700; //450;

  int16_t brightness = GetBodyColorPulse();

  // Cyan pulse
  Leds_SetFrontLeftBrightness(0u, brightness, brightness);
  Leds_SetFrontRightBrightness(0u, brightness, brightness);
  Leds_SetBackLeftBrightness(0u, brightness, brightness);
  Leds_SetBackRightBrightness(0u, brightness, brightness);

  if (!CalibrateLevelUsingButtons(&bs_black_level, &bs_white_level))
  {
    // Calibration is not in progress

    GetLineSensorsState(&bs_black_level, &bs_white_level, state);
#if 0
    T_Color color = ColorSensor_GetColor();

    switch (color)
    {
      case E_Color_Red:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, 0u);
        break;

      case E_Color_Orange:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 20u, 0u);
        break;

      case E_Color_Yellow:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
        break;

      case E_Color_Green:
        Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, 0u);
        break;

      case E_Color_Cyan:
        Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Blue:
        Leds_SetTopBrightness(0u, 0u, MAX_BRIGHTNESS);
        break;

      case E_Color_Purple:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
        break;

      case E_Color_White:
        Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        break;

      case E_Color_Unknown:
        Leds_SetTopBrightness(0u, 0u, 0u);
        break;

      default:
        Leds_SetTopBrightness(0u, 0u, 0u);
        break;
    }
#endif

    GetLineDirection(state, &dir);

    SetTargetAccordingToDirection(&dir);
  }
}

//_____________________________________________________________________________

static void RunFollower(void)
{
  static char sound_done;
  static char does_see_friend = 1;  // FIXME
  static unsigned char led_state;
  static char led_delta = 1;
  static int16_t speed = 300;

#define DETECT 500

  int i;
  int speed_diff;
  int speed_l = 0;
  int max, mi, t;

  max = vmVariables.prox[0];
  mi = 0;

  for (i = 1; i < 5; i++)
  {
    if (vmVariables.prox[i] > max)
    {
      max = vmVariables.prox[i];
      mi = i;
    }
  }

  t = (2 - mi);
  speed_diff = t * (speed / 2);

  if (max > 3500)
  {
    speed_l = (3500 - max) / 2;
  }

  if (max > 4000)
  {
    speed_l = -speed;
  }

  if (max < 3000)
  {
    t = 300 - ((max - 1000) / 7);
    speed_l = t;
  }

  if (max < 2000)
  {
    speed_l = speed;
  }

  if (speed_l > speed)
  {
    speed_l = speed;
  }

  if (speed_l < -speed)
  {
    speed_l = -speed;
  }

  if (max < DETECT)
  {
    //if (does_see_friend)
    {
      vmVariables.target[0] = speed;
      vmVariables.target[1] = speed;
    }
#if 0  // FIXME
    else
    {
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
    }
#endif
  }
  else
  {
    vmVariables.target[1] = (speed_diff + speed_l);
    vmVariables.target[0] = (speed_l - speed_diff);
  }

  if ((does_see_friend > 0) && sound_done)
  {
    unsigned char rgb[3];

    GetRainbow(rgb);

    // FIXME Leds_SetTopBrightness(rgb[0], rgb[1], rgb[2]);
    // FIXME Leds_SetBottomLeftBrightness(rgb[2], rgb[0], rgb[1]);
    // FIXME Leds_SetBottomRightBrightness(rgb[1], rgb[2], rgb[0]);
  }
  else
  {
    Leds_SetBodyBrightness(0, GetBodyColorPulse(), 0);
  }

  if (does_see_friend)
  {
    led_state += led_delta;

    if (led_state >= 31)
    {
      led_delta = -1;
    }
    else if (led_state == 0)
    {
      led_delta = 1;
    }
    else
    {
      // Do nothing
    }

    Leds_SetCircleBrightness(0, (led_state >> 4), (led_state >> 3), led_state, MAX_BRIGHTNESS, led_state, (led_state >> 3),
                             (led_state >> 4));
  }
  else
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u);
  }

  // Buttons management
  SetSpeedUsingButtons(&speed);

  when(max > DETECT)
  {
    // play_sound(SOUND_F_DETECT);  // FIXME
  }

  if (speed_diff == 0 && speed_l == 0 && sound_done == 0 && max > DETECT)
  {
    sound_done = 1;
    //play_sound(SOUND_F_OK);  // FIXME
  }

  if ((speed_diff != 0) || (max < DETECT))
  {
    sound_done = 0;
  }

  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, MAX_BRIGHTNESS);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, MAX_BRIGHTNESS);
  }
  else
  {
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0u);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0u);
  }

  if (does_see_friend)
  {
    does_see_friend--;
  }
#if 0  // FIXME
  if (IS_EVENT(EVENT_DATA))
  {
    CLEAR_EVENT(EVENT_DATA);
    does_see_friend = 0;
    mi = 0;
    max = vmVariables.intensity[0];
    vmVariables.intensity[0] = 0;

    for (i = 1; i < 7; i++)
    {
      if (vmVariables.intensity[i] > max)
      {
        mi = i;
        max = vmVariables.intensity[i];
      }

      vmVariables.intensity[i] = 0;
    }

    if (max > 3000)
    {
      vmVariables.ir_tx_data = mi;

      if ((vmVariables.rx_data > 0) && (vmVariables.rx_data < 4))
      {
        when(mi == 2)
        {
          play_sound(SOUND_F_OK);
        }

        if (mi == 2)
        {
          does_see_friend = 6;
        }
      }
    }
  }
#endif
}

//_____________________________________________________________________________

static void RunObedient(void)
{
  int16_t brightness = GetBodyColorPulse();

  // Magenta pulse
  Leds_SetFrontLeftBrightness(brightness, 0u, brightness);
  Leds_SetFrontRightBrightness(brightness, 0u, brightness);
  Leds_SetBackLeftBrightness(brightness, 0u, brightness);
  Leds_SetBackRightBrightness(brightness, 0u, brightness);

  if (!RecordSequenceIsFinished)
  {
    RecordButtonsSequence();
  }
  else
  {
    PlayMovementSequence();
  }

#if 0
  T_Color color = ColorSensor_GetColor();

  switch (color)
  {
    case E_Color_Red:
      Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, 0u);
      break;

    case E_Color_Orange:
      Leds_SetTopBrightness(MAX_BRIGHTNESS, 20u, 0u);
      break;

    case E_Color_Yellow:
      Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u);
      break;

    case E_Color_Green:
      Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, 0u);
      break;

    case E_Color_Cyan:
      Leds_SetTopBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Color_Blue:
      Leds_SetTopBrightness(0u, 0u, MAX_BRIGHTNESS);
      break;

    case E_Color_Purple:
      Leds_SetTopBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      break;

    case E_Color_White:
      Leds_SetTopBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Color_Unknown:
      Leds_SetTopBrightness(0u, 0u, 0u);
      break;

    default:
      Leds_SetTopBrightness(0u, 0u, 0u);
      break;
  }
#endif
}

//_____________________________________________________________________________

static void HandlePositiveSpeed(int16_t speed)
{
  int32_t temp1 = 0;
  int32_t temp2 = 0;

  temp1 += vmVariables.prox[0];
  temp1 += vmVariables.prox[1] * 2;
  temp1 += vmVariables.prox[2] * 3;
  temp1 += vmVariables.prox[3] * 2;
  temp1 += vmVariables.prox[4];

  temp2 += vmVariables.prox[0] * -4;
  temp2 += vmVariables.prox[1] * -3;
  temp2 += vmVariables.prox[3] * 3;
  temp2 += vmVariables.prox[4] * 4;

  //printf("speed = %d, temp1 = %d, temp2 = %d\n", speed, temp1, temp2);

  vmVariables.target[0] = speed - (((temp1 + temp2) * speed) / 2000); //2000);
  vmVariables.target[1] = speed - (((temp1 - temp2) * speed) / 2000); //2000);

  //printf("target = %d\n", vmVariables.target[0]);

  if (vmVariables.target[0] < -600)
  {
    vmVariables.target[0] = -600;
  }
  else if (vmVariables.target[0] > 600)
  {
    vmVariables.target[0] = 600;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < -600)
  {
    vmVariables.target[1] = -600;
  }
  else if (vmVariables.target[1] > 600)
  {
    vmVariables.target[1] = 600;
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static void HandleNegativeSpeed(int16_t speed)
{
  int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
  vmVariables.target[0] = speed + (temp / -300);

  temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
  vmVariables.target[1] = speed  + (temp / -300);

  if (vmVariables.target[0] < -600)
  {
    vmVariables.target[0] = -600;
  }
  else if (vmVariables.target[0] > 600)
  {
    vmVariables.target[0] = 600;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < -600)
  {
    vmVariables.target[1] = -600;
  }
  else if (vmVariables.target[1] > 600)
  {
    vmVariables.target[1] = 600;
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static int16_t GetBodyColorPulse(void)
{
  static int16_t led_pulse;
  int16_t ret;

  led_pulse++;

  if (led_pulse > 0)
  {
    ret = led_pulse;

    if (led_pulse >= MAX_BRIGHTNESS)
    {
      led_pulse = -(MAX_BRIGHTNESS * 4);
    }
  }
  else
  {
    ret = -led_pulse / 4;
  }

  //printf("p = %d, ret = %d\n", led_pulse, ret);

  return ret;
}

//_____________________________________________________________________________

static void GetRainbow(uint8_t* rgb)
{
  static uint8_t led_i;

  led_i++;

  if (led_i > 96)
  {
    led_i = 0;
  }

  rgb[0] = led_i;
  rgb[1] = (led_i + MAX_BRIGHTNESS);

  if (rgb[1] > 96)
  {
    rgb[1] -= 96;
  }

  rgb[2] = (led_i + 64);

  if (rgb[2] > 96)
  {
    rgb[2] -= 96;
  }

  rgb[0] = GetRainbowBrightness(rgb[0]);
  rgb[1] = GetRainbowBrightness(rgb[1]);
  rgb[2] = GetRainbowBrightness(rgb[2]);
}

//_____________________________________________________________________________

static uint8_t GetRainbowBrightness(uint8_t index)
{
  uint8_t brightness = 0u;

  if (index < 64u)
  {
    brightness = (64u - index);
  }
  else if (index < MAX_BRIGHTNESS)
  {
    brightness = index;
  }
  else
  {
    // Do nothing
  }

  return brightness;
}

//_____________________________________________________________________________

static void RunCircleLedRotation(void)
{
  static uint8_t led_state;
  uint8_t l[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t fixed;

  led_state += 2;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7] = MAX_BRIGHTNESS;
  l[(fixed - 1) & 0x7] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1)));
  l[(fixed + 1) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}

//_____________________________________________________________________________

static void RunCircleLedCross(void)
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

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}

//_____________________________________________________________________________

static void SetSpeedUsingButtons(int16_t* speed)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    *speed = (*speed + 50);

    if (*speed > 500)
    {
      *speed = 500;
    }
  }

  when(buttonState[E_Button_Backward])
  {
    *speed = (*speed - 50);

    if (*speed < -300)
    {
      *speed = -300;
    }
  }
}

//_____________________________________________________________________________

static bool CalibrateLevelUsingButtons(uint16_t* blackLevel, uint16_t* whiteLevel)
{
  uint8_t* buttonState;
  bool calibrationIsInProgress = false;

  buttonState = Buttons_GetStatus();

  //ESP_LOGI(Tag, "left = %d, right = %d", vmVariables.ground_delta[0], vmVariables.ground_delta[1]);

  // Calibration feature
  if (buttonState[E_Button_Backward] && buttonState[E_Button_Forward])
  {
    *blackLevel = (vmVariables.ground_delta[0] + vmVariables.ground_delta[1]) / 2;
    *blackLevel += 150u;
    calibrationIsInProgress = true;
  }

  if (buttonState[E_Button_Left] && buttonState[E_Button_Right])
  {
    *whiteLevel = (vmVariables.ground_delta[0] + vmVariables.ground_delta[1]) / 2;

    if (*whiteLevel < 150u)
    {
      *whiteLevel = 200u;
    }

    *whiteLevel -= 150u;
    calibrationIsInProgress = true;
  }

  // if the user is trying to calibrate, then don't try to move
  if (calibrationIsInProgress)
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
  }

  return calibrationIsInProgress;
}

//_____________________________________________________________________________

static void GetLineSensorsState(uint16_t* blackLevel, uint16_t* whiteLevel, uint8_t* state)
{
  if (vmVariables.ground_delta[0] < *blackLevel)
  {
    state[0] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[0] > *whiteLevel)
  {
    state[0] = STATE_WHITE;
  }

  if (vmVariables.ground_delta[1] < *blackLevel)
  {
    state[1] = STATE_BLACK;
  }

  if (vmVariables.ground_delta[1] > *whiteLevel)
  {
    state[1] = STATE_WHITE;
  }
}

//_____________________________________________________________________________

static void GetLineDirection(uint8_t* state, int16_t* direction)
{
  if ((state[0] == STATE_BLACK) && (state[1] == STATE_BLACK))
  {
    // Black line right under us
    *direction = DIR_FRONT;
  }
  else if ((state[0] == STATE_WHITE) && (state[1] == STATE_BLACK))
  {
    *direction = DIR_RIGHT;
  }
  else if ((state[1] == STATE_WHITE) && (state[0] == STATE_BLACK))
  {
    *direction = DIR_LEFT;
  }
  else
  {
    // Lost the line
    if (*direction > 0)
    {
      *direction = DIR_L_RIGHT;
    }
    else if (*direction < 0)
    {
      *direction = DIR_L_LEFT;
    }
    else
    {
      *direction = DIR_LOST;
    }
  }
}

//_____________________________________________________________________________

static void SetTargetAccordingToDirection(int16_t* direction)
{
  if (*direction == DIR_FRONT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }
  else if (*direction == DIR_RIGHT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = 0;
    Leds_SetCircleBrightness(0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u);
  }
  else if (*direction == DIR_LEFT)
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
  }
  else if (*direction == DIR_L_LEFT)
  {
    vmVariables.target[0] = -SPEED_LINE;
    vmVariables.target[1] = SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
  }
  else if (*direction == DIR_L_RIGHT)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = -SPEED_LINE;
    Leds_SetCircleBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
  }
  else if (*direction == DIR_LOST)
  {
    vmVariables.target[0] = SPEED_LINE;
    vmVariables.target[1] = -SPEED_LINE;
    //leds_set_circle(MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS);
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static void RecordButtonsSequence(void)
{
  uint8_t* buttonState;
  uint8_t  tap = Accelerometer_GetTapSource();
  uint8_t  data = 0u;

  RunCircleLedCross();

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Backward])
  {
    data |= (1 << E_Button_Backward);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Left])
  {
    data |= (1 << E_Button_Left);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Forward])
  {
    data |= (1 << E_Button_Forward);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Right])
  {
    data |= (1 << E_Button_Right);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(tap)
  {
    if (!Fifo8bits_IsEmpty(ButtonsSeqFifo))  // Ready to play a new sequence
    {
      ESP_LOGI(Tag, "Fifo is NOT empty");
    }
    else  // Ready to play the old sequence
    {
      ESP_LOGI(Tag, "Fifo is empty %d", Position);
      Fifo8bits_SetConsumePosition(ButtonsSeqFifo, Position);
    }

    RecordSequenceIsFinished = true;
    MovementIsStarted = false;
    ESP_LOGI(Tag, "End of recording");
  }
}

//_____________________________________________________________________________

static void PlayMovementSequence(void)
{
  uint8_t  data = 0u;
  uint8_t  next = 0u;

  if (!Fifo8bits_IsEmpty(ButtonsSeqFifo))
  {
    if (!MovementIsInProgress && !MovementTimerIsRunning && !StopTimerIsRunning)
    {
      if (!MovementIsStarted)
      {
        Position = Fifo8bits_GetConsumePosition(ButtonsSeqFifo);
        MovementIsStarted = true;
      }

      Fifo8bits_Read(ButtonsSeqFifo, &data, 1);
      Fifo8bits_Peek(ButtonsSeqFifo, &next, 1);

      if (data == (1 << E_Button_Backward))
      {
        TimerSw_StartTimerOnce(MovementTimer, MOVEMENT_DURATION_us);
        MovementIsInProgress = true;
        MovementTimerIsRunning = true;

        vmVariables.target[0] = -260;  // TODO Check why the speed in backward direction is slower
        vmVariables.target[1] = -260;
      }

      if (data == (1 << E_Button_Forward))
      {
        TimerSw_StartTimerOnce(MovementTimer, MOVEMENT_DURATION_us);
        MovementIsInProgress = true;
        MovementTimerIsRunning = true;

        vmVariables.target[0] = 240;
        vmVariables.target[1] = 240;
      }

      if (next == 0)
      {
        Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u);
      }

      if (next == (1 << E_Button_Backward))
      {
        Leds_SetCircleBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
      }

      if (next == (1 << E_Button_Forward))
      {
        Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
      }
    }
    else if (MovementIsInProgress && !MovementTimerIsRunning && !StopTimerIsRunning)
    {
      TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_us);
      StopTimerIsRunning = true;
    }
  }
  else if (!MovementTimerIsRunning)  // Handle the last stop delay
  {
    TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_us);
    StopTimerIsRunning = true;

    RecordSequenceIsFinished = false;  // Allow a new buttons recording sequence
  }
}

//_____________________________________________________________________________

static void Callback_TimerMovement(void* arg)
{
  if (MovementIsInProgress)
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;

    //MovementIsInProgress = false;
    MovementTimerIsRunning = false;
  }
}

//_____________________________________________________________________________

static void Callback_TimerStop(void* arg)
{
  MovementIsInProgress = false;
  StopTimerIsRunning = false;
}
