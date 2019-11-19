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

#include "esp_log.h"

#include "mode.h"

#include "accelerometer.h"
#include "angle_controller.h"
#include "aseba_esp32.h"
#include "behavior.h"
#include "buttons.h"
#include "codec.h"
#include "color_sensor.h"
#include "fifo.h"
#include "gyroscope.h"
#include "leds.h"
#include "rc5.h"
#include "stm32_i2c.h"
#include "tcp_server.h"
#include "timer_sw.h"

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

#define ACC_OBSTACLE                175
#define ACC_FREE_FALL              1000

#define DETECT                       85

#define MIN_SPEED                  -600
#define MAX_SPEED                   600

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
static bool RotationIsInProgress = false;
static bool MovementTimerIsRunning = false;
static bool StopTimerIsRunning = false;
static uint16_t Position = 0u;

static T_TimerSw* MovementTimer = NULL;  //!< Used to move the robot in obedient mode
static T_TimerSw* StopTimer = NULL;      //!< Used to stop the robot in obedient mode

static bool VMIsActive = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void StartMode(T_Mode mode);

static void ExitMode(T_Mode mode);

static T_Mode SelectNextMode(T_Mode mode, int16_t index);

static bool IsModeEnabled(T_Mode mode);

static void SetModeColor(T_Mode mode);

static void RunFollower(void);

static void RunExplorer(void);

static void RunFearful(void);

static void RunAttentive(void);

static void RunLineTracker(void);

static void RunObedient(void);

static void HandlePositiveSpeed(int16_t speed);

static void HandleNegativeSpeed(int16_t speed);

static void LimitSpeed(int16_t min, int16_t max);

static uint8_t GetBodyColorPulse(void);

static void GetRainbow(uint8_t* rgb);

static uint8_t GetRainbowBrightness(uint8_t index);

static void RunCircleLedRotation(void);

static void RunCircleLedCross(void);

static void RunLegoLedAnimation(void);

static void SetSpeedUsingButtons(int16_t* speed);

static bool CalibrateLevelUsingButtons(uint16_t* blackLevel, uint16_t* whiteLevel);

static void GetLineSensorsState(uint16_t* blackLevel, uint16_t* whiteLevel, uint8_t* state);

static void GetLineDirection(uint8_t* state, int16_t* direction);

static void SetTargetAccordingToDirection(int16_t* direction);

//! \brief     Record the movement sequence in the obedient mode
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RecordMovementSequence(void);

//! \brief     Play the movement sequence in the obedient mode
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void PlayMovementSequence(void);

static void Callback_TimerMovement(void* arg);

static void Callback_TimerStop(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Mode_Init(bool enableVM)
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

  ButtonsSeqFifo = Fifo8bits_Create(ButtonsSeqBuffer, BUTTONS_SEQ_BUFFER_SIZE);
  RecordSequenceIsFinished = false;
  MovementIsStarted = false;
  MovementIsInProgress = false;
  RotationIsInProgress = false;
  MovementTimerIsRunning = false;
  StopTimerIsRunning = false;
  MovementTimer = TimerSw_Create(MOVEMENT_DURATION_us, Callback_TimerMovement);
  StopTimer = TimerSw_Create(STOP_DURATION_us, Callback_TimerStop);

  VMIsActive = enableVM;

  StartMode(E_Mode_Menu);

  if (VMIsActive)
  {
    SelectMode = E_Mode_Menu;
  }
  else
  {
	SelectMode = E_Mode_Follower;
	SetModeColor(SelectMode);
  }

  Behavior_Enable(B_ALWAYS | B_MODE);

  ESP_LOGI(Tag, "Mode is initialized");
}

//_____________________________________________________________________________

void Mode_InitVM(void)
{
  Behavior_Enable(B_LEDS_ACC);
  Behavior_Enable(B_LEDS_LEGO);
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
  if (ignore > 100u)
  {
    ignore = 101;

    when(buttonState[E_Button_Center])
    {
      ExitMode(CurrentMode);

      if (SelectMode == E_Mode_Menu)
      {
        // Special case, if we select the mode menu stuff
        Behavior_Disable(B_MODE | B_SETTING);
        Mode_InitVM();
        return;
      }

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

    case E_Mode_Fearful:
      RunFearful();
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
      Behavior_Enable(B_SETTING);
      break;

    case E_Mode_Follower:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Explorer:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Fearful:
      Behavior_Enable(B_LEDS_PROX);
      Behavior_Enable(B_LEDS_ACC);
      Behavior_Enable(B_LEDS_LEGO);
      Accelerometer_ClearTapStatus();  // Clear any tap made before entering this mode
      break;

    case E_Mode_Attentive:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_LineTracker:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      Behavior_Enable(B_LEDS_PROX);
      Behavior_Enable(B_LEDS_LEGO);
      RecordSequenceIsFinished = false;
      Accelerometer_ClearTapStatus();  // Clear any tap made before entering this mode
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
  Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);

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

    case E_Mode_Fearful:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Disable(B_LEDS_PROX);
      Behavior_Disable(B_LEDS_ACC);
      Behavior_Disable(B_LEDS_LEGO);
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
      Behavior_Disable(B_LEDS_LEGO);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static T_Mode SelectNextMode(T_Mode mode, int16_t index)
{
  int16_t temp = (int16_t)mode;

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

  if ((mode == E_Mode_Menu) && !VMIsActive)
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
      Leds_SetBodyBrightness(0u, 0u, 0u);
      break;

    case E_Mode_Follower:  // Green
      Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);
      break;

    case E_Mode_Explorer:  // Yellow
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 12, 0u);
      break;

    case E_Mode_Fearful:  // Red
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
      break;

    case E_Mode_Attentive:  // Dark blue
      Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
      break;

    case E_Mode_LineTracker:  // Cyan
      Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Obedient:  // Magenta
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void RunFollower(void)
{
  int16_t max = vmVariables.prox[0];
  int16_t min = 0;
  int16_t t;
  uint8_t brightness = GetBodyColorPulse();
  int16_t speedDiff;
  int16_t speed_l = 0;

  static int16_t speed = 300;

  // Green pulse
  Leds_SetBodyBrightness(0u, brightness, 0u);

  for (uint8_t index = 1u; index < 5u; index++)
  {
    if (vmVariables.prox[index] > max)
    {
      max = vmVariables.prox[index];
      min = index;
    }
  }

  t = 2 - min;
  speedDiff = t * (speed / 2);

  if (max > 600)
  {
    speed_l = (600 - max) / 2;
  }

  if (max > 700)
  {
    speed_l = -speed;
  }

  if (max < 520)
  {
    t = 52 - ((max - 175) / 7);
    speed_l = t;
  }

  if (max < 350)
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
#if 0  // FIXME
    if (does_see_friend)
    {
      vmVariables.target[0] = speed;
      vmVariables.target[1] = speed;
    }
    else
#endif
    {
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
    }
  }
  else
  {
    vmVariables.target[1] = (speedDiff + speed_l);
    vmVariables.target[0] = (speed_l - speedDiff);
  }

  when(max > DETECT)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Detection);
  }

#if 0
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
#endif
}

//_____________________________________________________________________________

static void RunExplorer(void)
{
  static int16_t speed = 150;

  uint8_t brightness = GetBodyColorPulse();

  // Yellow pulse
  Leds_SetBodyBrightness(brightness, brightness, 0u);

//#if 0
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
// FIXME    vmVariables.target[0] = 0;
// FIXME    vmVariables.target[1] = 0;
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, MAX_BRIGHTNESS);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, MAX_BRIGHTNESS);
  }
  else
  {
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0u);
    // FIXME Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0u);
  }
//#endif
}

//_____________________________________________________________________________

static void RunFearful(void)
{
  uint8_t brightness = GetBodyColorPulse();
  bool play = false;
//  static unsigned int acc = 32;
//  static uint8_t counter = 0u;

  // Red pulse
  //Leds_SetBodyBrightness(brightness, 0u, 0u);

  //acc = acc + acc + acc + abs(vmVariables.acc[0]) + abs(vmVariables.acc[1]) + abs(vmVariables.acc[2]);
  //acc >>= 2;
  //acc = abs(vmVariables.acc[0]) + abs(vmVariables.acc[1]) + abs(vmVariables.acc[2]);

  //if (acc < ACC_FREE_FALL)
  if (Accelerometer_IsFreeFallDetected())
  {
    //ESP_LOGE(Tag, "acc = %d", acc);
    ESP_LOGE(Tag, "FREE FALL DETECTED");
    play = true;
  }

#if 0
  when(acc > ACC_FREE_FALL)
  {
    Leds_SetBodyBrightness((MAX_BRIGHTNESS / 2), 0u, 0u);
  }

  if (acc < ACC_FREE_FALL)
  {
    counter++;

    if (counter > 5)
    {
      if (counter == 10)
      {
        counter = 0;
      }

      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
    }
    else
    {
      Leds_SetBodyBrightness(0u, 0u, 0u);
    }
  }
  else
  {
    // Red pulse
    Leds_SetBodyBrightness(brightness, 0u, 0u);
  }
#endif

#if 0
  if (Accelerometer_IsTapDetected())
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Tick);
  }
#endif

//#if 0
  // Moving part.
  if ((vmVariables.prox[1] > ACC_OBSTACLE) && (vmVariables.prox[2] > ACC_OBSTACLE)
      && (vmVariables.prox[3] > ACC_OBSTACLE) &&
      ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))) //&&
    //(vmVariables.ground_delta[0] > 130 && vmVariables.ground_delta[1] > 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    play = true;
  }
  else if ((vmVariables.prox[0] > ACC_OBSTACLE) || (vmVariables.prox[1] > ACC_OBSTACLE) ||
           (vmVariables.prox[2] > ACC_OBSTACLE) || (vmVariables.prox[3] > ACC_OBSTACLE) ||
           (vmVariables.prox[4] > ACC_OBSTACLE))
  {
    //int temp = vmVariables.prox[0]/5 + vmVariables.prox[1]/4 + vmVariables.prox[2]/4;
    //temp += vmVariables.prox[3]/4 + vmVariables.prox[4]/5;
    int16_t temp = (vmVariables.prox[0] / 3) + (vmVariables.prox[1] / 2) + (vmVariables.prox[2] / 2);
    temp += (vmVariables.prox[3] / 2) + (vmVariables.prox[4] / 3);

    int16_t temp2 = (vmVariables.prox[0] / 4) + (vmVariables.prox[1] / 3);
    temp2 -= (vmVariables.prox[3] / 3) + (vmVariables.prox[4] / 4);

    vmVariables.target[0] = -(temp + temp2);
    vmVariables.target[1] = temp2 - temp;

    //ESP_LOGE(Tag, "target_left = %d, target_right = %d", vmVariables.target[0], vmVariables.target[1]);
  }
  else if ((vmVariables.prox[5] > ACC_OBSTACLE) || (vmVariables.prox[6] > ACC_OBSTACLE))
  {
    vmVariables.target[0] = vmVariables.prox[5] / 2;
    vmVariables.target[1] = vmVariables.prox[6] / 2;
  }
  else
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
  }
#if 0
  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    leds_set_br(32, 0, 0);
    leds_set_bl(32, 0, 0);
  }
  else
  {
    leds_set_br(0, 0, 0);
    leds_set_bl(0, 0, 0);
  }
#endif
  LimitSpeed(MIN_SPEED, MAX_SPEED);
//#endif
  when(play)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Fall);
  }
}

//_____________________________________________________________________________

// When the Thymio is placed on the left side, the WAV recorder is activated.
// When the Thymio is placed on the right side, the WAV player is activated (replay).
static void RunAttentive(void)
{
  uint8_t brightness = GetBodyColorPulse();
  int16_t acceleration = Accelerometer_GetAccelerationY();

  // Dark blue pulse
  Leds_SetBodyBrightness(0u, 0u, brightness);

  when(acceleration >= 15000)  // Left side
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Startup);
  }

  when(acceleration <= -15000)  // Right side
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Bye);
  }
}

//_____________________________________________________________________________

static void RunLineTracker(void)
{
  static uint8_t state[2] = {STATE_WHITE, STATE_WHITE};
  static int16_t dir = DIR_LOST;
  static uint16_t bs_black_level = 650; //400;
  static uint16_t bs_white_level = 700; //450;

  uint8_t brightness = GetBodyColorPulse();

  // Cyan pulse
  Leds_SetBodyBrightness(0u, brightness, brightness);

#if 0
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
#endif
}

//_____________________________________________________________________________

static void RunObedient(void)
{
  uint8_t brightness = GetBodyColorPulse();

  // Magenta pulse
  Leds_SetBodyBrightness(brightness, 0u, brightness);

  if (!RecordSequenceIsFinished)
  {
    RecordMovementSequence();
  }
  else
  {
    PlayMovementSequence();
  }
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

  vmVariables.target[0] = speed - (((temp1 + temp2) * speed) / 200); //2000);
  vmVariables.target[1] = speed - (((temp1 - temp2) * speed) / 200); //2000);

  //printf("target = %d\n", vmVariables.target[0]);
  LimitSpeed(MIN_SPEED, MAX_SPEED);
}

//_____________________________________________________________________________

static void HandleNegativeSpeed(int16_t speed)
{
  int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
  vmVariables.target[0] = speed + (temp / -300);

  temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
  vmVariables.target[1] = speed  + (temp / -300);

  LimitSpeed(MIN_SPEED, MAX_SPEED);
}

//_____________________________________________________________________________

static void LimitSpeed(int16_t min, int16_t max)
{
  if (vmVariables.target[0] < min)
  {
    vmVariables.target[0] = min;
  }
  else if (vmVariables.target[0] > max)
  {
    vmVariables.target[0] = max;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < min)
  {
    vmVariables.target[1] = min;
  }
  else if (vmVariables.target[1] > max)
  {
    vmVariables.target[1] = max;
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static uint8_t GetBodyColorPulse(void)
{
  static int16_t pulse = 0;
  int16_t brightness = 0;

  pulse++;

  if (pulse > 0)
  {
	brightness = pulse;

    if (pulse >= MAX_BRIGHTNESS)
    {
      pulse = -(MAX_BRIGHTNESS * 4);
    }
  }
  else
  {
    brightness = -pulse / 4;
  }

  return (uint8_t)brightness;
}

//_____________________________________________________________________________

static void GetRainbow(uint8_t* rgb)
{
  static uint8_t led = 0u;

  led++;

  if (led > 96u)
  {
    led = 0u;
  }

  rgb[0] = led;
  rgb[1] = (led + MAX_BRIGHTNESS);

  if (rgb[1] > 96u)
  {
    rgb[1] -= 96u;
  }

  rgb[2] = (led + 64u);

  if (rgb[2] > 96u)
  {
    rgb[2] -= 96u;
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
  static uint8_t led_state = 0u;
  uint8_t l[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint8_t fixed;

  led_state += 2u;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7] = MAX_BRIGHTNESS;
  l[(fixed - 1) & 0x7] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1)));
  l[(fixed + 1) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}

//_____________________________________________________________________________

static void RunCircleLedCross(void)
{
  static uint8_t led_state = 0u;
  uint8_t l[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint8_t fixed;

  led_state += 2u;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7] = MAX_BRIGHTNESS;
  l[(fixed - 2) & 0x7] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1)));
  l[(fixed + 2) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));
  l[(fixed + 4) & 0x7] = (led_state & (MAX_BRIGHTNESS - 1));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
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

static void RecordMovementSequence(void)
{
  uint8_t* buttonState;
  uint8_t  data = 0u;
  int16_t command = 0;

  static int16_t toggle = 0;

  RunCircleLedCross();
  //RunLegoLedAnimation();

  buttonState = Buttons_GetStatus();

  if (RC5_IsNewMessageReceived(&toggle))
  {
    command = RC5_GetCommand();
  }

  when(buttonState[E_Button_Backward] || (command == E_Command_DownArrow))
  {
    data |= (1 << E_Button_Backward);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Left] || (command == E_Command_LeftArrow))
  {
    data |= (1 << E_Button_Left);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Forward] || (command == E_Command_UpArrow))
  {
    data |= (1 << E_Button_Forward);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when(buttonState[E_Button_Right] || (command == E_Command_RightArrow))
  {
    data |= (1 << E_Button_Right);
    Fifo8bits_Write(ButtonsSeqFifo, &data, 1u);
  }

  when (Accelerometer_IsTapDetected() || (command == E_Command_Go))
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

    //Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
    RecordSequenceIsFinished = true;
    MovementIsStarted = false;
    ESP_LOGI(Tag, "End of recording");
  }
}

//_____________________________________________________________________________

static void PlayMovementSequence(void)
{
  uint8_t data = 0u;
  uint8_t next = 0u;

  static int16_t angleTarget = 0;

  if (!Fifo8bits_IsEmpty(ButtonsSeqFifo))
  {
    if (!MovementIsInProgress && !MovementTimerIsRunning && !StopTimerIsRunning && !RotationIsInProgress)
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

        vmVariables.target[0] = -300;  // TODO Check why the speed in backward direction is slower
        vmVariables.target[1] = -300;
      }

      if (data == (1 << E_Button_Forward))
      {
        TimerSw_StartTimerOnce(MovementTimer, MOVEMENT_DURATION_us);
        MovementIsInProgress = true;
        MovementTimerIsRunning = true;

        vmVariables.target[0] = 300;
        vmVariables.target[1] = 300;
      }

      if (data == (1 << E_Button_Left))
      {
        RotationIsInProgress = true;
        angleTarget = 16383;  //90;
        Gyroscope_ResetAngle();
      }

      if (data == (1 << E_Button_Right))
      {
        RotationIsInProgress = true;
        angleTarget = -16383;  //-90;
        Gyroscope_ResetAngle();
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

      if (next == (1 << E_Button_Left))
      {
        Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
      }

      if (next == (1 << E_Button_Right))
      {
        Leds_SetCircleBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
      }
    }
    else if (MovementIsInProgress && !MovementTimerIsRunning && !StopTimerIsRunning)
    {
      TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_us);
      StopTimerIsRunning = true;
    }
    else if (RotationIsInProgress)
    {
      if (AngleController_Update(angleTarget, 500) == 0)
      {
        RotationIsInProgress = false;
      }
    }
    else
    {
      // Do nothing
    }
  }
  else if (!MovementTimerIsRunning && !RotationIsInProgress)  // Handle the last stop delay
  {
    RecordSequenceIsFinished = false;  // Allow a new buttons recording sequence
    ESP_LOGE(Tag, "Last movement finished");
  }
  else if (RotationIsInProgress)  // Handle the rotation at the end of the sequence
  {
    if (AngleController_Update(angleTarget, 500) == 0)
    {
      RotationIsInProgress = false;
      RecordSequenceIsFinished = false;  // Allow a new buttons recording sequence
      ESP_LOGE(Tag, "Last rotation finished");
    }
  }
  else
  {
    // Do nothing
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
