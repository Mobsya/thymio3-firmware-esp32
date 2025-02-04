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
#include "aseba_esp32.h"
#include "buttons.h"
#include "codec.h"
#include "color_sensor.h"
#include "gyroscope.h"
#include "i2c.h"
#include "settings.h"

#include "es8374.h"
#include "pins_def.h"

#include <sys/time.h>

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
static TaskHandle_t ButtonsTask = NULL;

static bool TaskIsStarted = false;
static bool ButtonsInhibit = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the sensors task
//! \pre       First initialize the sensors
//! \param     arg - Task parameter
//! \return    None
static void RunSensorsTask(void* arg);
static void RunButtonsTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sensors_Init(void)
{
  TaskIsStarted = false;

  I2C_Init();

  I2CMutex = xSemaphoreCreateMutex();

  Codec_Init();

  Buttons_Init();

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
    6,               // Priority of the task
    &SensorsTask,    // Task handle
    0);              // Core where the task should run

  TaskIsStarted = true;

  xTaskCreatePinnedToCore(
	RunButtonsTask,  // Function to implement the task
    "buttons",       // Name of the task
    2048,            // Stack size in words
    NULL,            // Task input parameter
    6,               // Priority of the task
    &ButtonsTask,    // Task handle
    0);              // Core where the task should run
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
    vTaskDelete(ButtonsTask);
  }
}

//_____________________________________________________________________________

static void RunSensorsTask(void* arg)
{
  int64_t time_start, time_end;

  ESP_LOGI(Tag, "Start Sensors Task");

  if (ColorSensor_CheckManufacturerId() != E_Error_None)
  {
    ESP_LOGE(Tag, "Color sensor error");
  }

  if (Accelerometer_CheckManufacturerId() != E_Error_None)
  {
    ESP_LOGE(Tag, "Accelerometer/Gyroscope error");
  }

  while (1)
  {
    time_start = esp_timer_get_time();

    xSemaphoreTake(I2CMutex, portMAX_DELAY);

    // Time needed to read the sensors takes about 33-39 ms
    Accelerometer_ReadTapSource();
    Accelerometer_ReadAcceleration();
    Gyroscope_ReadAngularVelocity();
    Gyroscope_ReadAngle();
    ColorSensor_ReadColor();

    SET_EVENT(EVENT_SENSORS);

    xSemaphoreGive(I2CMutex);

		time_end = esp_timer_get_time();
		//printf("%lld usec\n", time_end - time_start);
		if((time_end - time_start) < 62500) { // Run task @ 16 Hz like in T2 (acc rate)
			vTaskDelay((62500 - (time_end - time_start))/1000 / portTICK_PERIOD_MS);
		}
  }
}

//_____________________________________________________________________________

static void RunButtonsTask(void* arg) {
	int64_t time_start, time_end;
	while (1) {
		time_start = esp_timer_get_time();
		if(!ButtonsInhibit) {
			Buttons_UpdateStatus();
		}
		time_end = esp_timer_get_time();
		//printf("%lld usec\n", time_end - time_start);
		if((time_end - time_start) < 50000) { // Run task @ 20 Hz like in T2
			vTaskDelay((50000 - (time_end - time_start))/1000 / portTICK_PERIOD_MS);
		}
	}
}

extern void Sensors_buttons_pause(void) {
	ButtonsInhibit = true;
}

extern void Sensors_buttons_resume(void) {
	ButtonsInhibit = false;
}


