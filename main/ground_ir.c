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

#define CALIB_HYSTERESIS     20

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static const char* Tag = "ground_ir";

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor);

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

  ESP_LOGI(Tag, "Ground IR sensors are initialized");
}

//_____________________________________________________________________________

void GroundIR_EmitPulses(uint16_t tick, uint16_t left, uint16_t right)
{
  switch (tick)
  {
    case 50:
      vmVariables.ground_ambiant[0] = left;
      vmVariables.ground_ambiant[1] = right;

      Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_High);

      break;

    case 53:
      vmVariables.ground_reflected[1] = right;
      vmVariables.ground_delta[1] = PerformCalibration((right - vmVariables.ground_ambiant[1]), E_Sensor_Right);

      Gpio_SetPinLevel(IR_PULSE_GROUND_RIGHT_PIN, E_GpioLevel_Low);
      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_High);
      break;

    case 56:
      vmVariables.ground_reflected[0] = left;
      vmVariables.ground_delta[0] = PerformCalibration((left - vmVariables.ground_ambiant[0]), E_Sensor_Left);

      Gpio_SetPinLevel(IR_PULSE_GROUND_LEFT_PIN, E_GpioLevel_Low);

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
