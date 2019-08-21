//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    power.c
//! \brief   This module provides the useful functions to manage the power
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
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "power.h"

#include "board.h"
#include "gpio.h"
#include "stm32_i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define POWER_MODE_DURATION_us   10000u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "power";

static T_GpioPinConfig PinConfig = {GPIO0_PIN,
                                    E_GpioMode_Input,
                                    E_GpioResistor_None,
                                    E_GpioLevel_Low,
                                    E_GpioInterrupt_Disable
                                   };

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Power_Init(void)
{
  Gpio_ConfigurePin(&PinConfig);

  ESP_LOGI(Tag, "Power is initialized");
}

//_____________________________________________________________________________

void Power_HandlePowerModeRequest(void)
{
  static bool first = true;

  if (STM32_IsModeUpdateRequested())
  {
    if (STM32_IsReadyToSwitchOff())
    {
      if (!STM32_IsAllowedToSwitchOff())
      {
        if (first)
        {
          STM32_AllowToSwitchOff();  // Give the permission to the STM32 to switch off
          first = false;
        }
      }
    }
  }
}
