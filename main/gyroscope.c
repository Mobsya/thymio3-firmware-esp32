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

void Gyroscope_GetAngle(void)
{
  LSM6DS3US_GetAngle(&Angle);

  vmVariables.angle[0] = Angle.X;
  vmVariables.angle[1] = Angle.Y;
  vmVariables.angle[2] = Angle.Z;

  vmVariables.angle_deg[0] = ((Angle.X * 90) / 16384);
  vmVariables.angle_deg[1] = ((Angle.Y * 90) / 16384);
  vmVariables.angle_deg[2] = ((Angle.Z * 90) / 16384);

  ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", vmVariables.angle_deg[0], vmVariables.angle_deg[1], vmVariables.angle_deg[2]);

  SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

void Gyroscope_ResetAngle(void)
{
  LSM6DS3US_ResetAngle();

  Angle.X = 0;
  Angle.Y = 0;
  Angle.Z = 0;

  vmVariables.angle[0] = Angle.X;
  vmVariables.angle[1] = Angle.Y;
  vmVariables.angle[2] = Angle.Z;
}

//_____________________________________________________________________________

void Gyroscope_ResetCalibration(void)
{
  LSM6DS3US_ResetCalibration();
}
