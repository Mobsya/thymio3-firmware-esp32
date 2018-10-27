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

#include "esp_log.h"

#include "ground_ir.h"

#include "aseba_esp32.h"
#include "board.h"
#include "gpio.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define GROUND_IR_PIN_NUM    2u

#define SENSORS_NUM          2u

#define CALIB_HYSTERESIS     20

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Sensor_Right,
  E_Sensor_Left
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
//  {IR_SENSE_GROUND_LEFT_PIN,  E_GpioMode_Input,  E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
//  {IR_SENSE_GROUND_RIGHT_PIN, E_GpioMode_Input,  E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable}
};

static uint8_t ProxCalibMaxCounter[SENSORS_NUM];
static int16_t ProxGroundMax[SENSORS_NUM];  // the calibration is not stored in settings

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static const char* Tag = "ground_ir";

static int PerformCalibration(uint16_t raw, T_Sensor sensor);

static int16_t Calibrate(int16_t value, T_Sensor sensor);

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

  //ESP_LOGI(Tag, "Ground IR sensors are initialized");
}

//_____________________________________________________________________________

void GroundIR_Run(uint16_t left, uint16_t right, uint16_t tick)
{
  switch (tick)
  {
    case 50:
      Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_High);
      vmVariables.ground_ambiant[0] = right;
      vmVariables.ground_ambiant[1] = left;
      break;

    case 53:
      Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_Low);
      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_High);
      vmVariables.ground_reflected[0] = right;
      vmVariables.ground_delta[0] = PerformCalibration((right - vmVariables.ground_ambiant[0]), E_Sensor_Right);
      break;

    case 56:
      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);
      vmVariables.ground_reflected[1] = left;
      vmVariables.ground_delta[1] = PerformCalibration((left - vmVariables.ground_ambiant[1]), E_Sensor_Left);

      SET_EVENT(EVENT_PROX);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

void GroundIR_Shutdown(void)
{
  Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);
  Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_Low);
}

//_____________________________________________________________________________

static int PerformCalibration(uint16_t raw, T_Sensor sensor)
{
  int value;

  if (raw > 32767)
  {
    return 0;  // Sanity check
  }

  value = raw;

  if (settings.prox_ground_max[sensor] >= 0)
  {
    // On the fly re-calibration
    return Calibrate(value, sensor);
  }
  else
  {
    // Calibration disabled if settings are negative
    return value;
  }
}

//_____________________________________________________________________________

static int16_t Calibrate(int16_t value, T_Sensor sensor)
{
  int ret;

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
    ret = ((int32_t)value * 1024) / ProxGroundMax[sensor];
  }

  if (ret < 0)
  {
    ret = 0;
  }

  return ret;
}
