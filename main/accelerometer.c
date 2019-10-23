//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    accelerometer.c
//! \brief   This module provides the useful functions to use the accelerometer
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "accelerometer.h"

#include "aseba_esp32.h"
#include "lsm6ds3us.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "accelerometer";

static T_Axis  Acceleration;

static uint8_t TapSource;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Accelerometer_Init(void)
{
  LSM6DS3US_InitAccelerometer();

  ESP_LOGI(Tag, "Accelerometer is initialized");
}

//_____________________________________________________________________________

void Accelerometer_ReadAcceleration(void)
{
  LSM6DS3US_GetAcceleration(&Acceleration);

  vmVariables.acc[0] = Acceleration.X;
  vmVariables.acc[1] = Acceleration.Y;
  vmVariables.acc[2] = Acceleration.Z;

  SET_EVENT(EVENT_ACC);
}

//_____________________________________________________________________________

int16_t Accelerometer_GetAccelerationY(void)
{
  return Acceleration.Y;
}

//_____________________________________________________________________________

void Accelerometer_ReadTapSource(void)
{
  LSM6DS3US_GetTapSource(&TapSource);

  vmVariables.acc_tap = TapSource;

  if (TapSource > 0u)
  {
    SET_EVENT(EVENT_TAP);
  }
}

//_____________________________________________________________________________

uint8_t Accelerometer_GetTapSource(void)
{
  return TapSource;
}

//_____________________________________________________________________________

T_Error Accelerometer_CheckManufacturerId(void)
{
  T_Error err = E_Error_None;

  if (LSM6DS3US_CheckManufacturerId() != E_Error_None)
  {
    err = E_Error_Acc_InvalidID;
  }

  ESP_LOGI(Tag, "Accelerometer test is done");

  return err;
}
