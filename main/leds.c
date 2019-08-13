//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    leds.c
//! \brief   This module provides the useful functions to use the LEDs
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
#include "freertos/queue.h"

#include "esp_log.h"

#include "leds.h"

#include "shift_registers.h"
#include "timer_sw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MAX_LEDS_NUM            (REGISTERS_NUM * PINS_PER_REGISTER_NUM)

#define LED_OFF_BANK_0          0x0Fu  //!< LSB --> U26.QA
#define LED_OFF_BANK_1          0x0Fu  //!< LSB --> U27.QA
#define LED_OFF_BANK_2          0x00u  //!< LSB --> U25.QA
#define LED_OFF_BANK_3          0x0Fu  //!< LSB --> U28.QA
#define LED_OFF_BANK_4          0x0Fu  //!< LSB --> U29.QA
#define LED_OFF_BANK_5          0x00u  //!< LSB --> U30.QA

#define LEDS_FREQUENCY_Hz      100.0F  //!< Frequency of the LEDs in [Hz]

#define TIMING_FACTOR      1000000.0F  //!< Factor to convert [s] to [us]

//!< Calculation of the LEDs task period in [Hz]
#define LEDS_TASK_PERIOD_Hz      (LEDS_FREQUENCY_Hz * MAX_BRIGHTNESS)

//! Calculation of the LEDs task period in [us]
const uint32_t LEDS_TASK_PERIOD_us = (uint32_t)((double)(1.0 / LEDS_TASK_PERIOD_Hz) * TIMING_FACTOR);

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "leds";

//                       Rows            Columns
static uint8_t LedsTable[MAX_BRIGHTNESS][REGISTERS_NUM];

static const uint8_t LedsOff[REGISTERS_NUM] = {LED_OFF_BANK_0,
                                               LED_OFF_BANK_1,
                                               LED_OFF_BANK_2,
                                               LED_OFF_BANK_3,
                                               LED_OFF_BANK_4,
                                               LED_OFF_BANK_5
                                              };

static T_TimerSw* LedsTaskTimer = NULL;  //!< Used to schedule the Leds task

static TaskHandle_t TaskToNotify;        //!< Used to notified the Leds task

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the LEDs task
//! \pre       First initialize the LEDs
//! \param     arg - Task parameter
//! \return    None
static void RunLedsTask(void* arg);

//! \brief     Called when the LedsTaskTimer elapsed
//! \pre       First initialize the LEDs
//! \param     arg - Task parameter
//! \return    None
static void Callback_TimerLedsTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Leds_Init(void)
{
  ShiftRegisters_Init();

  for (uint8_t row = 0u; row < MAX_BRIGHTNESS; row++)
  {
    for (uint8_t column = 0u; column < REGISTERS_NUM; column++)
    {
      LedsTable[row][column] = LedsOff[column];
    }
  }

  ShiftRegisters_Fill(&LedsTable[0][0], REGISTERS_NUM);

  LedsTaskTimer = TimerSw_Create(LEDS_TASK_PERIOD_us, Callback_TimerLedsTask);

  ESP_LOGI(Tag, "LEDs are initialized at %d [us]", LEDS_TASK_PERIOD_us);
}

//_____________________________________________________________________________

void Leds_Start(void)
{
  xTaskCreatePinnedToCore(
    RunLedsTask,    // Function to implement the task
    "sensor",       // Name of the task
    4096,           // Stack size in words
    NULL,           // Task input parameter
    2,              // Priority of the task
    &TaskToNotify,  // Task handle
    0);             // Core where the task should run
}

//_____________________________________________________________________________

void Leds_SetSingleBrightness(T_Led led, uint8_t brightness)
{
  uint8_t bank = 0u;
  uint8_t pin = 0u;
  uint8_t polarity = 0u;
  uint8_t position = 0u;

  if (led < MAX_LEDS_NUM)
  {
    bank = (led >> 0x03u);
    pin = (led & 0x07u);
    position = (1u << pin);
    polarity = (LedsOff[bank] & position);

    for (uint8_t row = 0u; row < MAX_BRIGHTNESS; row++)
    {
      if (row < brightness)
      {
        if (polarity != 0u)
        {
          LedsTable[row][bank] &= ~position;
        }
        else
        {
          LedsTable[row][bank] |= position;
        }
      }
      else
      {
        if (polarity != 0u)
        {
          LedsTable[row][bank] |= position;
        }
        else
        {
          LedsTable[row][bank] &= ~position;
        }
      }
    }
  }
}

//_____________________________________________________________________________

void Leds_SetCircleBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
		                      uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7)
{
  Leds_SetSingleBrightness(E_Led_Circle_0, l0);
  Leds_SetSingleBrightness(E_Led_Circle_1, l1);
  Leds_SetSingleBrightness(E_Led_Circle_2, l2);
  Leds_SetSingleBrightness(E_Led_Circle_3, l3);
  Leds_SetSingleBrightness(E_Led_Circle_4, l4);
  Leds_SetSingleBrightness(E_Led_Circle_5, l5);
  Leds_SetSingleBrightness(E_Led_Circle_6, l6);
  Leds_SetSingleBrightness(E_Led_Circle_7, l7);
}

//_____________________________________________________________________________

void Leds_SetLegoFrontBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
		                         uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7)
{
  Leds_SetSingleBrightness(E_Led_Lego_Front_0, l0);
  Leds_SetSingleBrightness(E_Led_Lego_Front_1, l1);
  Leds_SetSingleBrightness(E_Led_Lego_Front_2, l2);
  Leds_SetSingleBrightness(E_Led_Lego_Front_3, l3);
  Leds_SetSingleBrightness(E_Led_Lego_Front_4, l4);
  Leds_SetSingleBrightness(E_Led_Lego_Front_5, l5);
  Leds_SetSingleBrightness(E_Led_Lego_Front_6, l6);
  Leds_SetSingleBrightness(E_Led_Lego_Front_7, l7);
}

//_____________________________________________________________________________

void Leds_SetLegoBackBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
		                        uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7)
{
  Leds_SetSingleBrightness(E_Led_Lego_Back_0, l0);
  Leds_SetSingleBrightness(E_Led_Lego_Back_1, l1);
  Leds_SetSingleBrightness(E_Led_Lego_Back_2, l2);
  Leds_SetSingleBrightness(E_Led_Lego_Back_3, l3);
  Leds_SetSingleBrightness(E_Led_Lego_Back_4, l4);
  Leds_SetSingleBrightness(E_Led_Lego_Back_5, l5);
  Leds_SetSingleBrightness(E_Led_Lego_Back_6, l6);
  Leds_SetSingleBrightness(E_Led_Lego_Back_7, l7);
}

//_____________________________________________________________________________

void Leds_SetColorSensorBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_R_Color_Sensor, red);
  Leds_SetSingleBrightness(E_Led_G_Color_Sensor, green);
  Leds_SetSingleBrightness(E_Led_B_Color_Sensor, blue);
}

//_____________________________________________________________________________

void Leds_SetFrontLeftBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_R_Front_Left, red);
  Leds_SetSingleBrightness(E_Led_G_Front_Left, green);
  Leds_SetSingleBrightness(E_Led_B_Front_Left, blue);
}

//_____________________________________________________________________________

void Leds_SetFrontRightBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_R_Front_Right, red);
  Leds_SetSingleBrightness(E_Led_G_Front_Right, green);
  Leds_SetSingleBrightness(E_Led_B_Front_Right, blue);
}

//_____________________________________________________________________________

void Leds_SetBackLeftBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_R_Back_Left, red);
  Leds_SetSingleBrightness(E_Led_G_Back_Left, green);
  Leds_SetSingleBrightness(E_Led_B_Back_Left, blue);
}

//_____________________________________________________________________________

void Leds_SetBackRightBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_R_Back_Right, red);
  Leds_SetSingleBrightness(E_Led_G_Back_Right, green);
  Leds_SetSingleBrightness(E_Led_B_Back_Right, blue);
}

//_____________________________________________________________________________

void Leds_SetDebugBrightness(uint8_t green, uint8_t blue)
{
  Leds_SetSingleBrightness(E_Led_G_Debug, green);
  Leds_SetSingleBrightness(E_Led_B_Debug, blue);
}

//_____________________________________________________________________________

void Leds_SetBodyBrightness(uint8_t red, uint8_t green, uint8_t blue)
{
#if 0
  Leds_SetTopBrightness(red, green, blue);
  Leds_SetBottomLeftBrightness(red, green, blue);
  Leds_SetBottomRightBrightness(red, green, blue);
#endif
}

//_____________________________________________________________________________

static void RunLedsTask(void* arg)
{
  static uint8_t row = 0u;

  ESP_LOGI(Tag, "Start Leds Task");

  TimerSw_StartTimerPeriodically(LedsTaskTimer, LEDS_TASK_PERIOD_us);

  while (1)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0u)
    {
      ShiftRegisters_Fill(&LedsTable[row][0u], REGISTERS_NUM);

      row++;

      if (row == MAX_BRIGHTNESS)
      {
        row = 0u;
      }
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }
  }
}

//_____________________________________________________________________________

static void Callback_TimerLedsTask(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  vTaskNotifyGiveFromISR(TaskToNotify, &higherPriorityTaskWoken);

  if (higherPriorityTaskWoken != pdFALSE)
  {
    portYIELD_FROM_ISR();
  }
}
