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
#include "power.h"
#include "stm32.h"
#include "uart.h"

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

static const char* Tag = "comm";

static bool BusIsAvailable = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the communication task
//! \pre       First initialize the communication
//! \param     arg - Task parameter
//! \return    None
static void RunCommTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Comm_Init(void)
{
  I2C_Init();
  UART_Init();

  ColorSensor_Init();
  Accelerometer_Init();
  Gyroscope_Init();

  BusIsAvailable = false;
}

//_____________________________________________________________________________

void Comm_Start(void)
{
  xTaskCreatePinnedToCore(
    RunCommTask,  // Function to implement the task
    "comm",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    3,            // Priority of the task
    NULL,         // Task handle
    0);           // Core where the task should run
}

//_____________________________________________________________________________

bool Comm_IsBusAvailable(void)
{
  return BusIsAvailable;
}

//_____________________________________________________________________________

static void RunCommTask(void* arg)
{
  static uint8_t counter = 0;

  ESP_LOGI(Tag, "Start Comm Task");

  while (1)
  {
    BusIsAvailable = false;

    Accelerometer_ReadTapSource();
    Accelerometer_GetAcceleration();
    Gyroscope_GetAngularPosition();

    STM32_ReadInducedVoltage();
    STM32_ReadButtonStatus();

    if ((counter % 2u) == 0u)  // Every 40 [ms] (vTaskDelay = 20 [ms])
    {
      ColorSensor_ReadColor();
    }

    if ((counter % 3u) == 0u)  // Every 60 [ms] (vTaskDelay = 20 [ms])
    {
      STM32_ReadStatus();
      STM32_ReadMotorCurrent();
      STM32_ReadBatteryVoltage();
      STM32_ReadPwmDutyCycle();
      STM32_ReadButtonRawData();
      STM32_ReadButtonMean();
      STM32_ReadButtonNoise();

      Power_HandlePowerModeRequest();
    }

    counter++;

    BusIsAvailable = true;

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
