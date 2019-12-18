//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.c
//! \brief   This module provides the useful functions to use the capacitive buttons
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
//#include "soc/sens_periph.h"

#include "esp_log.h"

#include "buttons.h"

#include "aseba_esp32.h"
#include "behavior.h"
#include "board.h"
#include "gpio.h"

#include "common.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TOUCH_THRESH_NO_USE   (0)
#define PRESSED_THRESHOLD_PERCENT  (97)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "buttons";

static touch_pad_t Buttons_Table[BUTTONS_NUM];

static uint8_t ButtonStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
static uint16_t ButtonRaw[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
static uint32_t Threshold[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

//static uint8_t ButtonBehaviorStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

static const T_GpioPinConfig PinConfig = {BUTTON_SIDE_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_FallingEdge};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void InitTouchPad();

static void SetThresholds(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Buttons_Init(void)
{
  // Configure the GPIO of the side button
  Gpio_ConfigurePin(&PinConfig);

  // Initialize touch pad peripheral, it will start a timer to run a filter
  touch_pad_init();

  // Set reference voltage for charging/discharging
  // For most usage scenarios, we recommend using the following combination:
  // the high reference voltage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

  //touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);

  // Init touch pad IO
  InitTouchPad();

  // Initialize and start a software filter to detect slight change of capacitance.
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

  // Set threshold
  SetThresholds();

  ESP_LOGI(Tag, "Buttons are initialized");
}

//_____________________________________________________________________________

uint8_t* Buttons_GetStatus(void)
{
  return ButtonStatus;
}

//_____________________________________________________________________________
#if 0
uint8_t* Buttons_GetBehaviorStatus(void)
{
  return ButtonBehaviorStatus;
}

//_____________________________________________________________________________

void Buttons_ClearBehaviorStatus(void)
{
  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    ButtonBehaviorStatus[button] = 0u;
  }
}
#endif
//_____________________________________________________________________________

void Buttons_UpdateStatus(void)
{
  //SetThresholds();

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    //touch_pad_read_raw_data(Buttons_Table[button], &ButtonRaw[button]);
    touch_pad_read_filtered(Buttons_Table[button], &ButtonRaw[button]);

    //ESP_LOGE(Tag, "Button[%d]: %d", button, ButtonRaw[button]);

    if (ButtonRaw[button] < ((Threshold[button] * PRESSED_THRESHOLD_PERCENT) / 100))
    {
      //if (button == 2)
      {
        //ESP_LOGE(Tag, "Pressed %d, %d", ButtonRaw[button], ButtonStatus[button]);
      }

      when(ButtonStatus[button] != 0u)
      {
        //ESP_LOGE(Tag, "Buttons %d pressed", button);
        //ButtonBehaviorStatus[button] = 1u;
        SET_EVENT(button);
        Behavior_PlaySoundButtons(button);
      }

      ButtonStatus[button] = 1u;
    }
    else
    {
      ButtonStatus[button] = 0u;

      //if (button == 2)
      {
        //ESP_LOGE(Tag, "Released %d, %d", ButtonRaw[button], ButtonStatus[button]);
      }
    }

    vmVariables.buttons_state[button] = (int16_t)ButtonStatus[button];
    vmVariables.buttons[button] = (int16_t)ButtonRaw[button];
  }
}

//_____________________________________________________________________________

static void InitTouchPad()
{
  Buttons_Table[E_Button_Backward] = TOUCH_PAD_NUM2;
  Buttons_Table[E_Button_Left]     = TOUCH_PAD_NUM5;
  Buttons_Table[E_Button_Center]   = TOUCH_PAD_NUM0;
  Buttons_Table[E_Button_Forward]  = TOUCH_PAD_NUM7;
  Buttons_Table[E_Button_Right]    = TOUCH_PAD_NUM8;

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    // Initialize RTC IO and mode for touch pad
    touch_pad_config(Buttons_Table[button], TOUCH_THRESH_NO_USE);
  }
}

//_____________________________________________________________________________

static void SetThresholds(void)
{
  uint16_t value;

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    // Read filtered value
    touch_pad_read_filtered(Buttons_Table[button], &value);

    Threshold[button] = value;

    ESP_LOGE(Tag, "test init: touch pad [%d] val is %d", button, value);
  }
}
