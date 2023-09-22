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
#include "explorer.h"
#include "fearful.h"
#include "friendly.h"
#include "line_tracker.h"
#include "musician.h"
#include "painter.h"
#include "sequence.h"
#include "python_handler.h"
#include "mp_component.h"
#include "codec.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Mode_Menu,
  E_Mode_Friendly,
  E_Mode_Explorer,
  E_Mode_Fearful,
  E_Mode_Attentive,
  E_Mode_Investigator,
  E_Mode_Obedient,  
  E_Mode_Painter,  
  E_Mode_Sequence,
  E_Mode_Musician,
  E_Mode_NN,
  E_Mode_Python_REPL,
  E_Mode_Python_Main1,
  E_Mode_Python_Main2,
  E_Mode_Python_Main3,
  E_Mode_Python_Main4,
  E_Mode_Python_Main5,
  E_Mode_Python_Main6,
  E_Mode_Python_Main7,
  E_Mode_Max = E_Mode_Python_Main7
};
typedef int16_t T_Mode;  // Mode selection

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

static uint8_t navigation_state = RUNNING_MENU;

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
#endif

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Mode_Init(bool enableVM)
{
  static bool first = false;

  VMIsActive = enableVM;

  StartMode(E_Mode_Menu);
  navigation_state = RUNNING_MENU;

  if (VMIsActive)
  {
    SelectMode = E_Mode_Menu;
  }
  else
  {
    SelectMode = E_Mode_Friendly;
    SetModeColor(SelectMode);
  }

  Behavior_Enable(B_ALWAYS | B_MODE | B_SETTING);

  if (!first)
  {
    first = true;
    Sequence_Init();
  }

  ESP_LOGI(Tag, "Mode is initialized");
}

//_____________________________________________________________________________

void Mode_InitVM(void)
{
  Behavior_Enable(B_LEDS_ACC);
  Behavior_Enable(B_LEDS_LEGO);
  Behavior_Enable(B_LEDS_PROX);
  Behavior_Enable(B_SOUND_BUTTON);
  Behavior_Enable(B_LED_MIC);
  Behavior_Enable(B_LED_RC5);

  ESP_LOGI(Tag, "VM Mode is initialized");
}

//_____________________________________________________________________________

void Mode_Run(void)
{
  uint8_t* buttonState;

  static uint8_t ignore = 0;
  static bool vmIsRunning = false;

  buttonState = Buttons_GetStatus();

  // Handle pre-programmed behavior entering/exiting with the center button
  when(buttonState[E_Button_Center]) {
    if(navigation_state == RUNNING_MENU) {  // Entering a behavior
      // In case the selected mode is one of user python scripts, then we need to first check if they are present.
      // In case they are not present, then emit a sound and remain in the menu.
      ignore = 0;
      if(SelectMode>=E_Mode_Python_Main1 && SelectMode<= E_Mode_Python_Main7) {
        switch(SelectMode) {
          case E_Mode_Python_Main1:
            if(!script_is_present(1)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }
            break;

          case E_Mode_Python_Main2:
            if(!script_is_present(2)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;

          case E_Mode_Python_Main3:
            if(!script_is_present(3)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;

          case E_Mode_Python_Main4:
            if(!script_is_present(4)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;

          case E_Mode_Python_Main5:
            if(!script_is_present(5)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;

          case E_Mode_Python_Main6:
            if(!script_is_present(6)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;

          case E_Mode_Python_Main7:
            if(!script_is_present(7)) {
              Codec_PlayMP3FileFromFlash(E_SoundIndex_Bad);
              ignore = 1;
            }          
            break;
        }
      }

      if(ignore == 0) {
        navigation_state = RUNNING_BEHAVIOR;
        if (SelectMode != CurrentMode) {
          ExitMode(CurrentMode);
          StartMode(SelectMode);        
          CurrentMode = SelectMode;
        }
      }
    } else {  // Entering menu
      // Once you enter one of the user python scripts or the REPL, then you cannot exit normally but you need to power off the robot.
      if(SelectMode<E_Mode_Python_REPL) {
        navigation_state = RUNNING_MENU;
        ExitMode(CurrentMode);
        StartMode(E_Mode_Menu);
        CurrentMode = E_Mode_Menu;
      }
    }
  }


/*

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
      if (!TCPServer_IsSocketAccepted() && vmIsRunning)
      {
        vmIsRunning = false;
        ESP_LOGE(Tag, "BYE Aseba");
      }

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
      //else
      {
        //StartMode(SelectMode);
        //CurrentMode = SelectMode;
        //ESP_LOGE(Tag, "D");
      }

      //Gpio_ClearButtonStatus();
    }
  }

  //if (STM32_IsUSBPortOpen() || TCPServer_IsSocketAccepted())
  if (TCPServer_IsSocketAccepted())
  {
    ExitMode(CurrentMode);
    Behavior_Disable(B_MODE);
    Leds_SetDebugBrightness(0u, MAX_BRIGHTNESS, 0u);
    Mode_InitVM();
    vmIsRunning = true;
    return;
  }
  else if (!TCPServer_IsSocketAccepted() && vmIsRunning)
  {
    ExitMode(CurrentMode);
    return;
  }
*/

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

    case E_Mode_Friendly:     // Green
      Friendly_Run();
      break;

    case E_Mode_Explorer:     // Yellow
      Explorer_Run();
      break;

    case E_Mode_Fearful:      // Red
      Fearful_Run();
      break;

    case E_Mode_Attentive:    // Blue
      break;

    case E_Mode_Investigator:  // Cyan
      LineTracker_Run();
      break;

    case E_Mode_Obedient:   // Magenta
      break;

    case E_Mode_Painter:     // White + front lego leds      
      Painter_Run();
      break;

    case E_Mode_Sequence:     // White + front lego leds
      Sequence_Run();
      break;

    case E_Mode_Musician:     // White + front lego leds
      Musician_Run();
      break;

    case E_Mode_NN:     // White + front lego leds
      break;      

    case E_Mode_Python_REPL:     // Black + back lego leds
      PythonHandler_Run(0);
      break;

    case E_Mode_Python_Main1:     // Black + back lego leds
      PythonHandler_Run(1);
      break;

    case E_Mode_Python_Main2:     // Black + back lego leds
      PythonHandler_Run(2);
      break;

    case E_Mode_Python_Main3:     // Black + back lego leds
      PythonHandler_Run(3);
      break;

    case E_Mode_Python_Main4:     // Black + back lego leds
      PythonHandler_Run(4);
      break;

    case E_Mode_Python_Main5:     // Black + back lego leds
      PythonHandler_Run(5);
      break;

    case E_Mode_Python_Main6:     // Black + back lego leds
      PythonHandler_Run(6);
      break;

    case E_Mode_Python_Main7:     // Black + back lego leds
      PythonHandler_Run(7);
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
      //Behavior_Enable(B_SETTING);
      break;

    case E_Mode_Friendly:
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

    case E_Mode_Attentive:
      break;

    case E_Mode_Investigator:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      break;

    case E_Mode_Painter:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_Sequence:
      Behavior_Enable(B_LEDS_PROX);
      Behavior_Enable(B_LEDS_LEGO);
      Behavior_Enable(B_LED_RC5);
      Sequence_Start();
      break;

    case E_Mode_Musician:
      Behavior_Enable(B_LEDS_PROX);
      break;

    case E_Mode_NN:
      break;        

    case E_Mode_Python_REPL:
      break;

    case E_Mode_Python_Main1:
      break;

    case E_Mode_Python_Main2:
      break;

    case E_Mode_Python_Main3:
      break;

    case E_Mode_Python_Main4:
      break;

    case E_Mode_Python_Main5:
      break;

    case E_Mode_Python_Main6:
      break;

    case E_Mode_Python_Main7:
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
      //Behavior_Disable(B_SETTING);
      break;

    case E_Mode_Friendly:
      Friendly_Stop();
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

    case E_Mode_Attentive:
      break;

    case E_Mode_Investigator:
      LineTracker_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Obedient:
      break;

    case E_Mode_Painter:
      Painter_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_Sequence:
      Sequence_Stop();
      Behavior_Disable(B_LEDS_PROX);
      Behavior_Disable(B_LEDS_LEGO);
      Behavior_Disable(B_LED_RC5);
      break;

    case E_Mode_Musician:
      Musician_Stop();
      Behavior_Disable(B_LEDS_PROX);
      break;

    case E_Mode_NN:
      break;        

    case E_Mode_Python_REPL:
      break;

    case E_Mode_Python_Main1:
      break;

    case E_Mode_Python_Main2:
      break;

    case E_Mode_Python_Main3:
      break;

    case E_Mode_Python_Main4:
      break;

    case E_Mode_Python_Main5:
      break;

    case E_Mode_Python_Main6:
      break;

    case E_Mode_Python_Main7:
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
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Friendly:  // Green
      Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Explorer:  // Yellow
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 12u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Fearful:  // Red
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Attentive:  // Dark blue
      Leds_SetBodyBrightness(0u, 0u, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Investigator:  // Cyan
      Leds_SetBodyBrightness(0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Obedient:  // Magenta
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Painter:  // White + front lego leds
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Sequence: // White + front lego leds
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_Musician:  // White + front lego leds
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;

    case E_Mode_NN:     // White + front lego leds
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoFrontBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      break;      

    case E_Mode_Python_REPL:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main1:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main2:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main3:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main4:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main5:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, 0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main6:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;

    case E_Mode_Python_Main7:     // Black + back lego leds
      Leds_SetBodyBrightness(0u, 0u, 0u);
      Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
      Leds_SetLegoBackBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      break;    

    default:
      // Do nothing
      break;
  }
}

void enter_micropython_mode(void) {
  ExitMode(CurrentMode);
  Behavior_Disable(B_MODE);
  Behavior_Disable(B_SETTING);
}

extern void exit_micropython_mode(void) {
  CurrentMode = E_Mode_Menu;
  Behavior_Enable(B_MODE);
  Behavior_Enable(B_SETTING);
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
#endif
