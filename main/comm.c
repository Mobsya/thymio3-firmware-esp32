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

#include "behavior.h"
#include "board.h"
#include "i2c.h"
#include "power.h"
#include "stm32_i2c.h"
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

xSemaphoreHandle I2CMutex;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "comm";

static TaskHandle_t CommTask = NULL;

static bool TaskIsStarted = false;

//static spi_device_handle_t Microcontroller;

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

  //STM32_Init();

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
    4,            // Priority of the task
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

  //static uint8_t tx[5] = {0x0A, 0x0C, 0x0E, 0x01, 0x03};
  //static uint16_t rx[13] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  //static uint16_t rx[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

  ESP_LOGI(Tag, "Start Comm Task");

  while (1)
  {
    xSemaphoreTake(I2CMutex, portMAX_DELAY);

    //STM32_Communicate();

    //STM32_CheckId();
    STM32_ReadStatus();
//#if 0
    STM32_ReadBatteryVoltage();
    STM32_ReadProxIRValue();
    STM32_ReadGroundIRValue();
    STM32_ReadMicrophoneVoltage();
    STM32_ReadInducedVoltage();
    STM32_ReadBatteryMotorVoltage();
    STM32_ReadMotorCurrent();
    STM32_ReadPwmDutyCycle();
//#endif

    //Spi_WriteVSPI(Microcontroller, tx, 5);

//    Spi_ReadVSPI(Microcontroller, rx, 13);

    //if ((tx[0] != 0x0A) || (tx[1] != 0x0B) || (tx[2] != 0x0C) || (tx[3] != 0x0D) || (tx[4] != 0x0E))
    {
      //ESP_LOGI(Tag, "%d, %d, %d, %d, %d", tx[0], tx[1], tx[2], tx[3], tx[4]);
    }

    //if ((rx[0] != 0x70) || (rx[1] != 0x71) || (rx[2] != 0x72) || (rx[3] != 0x73) || (rx[4] != 0x74))
    //if ((rx[0] != 0x6A) || (rx[1] != 0x70) || (rx[2] != 0x71) || (rx[3] != 0x72) || (rx[4] != 0x73) || (rx[5] != 0x74))
#if 0
    if ((rx[0] != 0x0201) || (rx[1] != 0x0403) || (rx[2] != 0x0605) || (rx[3] != 0x0807) || (rx[4] != 0x0A09) ||
        (rx[5] != 0x0C0B) || (rx[6] != 0x0E0D) || (rx[7] != 0x000F) || (rx[8] != 0x0201) || (rx[9] != 0x0403) ||
        (rx[10] != 0x0605))
    {
      ESP_LOGI(Tag, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d", rx[0], rx[1], rx[2], rx[3], rx[4], rx[5], rx[6], rx[7],
               rx[8], rx[9], rx[10]);
    }
#endif

    //ESP_LOGI(Tag, "%d, %d, %d, %d, %d", rx[0], rx[1], rx[2], rx[3], rx[4]);
    //Spi_ReadVSPI();
//#if 0
    if ((counter % 5u) == 0u)  // Every 100 [ms], 10 [Hz] (vTaskDelay = 20 [ms])
    {
      Power_HandlePowerModeRequest();
    }
//#endif
    counter++;

    xSemaphoreGive(I2CMutex);

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}
