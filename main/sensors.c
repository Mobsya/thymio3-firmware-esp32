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

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sensors";

static bool BusIsAvailable = false;

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
  // FIXME If the I2C is only used by the sensors, the initialization can be done here.
  // Else, it must be done in the main.c
  //I2C_Init();

  BusIsAvailable = false;

  Codec_Init();

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
    NULL,            // Task handle
    0);              // Core where the task should run
}

//_____________________________________________________________________________

bool Sensors_IsBusAvailable(void)
{
  return BusIsAvailable;
}

//_____________________________________________________________________________

static void RunSensorsTask(void* arg)
{
  ESP_LOGI(Tag, "Start Sensors Task");

  while (1)
  {
	BusIsAvailable = false;

    // Every 20 [ms], 50 [Hz] (vTaskDelay = 20 [ms])
    Accelerometer_ReadTapSource();
    Accelerometer_GetAcceleration();
    Gyroscope_GetAngularPosition();
    ColorSensor_ReadColor();

//    Buttons_UpdateStatus();

    BusIsAvailable = true;

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
