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
#include "settings.h"

#ifdef LSM6DS3US
#include "lsm6ds3us.h"
#else
#include "lsm6ds3tr.h"	
#endif 
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
  int16_t offset = Settings_ReadOffsetGyro();
#ifdef LSM6DS3US
  LSM6DS3US_InitGyroscope(offset);
#else
  LSM6DS3TR_InitGyroscope(offset);
#endif 


  ESP_LOGI(Tag, "Gyroscope is initialized");
}

//_____________________________________________________________________________

void Gyroscope_ReadAngularVelocity(void)
{
#ifdef LSM6DS3US
    LSM6DS3US_GetAngularVelocity(&AngularVelocity);
#else
    LSM6DS3TR_GetAngularVelocity(&AngularVelocity);
#endif 


  vmVariables.gyro[0] = AngularVelocity.X;
  vmVariables.gyro[1] = AngularVelocity.Y;
  vmVariables.gyro[2] = AngularVelocity.Z;

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

void Gyroscope_ReadAngle(void)
{
#ifdef LSM6DS3US
    LSM6DS3US_GetAngle(Angle);
#else
    LSM6DS3TR_GetAngle(Angle);
#endif 
  

  for (uint8_t index = 0u; index < 3u; index++)
  {
    vmVariables.angle[index] = Angle[index];

    Angle_deg[index] = ((Angle[index] * 90) / 16384);

    vmVariables.angle_deg[index] = Angle_deg[index];
  }

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", vmVariables.angle_deg[0], vmVariables.angle_deg[1], vmVariables.angle_deg[2]);

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

int16_t Gyroscope_GetAngularVelocityZ(void)
{
  return AngularVelocity.Z;
}

//_____________________________________________________________________________

int16_t Gyroscope_GetAngleZ(void)
{
  return Angle[2];
}

//_____________________________________________________________________________

int16_t Gyroscope_GetAngleZ_deg(void)
{
  return Angle_deg[2];
}

//_____________________________________________________________________________

void Gyroscope_ResetAngle(void)
{
#ifdef LSM6DS3US
    LSM6DS3US_ResetAngle();
#else
    LSM6DS3TR_ResetAngle();
#endif


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
#ifdef LSM6DS3US
	LSM6DS3US_ResetCalibration();
#else
	LSM6DS3TR_ResetCalibration();
#endif	

}

//_____________________________________________________________________________

void Gyroscope_SetOffset(int32_t offset)
{
#ifdef LSM6DS3US
	LSM6DS3US_SetOffset(offset);
#else
	LSM6DS3TR_SetOffset(offset);
#endif


  // Write to the settings file
  Settings_WriteOffsetGyro(offset);
}
