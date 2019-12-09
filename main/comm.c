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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "comm.h"

#include "aseba_esp32.h"
#include "behavior.h"
#include "board.h"
#include "power.h"
#include "spi.h"
#include "stm32_spi.h"
#include "uart.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DATA_SIZE              32u

#define PROX_IR_POSITION       15u
#define GROUND_IR_POSITION     22u

#define STM32_ID               0x4321

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

static const char* Tag = "comm";

static TaskHandle_t CommTask = NULL;

static bool TaskIsStarted = false;

static spi_device_handle_t Microcontroller;

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
  UART_Init();

  STM32_Init();

  Spi_InitVSPI();
  Spi_AddDeviceVSPI(&Microcontroller, SPI_CS_PIN);

  TaskIsStarted = false;

  ESP_LOGI(Tag, "Communication is initialized");
}

//_____________________________________________________________________________

void Comm_Start(void)
{
  xTaskCreatePinnedToCore(
    RunCommTask,  // Function to implement the task
    "comm",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    7,            // Priority of the task
    &CommTask,    // Task handle
    0);           // Core where the task should run

  TaskIsStarted = true;
}

//_____________________________________________________________________________

void Comm_Stop(void)
{
  if (TaskIsStarted)
  {
    ESP_LOGW(Tag, "Comm task is stopped");

    TaskIsStarted = false;
    vTaskDelete(CommTask);
  }
}

//_____________________________________________________________________________

static void RunCommTask(void* arg)
{
  static uint8_t counter = 0;

  static int16_t tx[DATA_SIZE] = {0x0000};
  static int16_t rx[DATA_SIZE] = {0x0000};

  ESP_LOGI(Tag, "Start Comm Task");

  while (1)
  {
    STM32_UpdateSettings();
    STM32_UpdateMotorTargets();

    // Transmit values to the STM32
    // tx[0] is not used
    tx[1] = STM32_GetStatus();
    tx[2] = STM32_GetLeftMotorSettings();
    tx[3] = STM32_GetRightMotorSettings();
    tx[4] = STM32_GetLeftMotorTarget();
    tx[5] = STM32_GetRightMotorTarget();
    tx[6] = STM32_GetSoundThreshold();
    tx[7] = Behavior_GetStatus();

    Spi_WriteVSPI(Microcontroller, tx, rx, DATA_SIZE);

    //STM32_SetIdentifier(rx[0]);

    if (rx[0] == STM32_ID)
    {
      //ESP_LOGE(Tag, "Data is valid");
      STM32_SetStatus(rx[1]);
      STM32_SetLeftMotorSettings(rx[2]);
      STM32_SetRightMotorSettings(rx[3]);
      STM32_SetBatteryVoltage(rx[4]);
      STM32_SetLeftBatteryMotorVoltage(rx[5]);
      STM32_SetRightBatteryMotorVoltage(rx[6]);
      STM32_SetLeftInducedVoltage(rx[7]);
      STM32_SetRightInducedVoltage(rx[8]);
      STM32_SetLeftMotorCurrent(rx[9]);
      STM32_SetRightMotorCurrent(rx[10]);
      STM32_SetLeftPwmDutyCycle(rx[11]);
      STM32_SetRightPwmDutyCycle(rx[12]);
      STM32_SetSoundLevel(rx[13]);
      //STM32_SetSoundThreshold(rx[13]);
      STM32_SetSoundMean(rx[14]);
      STM32_SetProxIRValues(rx, PROX_IR_POSITION);
      STM32_SetGroundIRValues(rx, GROUND_IR_POSITION);

      //if ((rx[1] != 0x0001) || (tx[1] != 0x0001))
      if (rx[1] != 0x0001)
      {
        ESP_LOGE(Tag, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
      		     rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7], rx[8], rx[9], rx[10],
			     rx[11], rx[12], rx[13], rx[14], rx[15], rx[16], rx[17], rx[18], rx[19], rx[20]);
      }

      if ((counter % 5u) == 0u)  // Every 100 [ms], 10 [Hz] (vTaskDelay = 20 [ms])
      {
        Power_HandlePowerModeRequest();
      }

      counter++;

      SET_EVENT(EVENT_STM32);
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
