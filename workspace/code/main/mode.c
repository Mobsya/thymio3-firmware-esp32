//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: mode.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>  // Only for debug

#include "esp_log.h"

#include "mode.h"

#include "aseba_esp32.h"
#include "behavior.h"
#include "leds.h"
#include "stm32.h"

#include "mp3.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void StartMode(T_Mode mode);

static void ExitMode(T_Mode mode);

static T_Mode SelectNextMode(T_Mode mode, int16_t index);

static bool IsModeEnabled(T_Mode mode);

static void SetModeColor(T_Mode mode);

static int16_t GetBodyColorPulse(void);

static void GetRainbow(uint8_t* rgb);

static uint8_t GetRainbowBrightness(uint8_t index);

static void SetSpeedUsingButtons(int16_t* speed);

static void RunExplorer(void);

static void RunFollower(void);

static void RunInvestigator(void);

static void RunObedient(void);

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

  Behavior_Start(B_ALWAYS | B_MODE);

  ESP_LOGI(Tag, "Mode is initialized");
}

//_____________________________________________________________________________

void Mode_InitVM(void)
{
  Behavior_Start(B_LEDS_ACC);
  Behavior_Start(B_LEDS_PROX);

  ESP_LOGI(Tag, "VM Mode is initialized");
}

//_____________________________________________________________________________

void Mode_Run(void)
{
  uint8_t* buttonState;

  static uint8_t ignore;

  buttonState = STM32_GetButtonStatus();

  ignore++;

  // As the user mode disable the "mode menu thing"...
  if (ignore > 100)
  {
    ignore = 101;

    when(buttonState[E_Button_Center])
    {
      ExitMode(CurrentMode);
#if 0  // FIXME
      if (SelectMode == E_Mode_Menu)
      {
        // Special case, if we select the mode menu stuff
        Behavior_Stop(B_MODE); // | B_SETTING);  FIXME
        Mode_InitVM();
        return;
      }
#endif

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

    case E_Mode_Explorer:
      RunExplorer();
      break;

    case E_Mode_Investigator:
      RunInvestigator();
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
  //SetModeColor(mode);

  switch (mode)
  {
    case E_Mode_Menu:
      break;

    case E_Mode_Explorer:
      Behavior_Start(B_LEDS_PROX);
      break;

    case E_Mode_Investigator:
      //Behavior_Start(B_LEDS_PROX);
      //MP3_Init();
      break;

    case E_Mode_Obedient:
      //Behavior_Start(B_LEDS_PROX);
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

  switch (mode)
  {
    case E_Mode_Menu:
      break;

    case E_Mode_Explorer:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Stop(B_LEDS_PROX);
      break;

    case E_Mode_Investigator:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Stop(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      vmVariables.target[0] = 0;
      vmVariables.target[1] = 0;
      Behavior_Stop(B_LEDS_PROX);
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
      Leds_SetTopBrightness(0u, 0u, 0u);
      break;

    case E_Mode_Explorer:
      Leds_SetTopBrightness(32u, 32u, 0u);
      break;

    case E_Mode_Investigator:
      Leds_SetTopBrightness(0u, 32u, 32u);
      break;

    case E_Mode_Obedient:
      Leds_SetTopBrightness(32u, 0u, 32u);
      break;

    default:
      // Do nothing
      break;
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

    if (led_pulse >= 32)
    {
      led_pulse = -128;
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
  rgb[1] = (led_i + 32);

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
  else if (index < 32u)
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

static void SetSpeedUsingButtons(int16_t* speed)
{
  uint8_t* buttonState;

  buttonState = STM32_GetButtonStatus();

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

static void RunExplorer(void)
{
  static uint8_t led_state;
  static int16_t speed = 150;

  uint8_t l[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t fixed;

  int16_t p = GetBodyColorPulse();

  Leds_SetTopBrightness(p, p, 0u);

  // circle led management
  led_state += 2;
  fixed = led_state / 32;
  l[fixed] = 32;
  l[(fixed - 1) & 0x7] = 32 - (led_state & 0x1F);
  l[(fixed + 1) & 0x7] = led_state & 0x1F;
  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);

  // Buttons management
  SetSpeedUsingButtons(&speed);

  if (speed >= 0)
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

    vmVariables.target[0] = speed - (((temp1 + temp2) * speed) / 500); //2000);
    vmVariables.target[1] = speed - (((temp1 - temp2) * speed) / 500); //2000);

    //printf("target = %d\n", vmVariables.target[0]);

    if (vmVariables.target[0] < -600)
    {
      vmVariables.target[0] = -600;
    }

    if (vmVariables.target[1] < -600)
    {
      vmVariables.target[1] = -600;
    }

    if (vmVariables.target[0] > 600)
    {
      vmVariables.target[0] = 600;
    }

    if (vmVariables.target[1] > 600)
    {
      vmVariables.target[1] = 600;
    }
  }
  else
  {
    int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
    vmVariables.target[0] = speed + (temp / -300);

    temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
    vmVariables.target[1] = speed  + (temp / -300);

    if (vmVariables.target[0] < -600)
    {
      vmVariables.target[0] = -600;
    }

    if (vmVariables.target[1] < -600)
    {
      vmVariables.target[1] = -600;
    }

    if (vmVariables.target[0] > 600)
    {
      vmVariables.target[0] = 600;
    }

    if (vmVariables.target[1] > 600)
    {
      vmVariables.target[1] = 600;
    }
  }

  if ((vmVariables.ground_delta[0] < 130) || (vmVariables.ground_delta[1] < 130))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 32);
    Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 32);
  }
  else
  {
    Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0);
    Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0);
  }
}

//_____________________________________________________________________________

static void RunFollower(void)
{
  static char sound_done;
  static char does_see_friend = 1;  // FIxME
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

    Leds_SetTopBrightness(rgb[0], rgb[1], rgb[2]);
    Leds_SetBottomLeftBrightness(rgb[2], rgb[0], rgb[1]);
    Leds_SetBottomRightBrightness(rgb[1], rgb[2], rgb[0]);
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

    Leds_SetCircleBrightness(0, (led_state >> 4), (led_state >> 3), led_state, 32, led_state, (led_state >> 3),
                             (led_state >> 4));
  }
  else
  {
    Leds_SetCircleBrightness(0, 0, 0, 32, 32, 32, 0, 0);
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
    Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 32);
    Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 32);
  }
  else
  {
    Leds_SetSingleBrightness(E_Led_R_Bottom_Left, 0);
    Leds_SetSingleBrightness(E_Led_R_Bottom_Right, 0);
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
  int16_t p = GetBodyColorPulse();

  Leds_SetTopBrightness(p, 0u, p);
}

//_____________________________________________________________________________

static void RunInvestigator(void)
{
  int16_t p = GetBodyColorPulse();

  Leds_SetTopBrightness(0u, p, p);
}
