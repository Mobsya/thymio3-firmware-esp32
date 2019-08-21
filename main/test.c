//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    test.c
//! \brief   This module provides the useful functions to execute the tests
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "test.h"

#include "color_sensor.h"
#include "accelerometer.h"
#include "error.h"
#include "stm32_i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TESTS_NUM    3u  //!< Number of tests available

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details The configuration of the self-tests
typedef struct
{
  T_Error(*testFunction)(void);  //!< Tests available
  //bool ContTestAllowed;              //!< true = continuous test allowed, false = continuous test not allowed
} T_TestUse;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "test";

static const T_TestUse TestTable[TESTS_NUM] =
{
  {STM32_CheckId},
  {ColorSensor_CheckManufacturerId},
  {Accelerometer_CheckManufacturerId}
  //{ColorSensor_CheckManufacturerId,   false}
};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunDebuggingTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Test_Run(void)
{
  T_Error err = E_Error_Nok;

  for (uint8_t index = 0u; index < TESTS_NUM; index++)
  {
    err = (*TestTable[index].testFunction)();

    if (err != E_Error_None)
    {
      break;
    }
  }

  if (err != E_Error_None)
  {
    //ProtectiveAbort();
  }
}

//_____________________________________________________________________________

void Test_StartDebugging(void)
{
  xTaskCreatePinnedToCore(
    RunDebuggingTask,  // Function to implement the task
    "debug",           // Name of the task
    4096,              // Stack size in words
    NULL,              // Task input parameter
    4,                 // Priority of the task
    NULL,              // Task handle
    0);                // Core where the task should run
}

//_____________________________________________________________________________

static void RunDebuggingTask(void* arg)
{
  ESP_LOGI(Tag, "Start Debug Task");

  char pmem[2048];

  while (1)
  {
    vTaskGetRunTimeStats(pmem);
    printf(pmem);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
