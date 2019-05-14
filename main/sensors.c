//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sensors.c
//! \brief   This module provides the useful functions to use the no-I2C sensors
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sensors.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "sensors.h"

#include "adc.h"
#include "ground_ir.h"
#include "leds.h"
#include "prox_ir.h"
#include "sound.h"
#include "timer.h"
#include "timer_hw.h"

#include "i2s.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SENSORS_TASK_PERIOD_us  125u  //!< Sensors task frequency = 8 [kHz]

#define PERIOD_100_ms           799u  //!< 125us * 800 = 100ms

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

static TaskHandle_t TaskToNotify;

static int16_t PeriodAccumulator = 0;

static T_TimerHw* SensorsTaskTimer = NULL;  //!< Used to schedule the Sensors task

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the sensor task
//! \pre       First initialize the sensors
//! \param     arg - Task parameter
//! \return    None
static void RunSensorsTask(void* arg);

//! \brief     Run the sensors
//! \pre       First initialize the sensors
//! \param     None
//! \return    None
static void RunSensors(void);

static void Callback_TimerSensorsTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sensors_Init(void)
{
  //ADC_Init();
  Leds_Init();
  //GroundIR_Init();
  //ProxIR_Init();

  //TaskToNotify = xTaskGetCurrentTaskHandle();

  SensorsTaskTimer = TimerHw_Create(SENSORS_TASK_PERIOD_us, Callback_TimerSensorsTask);
  //Timer_Init(0, 1, 1, 1);         // Timer used to handle the IR_SENSE_BACK_RIGHT_PIN

  ESP_LOGI(Tag, "Sensors are initialized");
}

//_____________________________________________________________________________

void Sensors_Start(void)
{
  xTaskCreatePinnedToCore(
    RunSensorsTask,  // Function to implement the task
    "sensor",        // Name of the task
    4096,            // Stack size in words
    NULL,            // Task input parameter
    3,               // Priority of the task
    &TaskToNotify,   // Task handle
    0);              // Core where the task should run
}

//_____________________________________________________________________________

static void RunSensorsTask(void* arg)
{
  uint32_t result = 0;
  //const TickType_t maxBlockTime = pdMS_TO_TICKS(500);

  ESP_LOGI(Tag, "Start Sensor Task");

  // Attempt to create a notification
  //TaskToNotify = xTaskGetCurrentTaskHandle();

  TimerHw_StartTimerPeriodically(SensorsTaskTimer, SENSORS_TASK_PERIOD_us);
  //Timer_Start(0, 1);

  while (1)
  {
    //result = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // portMAX_DELAY

    if (xTaskNotifyWait(0, 0, &result, portMAX_DELAY) == pdTRUE)  // portMAX_DELAY
      //if (result == 1)
    {
      //Leds_Run();
      //RunSensors();

      //xTaskNotifyGive(AcquisitionTask);
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }

    //taskYIELD();
    //FeedWatchdog();
  }
}

//_____________________________________________________________________________

static void RunSensors(void)
{
  static uint16_t tick = 0u;

  uint16_t micro = 0u;
  float val = 0.0;

  uint16_t sensors[3];

  if ((tick == 50) || (tick == 53) || (tick == 56))
  {
    ADC_AcquireGroundIRValues(sensors);
  }
#if 0
  if (tick % 100 == 0)  // Every 12.5 [ms]
  {
    I2S_AcquireMicrophoneValues();
  }
#endif
#if 0
  if (tick % 8 == 0)  // 8 = Every 1 [ms], 4 = Every 500 [us], 2 = Every 250 [us]
  {
    ADC_AcquireMicrophoneValues(&micro);

    val = ((micro * 1.1) / 4095) * 3.6;

    Sound_Process(&val, 1);
  }
#endif

  //ProxIR_Run(tick);
  //PeriodAccumulator += ProxIR_Run(tick);
  //PeriodAccumulator = 0;

  if (PeriodAccumulator > 100)
  {
    PeriodAccumulator = 100;
  }

  if (PeriodAccumulator < -100)
  {
    PeriodAccumulator = -100;
  }

  // The ground IR sensors need a period of 100ms (frequency = 10Hz),
  // The ground IR sensors trigger at time = 50, need 6 cycles
  //GroundIR_EmitPulses(tick, sensors[1], sensors[2]);

  if (PeriodAccumulator < 0)
  {
    if (tick == PERIOD_100_ms)
    {
      PeriodAccumulator++;
      tick++;
    }
    else if (tick > PERIOD_100_ms)
    {
      tick = 0u;
    }
    else
    {
      tick++;
    }
  }
  else if (PeriodAccumulator > 0)
  {
    if (tick == (PERIOD_100_ms - 1u))
    {
      PeriodAccumulator--;
      tick = 0u;
    }
    else if (tick++ >= PERIOD_100_ms)
    {
      // We have to re-check as per_acc might be set when time == PERIOD_100MS
      tick = 0u;
    }
    else
    {
      // Do nothing
    }
  }
  else
  {
    if (tick < PERIOD_100_ms)
    {
      tick++;
    }
    else
    {
      tick = 0u;
    }
  }
}

//_____________________________________________________________________________

static void Callback_TimerSensorsTask(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  BaseType_t result;

  //vTaskNotifyGiveFromISR(TaskToNotify, &higherPriorityTaskWoken);
  result = xTaskNotifyFromISR(TaskToNotify, 0, eNoAction, &higherPriorityTaskWoken);

  // If the call to xTaskNotifyFromISR() returns pdFAIL then the task
  // is not keeping up with the rate at which the timer elapsed.
  //configASSERT(result == pdTRUE);

  if (higherPriorityTaskWoken != pdFALSE)
  {
    portYIELD_FROM_ISR();
  }
}
