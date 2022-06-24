//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    gpio.c
//! \brief   This module provides the useful functions to use the GPIO
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <driver/gpio.h>
#include "driver/timer.h"

#include "esp_log.h"

#include "aseba_esp32.h"
#include "gpio.h"

#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ESP_INTR_FLAG_DEFAULT     0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  gpio_pullup_t   up;
  gpio_pulldown_t down;
} T_PullUpDown;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "gpio";

static bool FreeFall = false;
static bool Tap = false;
static bool Side = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static gpio_mode_t SetMode(T_GpioMode mode);

static T_PullUpDown SetResistor(T_GpioResistor resistor);

static gpio_int_type_t SetInterrupt(T_GpioInterrupt interrupt);

static void IRAM_ATTR ISR_GPIOHandler(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Gpio_Init(void)
{
  // Install the GPIO ISR service
  //gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT); // Already installed when initializing SPIFFS
}

//_____________________________________________________________________________

void Gpio_ConfigurePin(const T_GpioPinConfig* config)
{
  T_PullUpDown pullUpDown;
  gpio_config_t pinConfig;

  if (config->pinNumber < MAX_NUMBER_PIN)
  {
    pullUpDown = SetResistor(config->resistor);

    pinConfig.intr_type    = SetInterrupt(config->interrupt);
    pinConfig.mode         = SetMode(config->mode);
    pinConfig.pin_bit_mask = (((uint64_t) 1) << config->pinNumber);
    pinConfig.pull_down_en = pullUpDown.down;
    pinConfig.pull_up_en   = pullUpDown.up;

    if (gpio_config(&pinConfig) == ESP_OK)
    {
      if (config->interrupt > E_GpioInterrupt_Disable)
      {
        //ESP_ERROR_CHECK(gpio_isr_handler_add(config->pinNumber, gpio_isr_handler, (void*) config->pinNumber));
        gpio_isr_handler_add(config->pinNumber, ISR_GPIOHandler, (void*)config->pinNumber);
      }

      if (config->mode == E_GpioMode_Output)
      {
        //ESP_LOGI(Tag, "Pin %d is configured", config->pinNumber);
        ESP_ERROR_CHECK(gpio_set_level(config->pinNumber, config->level));
      }
    }
    else
    {
      ESP_LOGE(Tag, "Pin %d is not configured", config->pinNumber);
    }
  }
  else
  {
    ESP_LOGE(Tag, "Invalid pin %d", config->pinNumber);
  }
}

//_____________________________________________________________________________

void Gpio_SetPinLevel(uint16_t pinNumber, T_GpioLevel level)
{
  ESP_ERROR_CHECK(gpio_set_level(pinNumber, level));
}

//_____________________________________________________________________________

void Gpio_TogglePinLevel(uint16_t pinNumber)
{
  static uint8_t counter = 0;

  ESP_ERROR_CHECK(gpio_set_level(pinNumber, counter % 2));
  counter++;
}

//_____________________________________________________________________________

T_GpioLevel Gpio_GetPinLevel(uint16_t pinNumber)
{
  T_GpioLevel level = E_GpioLevel_Low;

  if (gpio_get_level(pinNumber) == 1)
  {
    level = E_GpioLevel_High;
  }

  return level;
}

//_____________________________________________________________________________

bool Gpio_IsFreeFallDetected(void)
{
  return FreeFall;
}

//_____________________________________________________________________________

void Gpio_ClearFreeFallStatus(void)
{
  FreeFall = false;
}

//_____________________________________________________________________________

bool Gpio_IsTapDetected(void)
{
  return Tap;
}

//_____________________________________________________________________________

void Gpio_ClearTapStatus(void)
{
  Tap = false;
}

//_____________________________________________________________________________

bool Gpio_IsButtonPressed(void)
{
  return Side;
}

//_____________________________________________________________________________

void Gpio_ClearButtonStatus(void)
{
  Side = false;
}

//_____________________________________________________________________________

static gpio_mode_t SetMode(T_GpioMode mode)
{
  gpio_mode_t pinMode = GPIO_MODE_INPUT;

  switch (mode)
  {
    case E_GpioMode_Input:
      pinMode = GPIO_MODE_INPUT;
      break;

    case E_GpioMode_Output:
      pinMode = GPIO_MODE_OUTPUT;
      break;

    case E_GpioMode_OutputOpenDrain:
      pinMode = GPIO_MODE_OUTPUT_OD;
      break;

    case E_GpioMode_InputOutput:
      pinMode = GPIO_MODE_INPUT_OUTPUT;
      break;

    case E_GpioMode_InputOutputOpenDrain:
      pinMode = GPIO_MODE_INPUT_OUTPUT_OD;
      break;

    default:
      ESP_LOGE(Tag, "Invalid mode %d", mode);
      break;
  }

  return pinMode;
}

//_____________________________________________________________________________

static T_PullUpDown SetResistor(T_GpioResistor resistor)
{
  T_PullUpDown pullUpDown;

  switch (resistor)
  {
    case E_GpioResistor_None:
      pullUpDown.up   = GPIO_PULLUP_DISABLE;
      pullUpDown.down = GPIO_PULLDOWN_DISABLE;
      break;

    case E_GpioResistor_PullUp:
      pullUpDown.up   = GPIO_PULLUP_ENABLE;
      pullUpDown.down = GPIO_PULLDOWN_DISABLE;
      break;

    case E_GpioResistor_PullDown:
      pullUpDown.up   = GPIO_PULLUP_DISABLE;
      pullUpDown.down = GPIO_PULLDOWN_ENABLE;
      break;

    default:
      ESP_LOGE(Tag, "Invalid resistor %d", resistor);
      break;
  }

  return pullUpDown;
}

//_____________________________________________________________________________

static gpio_int_type_t SetInterrupt(T_GpioInterrupt interrupt)
{
  gpio_int_type_t pinInterrupt = GPIO_INTR_DISABLE;

  switch (interrupt)
  {
    case E_GpioInterrupt_Disable:
      pinInterrupt = GPIO_INTR_DISABLE;
      break;

    case E_GpioInterrupt_RisingEdge:
      pinInterrupt = GPIO_INTR_POSEDGE;
      break;

    case E_GpioInterrupt_FallingEdge:
      pinInterrupt = GPIO_INTR_NEGEDGE;
      break;

    case E_GpioInterrupt_AnyEdge:
      pinInterrupt = GPIO_INTR_ANYEDGE;
      break;

    case E_GpioInterrupt_LowLevel:
      pinInterrupt = GPIO_INTR_LOW_LEVEL;
      break;

    case E_GpioInterrupt_HighLevel:
      pinInterrupt = GPIO_INTR_HIGH_LEVEL;
      break;

    default:
      ESP_LOGE(Tag, "Invalid interrupt %d", interrupt);
      break;
  }

  return pinInterrupt;
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_GPIOHandler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;

  if (gpio_num == ACC_INT1_PIN)
  {
    FreeFall = true;
    SET_EVENT(EVENT_FREEFALL);
  }

  if (gpio_num == ACC_INT2_PIN)
  {
    Tap = true;
    SET_EVENT(EVENT_TAP);
  }

  if (gpio_num == BUTTON_SIDE_PIN)
  {
    Side = true;
  }
}
