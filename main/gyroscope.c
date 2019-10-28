//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    gyroscope.c
//! \brief   This module provides the useful functions to use the gyroscope
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "gyroscope.h"

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

static const char* Tag = "gyroscope";

static T_Axis AngularVelocity;
static int16_t Angle[3];
static int16_t Angle_deg[3];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Gyroscope_Init(void)
{
  LSM6DS3US_InitGyroscope();

  ESP_LOGI(Tag, "Gyroscope is initialized");
}

//_____________________________________________________________________________

void Gyroscope_ReadAngularVelocity(void)
{
  LSM6DS3US_GetAngularVelocity(&AngularVelocity);

  vmVariables.gyro[0] = AngularVelocity.X;
  vmVariables.gyro[1] = AngularVelocity.Y;
  vmVariables.gyro[2] = AngularVelocity.Z;

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

void Gyroscope_ReadAngle(void)
{
  LSM6DS3US_GetAngle(Angle);

  for (uint8_t index = 0u; index < 3u; index++)
  {
    vmVariables.angle[index] = Angle[index];

    Angle_deg[index] = ((Angle[index] * 90) / 16384);

    vmVariables.angle_deg[index] = Angle_deg[index];
  }

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", vmVariables.angle_deg[0], vmVariables.angle_deg[1], vmVariables.angle_deg[2]);

  SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

int16_t Gyroscope_GetAngleZ(void)
{
  return Angle_deg[2];
}

//_____________________________________________________________________________

void Gyroscope_ResetAngle(void)
{
  LSM6DS3US_ResetAngle();

  vmVariables.angle[0] = 0;
  vmVariables.angle[1] = 0;
  vmVariables.angle[2] = 0;

  vmVariables.angle_deg[0] = 0;
  vmVariables.angle_deg[1] = 0;
  vmVariables.angle_deg[2] = 0;
}

//_____________________________________________________________________________

void Gyroscope_ResetCalibration(void)
{
  LSM6DS3US_ResetCalibration();
}
