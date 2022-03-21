//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.c
//! \brief   This module provides the useful functions to use the buttons
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "driver/touch_pad.h"

#include "esp_log.h"

#include "buttons.h"

#include "aseba_esp32.h"
#include "board.h"
#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TOUCH_THRESH_NO_USE              0u
#define PRESSED_THRESHOLD_PERCENT       98u
#define TOUCHPAD_FILTER_TOUCH_PERIOD    10u
#define THRESHOLD_AVERAGE_SIZE          16u
#define DEBOUNCE                         3

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

static uint8_t ButtonStatus[BUTTONS_NUM]    = {0u, 0u, 0u, 0u, 0u};
static uint16_t ButtonFiltered[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};
static uint16_t ButtonRaw[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
static uint16_t Threshold[BUTTONS_NUM]      = {0u, 0u, 0u, 0u, 0u};
static uint16_t Sum[BUTTONS_NUM]            = {0u, 0u, 0u, 0u, 0u};
static int16_t Count[BUTTONS_NUM]           = {-DEBOUNCE, -DEBOUNCE, -DEBOUNCE, -DEBOUNCE, -DEBOUNCE};

static const T_GpioPinConfig PinConfig = {BUTTON_SIDE_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_FallingEdge};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the touch pad
//! \pre       First initialize the buttons
//! \param     None
//! \return    None
static void InitTouchPad();

//! \brief     Initialize the detection threshold
//! \pre       First initialize the buttons
//! \param     None
//! \return    None
static void InitThresholds(void);

//! \brief     Update the detection threshold
//! \pre       First initialize the buttons
//! \param     button - Button on which the threshold is updated
//! \return    None
static void UpdateThresholds(uint8_t button);

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
  //touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
  touch_pad_set_voltage(TOUCH_HVOLT_2V6, TOUCH_LVOLT_0V6, TOUCH_HVOLT_ATTEN_0V5);

  // Init touch pad IO
  InitTouchPad();

  // Initialize and start a software filter to detect slight change of capacitance.
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

  // Set initial threshold
  InitThresholds();

  ESP_LOGI(Tag, "Buttons are initialized");
}

//_____________________________________________________________________________

uint8_t* Buttons_GetStatus(void)
{
  return ButtonStatus;
}

//_____________________________________________________________________________

void Buttons_UpdateStatus(void)
{
  static uint8_t oldButtonStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    touch_pad_read_filtered(Buttons_Table[button], &ButtonFiltered[button]);
    touch_pad_read_raw_data(Buttons_Table[button], &ButtonRaw[button]);

    // The button is pressed
    if (ButtonFiltered[button] < Threshold[button])
    {
      ButtonStatus[button] = 1u;

      // Only if the button was previously not pressed
      if (ButtonStatus[button] != oldButtonStatus[button])
      {
        SET_EVENT(button);
      }

      Sum[button] = 0u;
      Count[button] = -DEBOUNCE;
    }
    else
    {
      ButtonStatus[button] = 0u;

      // The threshold is updated only when the button is not pressed
      UpdateThresholds(button);
    }

    oldButtonStatus[button] = ButtonStatus[button];

    vmVariables.buttons_state[button]     = (int16_t)ButtonStatus[button];
    vmVariables.buttons[button]           = (int16_t)ButtonRaw[button];
    vmVariables.buttons_mean[button]      = (int16_t)ButtonFiltered[button];
    vmVariables.buttons_threshold[button] = (int16_t)Threshold[button];
  }
}

//_____________________________________________________________________________

static void InitTouchPad()
{
  Buttons_Table[E_Button_Backward] = TOUCH_PAD_NUM2;  // GPIO2
  Buttons_Table[E_Button_Left]     = TOUCH_PAD_NUM5;  // GPIO12
  Buttons_Table[E_Button_Center]   = TOUCH_PAD_NUM7;  // GPIO27
  Buttons_Table[E_Button_Forward]  = TOUCH_PAD_NUM9;  // GPIO32
  Buttons_Table[E_Button_Right]    = TOUCH_PAD_NUM8;  // GPIO33

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    // Initialize RTC IO and mode for touch pad
    touch_pad_config(Buttons_Table[button], TOUCH_THRESH_NO_USE);
  }
}

//_____________________________________________________________________________

static void InitThresholds(void)
{
  uint16_t value;

  for (uint8_t button = 0u; button < BUTTONS_NUM; button++)
  {
    // Read filtered value
    touch_pad_read_filtered(Buttons_Table[button], &value);
    Threshold[button] = (value * PRESSED_THRESHOLD_PERCENT / 100u);
  }
}

//_____________________________________________________________________________

static void UpdateThresholds(uint8_t button)
{
  Count[button]++;

  // The first 3 samples are skipped before calculating the new threshold
  if (Count[button] > 0)
  {
    Sum[button] += ButtonFiltered[button];

    if (Count[button] == THRESHOLD_AVERAGE_SIZE)
    {
      Threshold[button] = ((Sum[button] / THRESHOLD_AVERAGE_SIZE) * PRESSED_THRESHOLD_PERCENT / 100u);

      Sum[button] = 0u;
      Count[button] = 0u;
    }
  }
}
