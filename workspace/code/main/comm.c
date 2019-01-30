//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    comm.c
//! \brief   This module provides the useful functions to communicate
//!
//! \author  Vincent Gonet
//!
//! \version $Id: comm.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "comm.h"

#include "accelerometer.h"
#include "color_sensor.h"
#include "gyroscope.h"
#include "i2c.h"
#include "stm32.h"

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

static const char* Tag = "sensors";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Comm_Init(void)
{
  I2C_Init();

  ColorSensor_Init();
  Accelerometer_Init();
  Gyroscope_Init();
}

//_____________________________________________________________________________

void Comm_RunTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Comm Task");

  while (1)
  {
    STM32_ReadStatus();
	STM32_ReadMotorCurrent();
    STM32_ReadBatteryVoltage();
	STM32_ReadInducedVoltage();
	STM32_ReadPwmDutyCycle();
	STM32_ReadButtonStatus();
	STM32_ReadButtonRawData();

	ColorSensor_GetColor();
	Accelerometer_GetTapSource();
    Accelerometer_GetAcceleration();
	Gyroscope_GetAngularPosition();

#if 0
    if (Power_IsSwitchOffEnabled())
	{
      ESP_LOGI(Tag, "COUCOU");
	  //Power_SwitchOff();
	  Leds_SetTopBrightness(MAX_BRIGHTNESS, 0, 0);
	}
	else if (STM32_IsReadyToSwitchOff())
	{
	  Power_EnableSwitchOff();
	  Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
	  STM32_AllowToSwitchOff();
	   //Power_SwitchOff();
	}
#endif

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
