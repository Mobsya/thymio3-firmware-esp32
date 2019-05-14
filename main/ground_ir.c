//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ground_ir.c
//! \brief   This module provides the useful functions to use the Ground IR sensors
//!
//! \author  Vincent Gonet
//!
//! \version $Id: ground_ir.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "ground_ir.h"

#include "adc.h"
#include "aseba_esp32.h"
#include "board.h"
#include "gpio.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define GROUND_IR_PIN_NUM            2u

#define CALIB_HYSTERESIS             20

#define RIGHT_TASK_INTERVAL_us     375u
#define LEFT_TASK_INTERVAL_us      375u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Sensor_Left,
  E_Sensor_Right
} T_Sensor;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const T_GpioPinConfig PinConfig[GROUND_IR_PIN_NUM] =
{
  // PinNumber                Mode               Resistor             Level            Interrupt
  {IR_PULSE_GROUND_LEFT_PIN,  E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_PULSE_GROUND_RIGHT_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable}
};

static uint8_t ProxCalibMaxCounter[GROUND_IR_SENSORS_NUM];
static int16_t ProxGroundMax[GROUND_IR_SENSORS_NUM];       // the calibration is not stored in settings

static bool AmbientTaskIsInProgress = false;
static bool RightTaskIsInProgress   = false;
static bool LeftTaskIsInProgress    = false;

static T_TimerHw* RightTaskTimer  = NULL;
static T_TimerHw* LeftTaskTimer = NULL;

static TaskHandle_t RightTaskToNotify;
static TaskHandle_t LeftTaskToNotify;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static const char* Tag = "ground_ir";

static void RunGroundIRTask(void* arg);

static void RunRightGroundIRTask(void* arg);

static void RunLeftGroundIRTask(void* arg);

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor);

static int16_t Calibrate(int16_t value, T_Sensor sensor);

static void Callback_TimerRightTask(void* arg);

static void Callback_TimerLeftTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void GroundIR_Init(void)
{
  for (uint16_t index = 0u; index < GROUND_IR_PIN_NUM; index++)
  {
    Gpio_ConfigurePin(&PinConfig[index]);
  }

  RightTaskTimer = TimerHw_Create(RIGHT_TASK_INTERVAL_us, Callback_TimerRightTask);
  LeftTaskTimer  = TimerHw_Create(LEFT_TASK_INTERVAL_us, Callback_TimerLeftTask);

  ESP_LOGI(Tag, "Ground IR sensors are initialized");
}

//_____________________________________________________________________________

void GroundIR_Start(void)
{
  xTaskCreatePinnedToCore(
    RunGroundIRTask,  // Function to implement the task
    "sensor",         // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    4,                // Priority of the task
    NULL,             // Task handle
    0);               // Core where the task should run

  xTaskCreatePinnedToCore(
    RunRightGroundIRTask,  // Function to implement the task
    "sensor",              // Name of the task
    4096,                  // Stack size in words
    NULL,                  // Task input parameter
    4,                     // Priority of the task
    &RightTaskToNotify,    // Task handle
    0);                    // Core where the task should run

  xTaskCreatePinnedToCore(
    RunLeftGroundIRTask,  // Function to implement the task
    "sensor",             // Name of the task
    4096,                 // Stack size in words
    NULL,                 // Task input parameter
    4,                    // Priority of the task
    &LeftTaskToNotify,    // Task handle
    0);                   // Core where the task should run
}

//_____________________________________________________________________________

void GroundIR_Shutdown(void)
{
  Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);
  Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_Low);
}

//_____________________________________________________________________________

static void RunGroundIRTask(void* arg)
{
  ESP_LOGI(Tag, "Start Ground IR Task");

  uint16_t sensors[3];

  while (1)
  {
    AmbientTaskIsInProgress = true;

    ADC_AcquireGroundIRValues(sensors);

    vmVariables.ground_ambiant[0] = sensors[1];  // left
    vmVariables.ground_ambiant[1] = sensors[2];  // right

    Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_High);

    TimerHw_StartTimerOnce(RightTaskTimer, RIGHT_TASK_INTERVAL_us);
    //TimerHw_StartTimerOnce(LeftTaskTimer, LEFT_TASK_INTERVAL_us);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

static void RunRightGroundIRTask(void* arg)
{
  uint16_t sensors[3];

  ESP_LOGI(Tag, "Start Right Ground IR Task");

  while (1)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0u)
    {
      RightTaskIsInProgress = true;

      ADC_AcquireGroundIRValues(sensors);

      vmVariables.ground_reflected[1] = sensors[2];
      vmVariables.ground_delta[1] = PerformCalibration((sensors[2] - vmVariables.ground_ambiant[1]), E_Sensor_Right);

      Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_Low);
      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_High);

      TimerHw_StartTimerOnce(LeftTaskTimer, LEFT_TASK_INTERVAL_us);
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }
  }
}

//_____________________________________________________________________________

static void RunLeftGroundIRTask(void* arg)
{
  uint16_t sensors[3];

  ESP_LOGI(Tag, "Start Left Ground IR Task");

  while (1)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0u)
    {
      //ESP_LOGE(Tag, "L");

      LeftTaskIsInProgress = true;

      ADC_AcquireGroundIRValues(sensors);

      vmVariables.ground_reflected[0] = sensors[1];
      vmVariables.ground_delta[0] = PerformCalibration((sensors[1] - vmVariables.ground_ambiant[0]), E_Sensor_Left);

      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);

      SET_EVENT(EVENT_PROX);
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }
  }
}

//_____________________________________________________________________________

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor)
{
  int16_t value = raw;
  int16_t calibration = 0;

  if (settings.prox_ground_max[sensor] >= 0)
  {
    // On the fly re-calibration
    calibration = Calibrate(value, sensor);
  }
  else
  {
    // Calibration disabled if settings are negative
    calibration = value;
  }

  return calibration;
}

//_____________________________________________________________________________

static int16_t Calibrate(int16_t value, T_Sensor sensor)
{
  int16_t ret;

  if ((value + CALIB_HYSTERESIS) > ProxGroundMax[sensor])
  {
    if (++ProxCalibMaxCounter[sensor] > 3)
    {
      if (value > ProxGroundMax[sensor])
      {
        ProxGroundMax[sensor] = value;
      }
      else
      {
        ProxCalibMaxCounter[sensor] = 0;
      }
    }
  }
  else
  {
    ProxCalibMaxCounter[sensor] = 0;
  }

  if (ProxGroundMax[sensor] < 500)
  {
    ret = value;
  }
  else
  {
    ret = (int16_t)(((int32_t)value * 1024) / ProxGroundMax[sensor]);
  }

  if (ret < 0)
  {
    ret = 0;
  }

  return ret;
}

//_____________________________________________________________________________

static void Callback_TimerRightTask(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  if (AmbientTaskIsInProgress)
  {
    AmbientTaskIsInProgress = false;

    //TimerHw_StartTimerOnce(LeftTaskTimer, LEFT_TASK_INTERVAL_us);

    vTaskNotifyGiveFromISR(RightTaskToNotify, &higherPriorityTaskWoken);

    //if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
}

//_____________________________________________________________________________

static void Callback_TimerLeftTask(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  if (RightTaskIsInProgress)
  {
    RightTaskIsInProgress = false;

    vTaskNotifyGiveFromISR(LeftTaskToNotify, &higherPriorityTaskWoken);

    //if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
}


#if 0
static void Callback_TimerFirstInterval(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  if (AmbientTaskIsInProgress)
  {
    AmbientTaskIsInProgress = false;

    //TimerHw_Stop(FirstTimer);
    TimerHw_StartTimerOnce(FirstTimer, FIRST_INTERVAL_us);

    vTaskNotifyGiveFromISR(RightTaskToNotify, &higherPriorityTaskWoken);

    //if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
  if (RightTaskIsInProgress)
  {
    RightTaskIsInProgress = false;

    vTaskNotifyGiveFromISR(LeftTaskToNotify, &higherPriorityTaskWoken);

    //if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
}
#endif

//_____________________________________________________________________________
#if 0
static void Callback_TimerSecondInterval(void* arg)
{
  uint16_t sensors[3];

  ADC_AcquireGroundIRValues(sensors);

  vmVariables.ground_reflected[0] = sensors[1];
  vmVariables.ground_delta[0] = PerformCalibration((sensors[1] - vmVariables.ground_ambiant[0]), E_Sensor_Left);

  Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);

  SET_EVENT(EVENT_PROX);
}
#endif
