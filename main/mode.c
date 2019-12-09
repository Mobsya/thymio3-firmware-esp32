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

#include "behavior.h"
#include "buttons.h"
#include "common.h"
#include "gpio.h"
#include "leds.h"
#include "stm32_spi.h"
#include "tcp_server.h"

// Include of the modes
//#include "attentive.h"
#include "drawer.h"
#include "explorer.h"
#include "fearful.h"
#include "follower.h"
#include "line_tracker.h"
#include "responsive.h"

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

static bool VMIsActive = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void StartMode(T_Mode mode);

static void ExitMode(T_Mode mode);

static T_Mode SelectNextMode(T_Mode mode, int16_t index);

static bool IsModeEnabled(T_Mode mode);

static void SetModeColor(T_Mode mode);

#if 0
static void GetRainbow(uint8_t* rgb);

static uint8_t GetRainbowBrightness(uint8_t index);

static void RunLegoLedAnimation(void);
#endif

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Mode_Init(bool enableVM)
{
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

  Responsive_Init();

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
  //uint8_t sideState = Buttons_GetSideStatus();
  bool sideState = Gpio_IsButtonPressed();

  ignore++;

  // As the user mode disable the "mode menu thing"...
  if (ignore > 100u)
  {
    ignore = 101;

    // Enter into a mode
    when(buttonState[E_Button_Center])
    {
      ExitMode(CurrentMode);

      if (SelectMode == E_Mode_Menu)
      {
        // Special case, if we select the mode menu stuff
        Behavior_Disable(B_MODE | B_SETTING);
        Mode_InitVM();
        ESP_LOGE(Tag, "B");
        return;
      }

      if (SelectMode != CurrentMode)
      {
        StartMode(SelectMode);
        CurrentMode = SelectMode;
      }
    }

    // Exit from a mode
    when(sideState)
    {
      ExitMode(CurrentMode);

      if (SelectMode == E_Mode_Menu)
      {
        // Special case, if we select the mode menu stuff
        Behavior_Disable(B_MODE | B_SETTING);
        Mode_InitVM();
        ESP_LOGE(Tag, "E");
        return;
      }

      if (SelectMode == CurrentMode)
      {
        StartMode(E_Mode_Menu);
        CurrentMode = E_Mode_Menu;
      }
      //else
      {
        //StartMode(SelectMode);
        //CurrentMode = SelectMode;
        //ESP_LOGE(Tag, "D");
      }

      //Gpio_ClearButtonStatus();
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
      Follower_Run();
      break;

    case E_Mode_Explorer:
      Explorer_Run();
      break;

    case E_Mode_Fearful:
      Fearful_Run();
      break;

    case E_Mode_Drawer:
      //Attentive_Run();
      Drawer_Run();
      break;

    case E_Mode_LineTracker:
      LineTracker_Run();
      break;

    case E_Mode_Responsive:
      Responsive_Run();
      break;

    case E_Mode_Musician:
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
      Fearful_Start();
      break;

    case E_Mode_Drawer:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_LineTracker:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Responsive:
      Behavior_Enable(B_LEDS_PROX);
      Behavior_Enable(B_LEDS_LEGO);
      Responsive_Start();
      break;

    case E_Mode_Musician:
      Behavior_Enable(B_LEDS_PROX);
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
      Follower_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Explorer:
      Explorer_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Fearful:
      Fearful_Stop();
      Behavior_Disable(B_LEDS_PROX);
      Behavior_Disable(B_LEDS_ACC);
      Behavior_Disable(B_LEDS_LEGO);
      break;

    case E_Mode_Drawer:
      //Attentive_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_LineTracker:
      LineTracker_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Responsive:
      Responsive_Stop();
      Behavior_Disable(B_LEDS_PROX);
      Behavior_Disable(B_LEDS_LEGO);
      break;

    case E_Mode_Musician:
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
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 12u, 0u);
      break;

    case E_Mode_Fearful:  // Red
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
      break;

    case E_Mode_Drawer:  // Dark blue
      Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
      break;

    case E_Mode_LineTracker:  // Cyan
      Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Responsive:  // Magenta
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      break;

    case E_Mode_Musician:  // White
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________
#if 0
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
#endif
