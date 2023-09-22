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
#include "power.h"
#include "settings.h"
#include "spi.h"
#include "stm32_spi.h"
#include "uart.h"
#include <string.h>
#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DATA_SIZE                            40u

//#define SETTINGS_POSITION                     2u
#define BATTERY_MOTOR_VOLTAGES_POSITION       3u
#define INDUCED_VOLTAGES_POSITION             5u
#define MOTOR_CURRENTS_POSITION               7u
#define PWM_DUTY_CYCLES_POSITION              9u
#define PROX_IR_POSITION                     13u
#define GROUND_IR_POSITION                   20u
#define PROX_IR_DATA_POSITION                26u

#define STM32_ID                          0x4321

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
  //UART_Init();

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

  DMA_ATTR static int16_t tx[DATA_SIZE] = {0x0000};
  DMA_ATTR static int16_t rx[DATA_SIZE] = {0x0000};

  ESP_LOGI(Tag, "Start Comm Task");

  while (1)
  {
    Settings_UpdateSettings();
    //STM32_UpdateMotorTargets(); // This is needed to update the motors target speed from Aseba

    // Transmit values to the STM32
    // tx[0] is not used
    tx[1] = STM32_GetStatus();
    tx[2] = Settings_GetLeftMotorSettings();
    tx[3] = Settings_GetRightMotorSettings();
    tx[4] = STM32_GetLeftMotorTarget();
    tx[5] = STM32_GetRightMotorTarget();
    tx[6] = STM32_GetMicrophoneThreshold();
    tx[7] = Behavior_GetStatus();
    tx[8] = STM32_GetProxIRTxData();

    Spi_CommunicateVSPI(Microcontroller, tx, rx, DATA_SIZE);

    if (rx[0] == STM32_ID)
    {
      STM32_SetStatus(rx[1]);
      STM32_SetBatteryVoltage(rx[2]);
      STM32_SetBatteryMotorVoltages(rx, BATTERY_MOTOR_VOLTAGES_POSITION);
      STM32_SetInducedVoltages(rx, INDUCED_VOLTAGES_POSITION);
      STM32_SetMotorCurrents(rx, MOTOR_CURRENTS_POSITION);
      STM32_SetPwmDutyCycles(rx, PWM_DUTY_CYCLES_POSITION);
      STM32_SetMicrophoneIntensity(rx[11]);
      //ESP_LOGE(Tag, "mic = %d", rx[11]);
      //STM32_SetMicrophoneThreshold(rx[13]);
      STM32_SetMicrophoneMean(rx[12]); // Not implemented in the STM32
      STM32_SetProxIRValues(rx, PROX_IR_POSITION);
      STM32_SetGroundIRValues(rx, GROUND_IR_POSITION);
      STM32_SetProxIRData(rx, PROX_IR_DATA_POSITION);

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
