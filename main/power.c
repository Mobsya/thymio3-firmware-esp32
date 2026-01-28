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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "power.h"
#include "behavior.h"
#include "rc5.h"
#include "sensors.h"
#include "leds.h"
#include "codec.h"
#include "gpio.h"
#include "pins_def.h"
#include "stm32_spi.h"
#include "common.h"

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
  static uint8_t powerDownState = 0;
  //printf("powerDownState=%d\n", powerDownState);
  switch(powerDownState) {
    case 0: // Waiting for power down request. Then stop main tasks and play "bye bye".
      if(STM32_IsStandbyRequested()) {
        //printf("standby requested\n");
        powerDownState = 1;
        Behavior_Stop();
        RC5_Stop();
        Sensors_Stop();
        Leds_Stop();
        Common_SetTargetSpeed(0, 0);
        Codec_Stop();
        if(Codec_PlayOnboardSound(TONE_TYPE_OUTRO) != ESP_OK) {
          //printf("Cannot play byebye!\n");
        }
      }
      break;
    
    case 1: // Wait for "bye bye" sound terminates. Then tell STM32 to turn off.
      //printf("wait byebye\n");
      if(Codec_IsSoundFinished()) {
        //printf("byebye finished\n");
        powerDownState = 2;
        STM32_AllowToSwitchOff();  // Give the permission to the STM32 to switch off
      }
      break;
    
    case 2:
      break;
  }

}
