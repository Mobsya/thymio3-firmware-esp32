//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: accelerometer.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "accelerometer.h"

#include "aseba_esp32.h"
#include "lsm303c.h"
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

static T_Acc_Axis Acceleration_3;
static T_Axis     Acceleration_6;

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
  LSM303C_InitAccelerometer();
  LSM6DS3US_InitAccelerometer();

  ESP_LOGI(Tag, "Accelerometers are initialized");
}

//_____________________________________________________________________________

void Accelerometer_GetAcceleration(void)
{
  LSM303C_GetAcceleration(&Acceleration_3);

  vmVariables.acc[0] = Acceleration_3.X;
  vmVariables.acc[1] = Acceleration_3.Y;
  vmVariables.acc[2] = Acceleration_3.Z;

  SET_EVENT(EVENT_ACC);

  LSM6DS3US_GetAcceleration(&Acceleration_6);

  vmVariables.acc_bis[0] = Acceleration_6.X;
  vmVariables.acc_bis[1] = Acceleration_6.Y;
  vmVariables.acc_bis[2] = Acceleration_6.Z;

  SET_EVENT(EVENT_ACC_BIS);
}

//_____________________________________________________________________________

void Accelerometer_GetTapSource(void)
{
  LSM6DS3US_GetTapSource(&TapSource);

  vmVariables.acc_tap = TapSource;

  if (TapSource > 0)
  {
    SET_EVENT(EVENT_TAP);
  }
}
