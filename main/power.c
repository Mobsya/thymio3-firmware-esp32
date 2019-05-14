//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: power.c 18076 2017-04-20 12:28:12Z v.gonet $
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
#include "stm32.h"
#include "timer_hw.h"

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

static T_GpioPinConfig PinConfig = {VA_ENABLE_PIN,
                                    E_GpioMode_Output,
                                    E_GpioResistor_None,
                                    E_GpioLevel_Low,
                                    E_GpioInterrupt_Disable
                                   };

static T_TimerHw* PowerModeTimer = NULL;  //!< Used to switch off the ESP32

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void Callback_TimerPowerMode(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Power_Init(void)
{
  Gpio_ConfigurePin(&PinConfig);

  PowerModeTimer = TimerHw_Create(POWER_MODE_DURATION_us, Callback_TimerPowerMode);

  Power_EnableVA();
}

//_____________________________________________________________________________

void Power_EnableVA(void)
{
  Gpio_SetPinLevel(VA_ENABLE_PIN, E_GpioLevel_High);
}

//_____________________________________________________________________________

void Power_DisableVA(void)
{
  Gpio_SetPinLevel(VA_ENABLE_PIN, E_GpioLevel_Low);
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
          //PinConfig.mode = E_GpioMode_Input;
          //Gpio_ConfigurePin(&PinConfig);

          ESP_LOGW(Tag, "Sleep mode requested");

          STM32_AllowToSwitchOff();

          Power_DisableVA();  // Switch off the IR sensors, the color sensor, the accelerometer/gyroscope, the microphone and some LEDs

          //ESP_LOGW(Tag, "VA disabled");

          //vTaskDelay(200 / portTICK_PERIOD_MS);

          //ESP_LOGW(Tag, "Start timer");
          //TimerHw_StartTimerOnce(PowerModeTimer, POWER_MODE_DURATION_us);
          first = false;
        }
      }
    }
  }
}

//_____________________________________________________________________________

static void Callback_TimerPowerMode(void* arg)
{
  //Power_DisableVA();  // Switch off the IR sensors, the color sensor, the accelerometer/gyroscope, the microphone and some LEDs
}
