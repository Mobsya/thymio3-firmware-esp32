//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sound.c
//! \brief   This module provides the useful functions to generate the sound
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sound.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <math.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/dac.h"

#include <rom/ets_sys.h>

#include "sound.h"

#include "board.h"
#include "gpio.h"

#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SINE_TABLE_SIZE    256

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sound";

static uint8_t SineTable[SINE_TABLE_SIZE];
static float SoundTable[SINE_TABLE_SIZE];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sound_Init(void)
{
#if 0
  float conversionFactor = (2.0f * M_PI) / SINE_TABLE_SIZE;
  float angle_rad = 0.0f;

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    angle_rad = (pos * conversionFactor);
    SineTable[pos] = ((sin(angle_rad) * 127) + 128);
  }
#endif

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    SoundTable[pos] = 0;
  }

  //Sound_Add(1, 50);
  //Sound_Add(5, 50);

  dac_output_enable(DAC_CHANNEL_1);
}

//_____________________________________________________________________________

void Sound_Task(void)
{
  Sound_Generate();
}

//_____________________________________________________________________________

void Sound_Generate(void)
{
  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    dac_output_voltage(DAC_CHANNEL_1, (uint8_t)SoundTable[pos]);
    ets_delay_us(5);
    //ESP_LOGI(Tag, "table[%d] = %d", pos, SineTable[pos]);
    //vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void Sound_Add(float frequency, float amplitude)
{
  float angle_rad = 0.0f;
  float conversionFactor = (2 * M_PI) / SINE_TABLE_SIZE;

  static uint8_t count = 0u;

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    angle_rad = ((float)pos * conversionFactor);
    SoundTable[pos] += ((sin(angle_rad * frequency) * amplitude) + (amplitude + 1.0f));
#if 0
    if (count == 1)
    {
      ESP_LOGI(Tag, "table[%d] = %f", pos, SoundTable[pos]);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
#endif
  }

  count++;
}

//_____________________________________________________________________________

void Sound_Enable(void)
{
  // TODO GPIO0 shall be set in output on ESP32 and input on STM32 and next we can drive this pin
  //Gpio_SetPinLevel(GPIO0_PIN, E_GpioLevel_High);
}
