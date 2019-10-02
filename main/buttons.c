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

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define BUTTONS_NUM            5u

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
//static uint32_t s_pad_init_val[BUTTONS_NUM];

static uint8_t ButtonBehaviorStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void InitTouchPad();

static void Callback_DetectionTouchPad(void* arg);

static void SetThresholds(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Buttons_Init(void)
{
  // Initialize touch pad peripheral, it will start a timer to run a filter
  touch_pad_init();

  // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);

  // Set reference voltage for charging/discharging
  // For most usage scenarios, we recommend using the following combination:
  // the high reference voltage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

  // Init touch pad IO
  InitTouchPad();

  // Initialize and start a software filter to detect slight change of capacitance.
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

  // Set thresh hold
  SetThresholds();

  // Register touch interrupt ISR
  touch_pad_isr_register(Callback_DetectionTouchPad, NULL);

  // Interrupt mode, enable touch interrupt
  touch_pad_intr_enable();

  ESP_LOGI(Tag, "Buttons are initialized");
}

//_____________________________________________________________________________

uint8_t* Buttons_GetStatus(void)
{
  return ButtonBehaviorStatus;
}

//_____________________________________________________________________________

void Buttons_ClearStatus(void)
{
  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    ButtonBehaviorStatus[button] = 0;
  }
}

//_____________________________________________________________________________

void Buttons_UpdateStatus(void)
{
  //static uint8_t oldButtonStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    //touch_pad_read_raw_data(Buttons_Table[button], &ButtonRaw[button]);
    touch_pad_read_filtered(Buttons_Table[button], &ButtonRaw[button]);

    //if (button == 1)
    {
      //ESP_LOGI(Tag, "COUCOU Raw%d %d", button, ButtonRaw[button]);
    }

    if (ButtonStatus[button] != 0u)
    {
      if (button == 0)  // Backward
      {
        ESP_LOGI(Tag, "SALUT Raw%d %d", button, ButtonRaw[button]);
      }
      //ESP_LOGI(Tag, "COUCOU Raw%d %d", button, ButtonRaw[button]);
      //ESP_LOGI(Tag, "COUCOU T%d activated!", Buttons_Table[index]);

      // Wait a while for the pad being released
//      vTaskDelay(200 / portTICK_PERIOD_MS);
      ButtonBehaviorStatus[button] = 1;
      ButtonStatus[button] = 0u;
      //oldButtonStatus[button] = 1u;
    }
    //else if (oldButtonStatus[button] == 0)
    {
      //ButtonBehaviorStatus[button] = 0;
      //oldButtonStatus[button] = 0u;
    }
    //else
    {
      //oldButtonStatus[button] = 0u;
    }

    vmVariables.buttons_state[button] = (int16_t)ButtonBehaviorStatus[button];
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

static void Callback_DetectionTouchPad(void* arg)
{
  uint32_t pad_intr = touch_pad_get_status();

  //clear interrupt
  touch_pad_clear_status();

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    if ((pad_intr >> Buttons_Table[button]) & 0x01)
    {
      ButtonStatus[button] = 1u;
      SET_EVENT(button);
    }
  }

  SET_EVENT(EVENT_BUTTONS);
}

//_____________________________________________________________________________

static void SetThresholds(void)
{
  uint16_t value;

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    //read filtered value
    touch_pad_read_filtered(Buttons_Table[button], &value);
//    s_pad_init_val[button] = value;
//    ESP_LOGI(Tag, "test init: touch pad [%d] val is %d", button, touch_value);
    //set interrupt threshold.
    ESP_ERROR_CHECK(touch_pad_set_thresh(Buttons_Table[button], value * 12 / 13));  // for proto 1 16 / 17                // 19 / 20
  }
}
