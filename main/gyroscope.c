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

#include <string.h>

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

static T_Axis AngularPosition;
static T_Axis Angle;
static T_Axis Angle_deg;

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

void Gyroscope_GetAngularPosition(void)
{
  LSM6DS3US_GetAngularPosition(&AngularPosition);

  vmVariables.gyro[0] = AngularPosition.X;
  vmVariables.gyro[1] = AngularPosition.Y;
  vmVariables.gyro[2] = AngularPosition.Z;

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

void Gyroscope_ReadAngle(void)
{
  LSM6DS3US_GetAngle(&Angle);

  vmVariables.angle[0] = Angle.X;
  vmVariables.angle[1] = Angle.Y;
  vmVariables.angle[2] = Angle.Z;

  Angle_deg.X = ((Angle.X * 90) / 16384);
  Angle_deg.Y = ((Angle.Y * 90) / 16384);
  Angle_deg.Z = ((Angle.Z * 90) / 16384);

  vmVariables.angle_deg[0] = Angle_deg.X;
  vmVariables.angle_deg[1] = Angle_deg.Y;
  vmVariables.angle_deg[2] = Angle_deg.Z;

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", vmVariables.angle_deg[0], vmVariables.angle_deg[1], vmVariables.angle_deg[2]);

  SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

T_Axis Gyroscope_GetAngle(void)
{
  return Angle_deg;
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
