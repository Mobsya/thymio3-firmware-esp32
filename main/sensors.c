//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sensors.c
//! \brief   This module provides the useful functions to acquire sensors values
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

#include "sensors.h"

#include "accelerometer.h"
#include "board.h"
#include "buttons.h"
#include "codec.h"
#include "color_sensor.h"
#include "gyroscope.h"
#include "i2c.h"

#include "es8374.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

xSemaphoreHandle I2CMutex;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sensors";

static TaskHandle_t SensorsTask = NULL;

static bool TaskIsStarted = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the sensors task
//! \pre       First initialize the sensors
//! \param     arg - Task parameter
//! \return    None
static void RunSensorsTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sensors_Init(void)
{
  TaskIsStarted = false;

  // FIXME If the I2C is only used by the sensors, the initialization can be done here.
  // Else, it must be done in the main.c
  //I2C_Init();

  I2CMutex = xSemaphoreCreateMutex();

  //Codec_Init();

  ColorSensor_Init();
  Accelerometer_Init();
  Gyroscope_Init();
}

//_____________________________________________________________________________

void Sensors_Start(void)
{
  xTaskCreatePinnedToCore(
    RunSensorsTask,  // Function to implement the task
    "sensors",       // Name of the task
    2048,            // Stack size in words
    NULL,            // Task input parameter
    3,               // Priority of the task
    &SensorsTask,    // Task handle
    0);              // Core where the task should run

  TaskIsStarted = true;
}

//_____________________________________________________________________________

void Sensors_Stop(void)
{
  if (TaskIsStarted)
  {
    ESP_LOGW(Tag, "Sensors task is stopped");

    I2C_DeleteDriver();
    TaskIsStarted = false;
    vTaskDelete(SensorsTask);
  }
}

//_____________________________________________________________________________

static void RunSensorsTask(void* arg)
{
  ESP_LOGI(Tag, "Start Sensors Task");

  while (1)
  {
    Buttons_UpdateStatus();

    xSemaphoreTake(I2CMutex, portMAX_DELAY);

    // Every 20 [ms], 50 [Hz] (vTaskDelay = 20 [ms])
    Accelerometer_ReadTapSource();
    Accelerometer_GetAcceleration();
    Gyroscope_GetAngularVelocity();
    Gyroscope_ReadAngle();
    ColorSensor_ReadColor();

    xSemaphoreGive(I2CMutex);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
