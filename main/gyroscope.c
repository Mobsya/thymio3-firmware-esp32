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
//! \author  Vincent Gonet, Stefano Morgani
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
#include "i2c.h"

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

static uint8_t currGyro = LSM6DS3US;

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

	uint8_t data = 0x00u;
	I2C_ReadFromAddress(0x6B, 0x0F, &data, 1u); // WHO_AM_I register
	//ESP_LOGD(Tag, "LSM6DS id = %x", data);
	if(data == 0x69) {
		currGyro = LSM6DS3US;
		LSM6DS3US_InitGyroscope(offset);
	} else if(data == 0x6A) {
		currGyro = LSM6DS3TR;
		LSM6DS3TR_InitGyroscope(offset);
	} else if(data == 0x6C) {
		currGyro = LSM6DS0;
		LSM6DS0_InitGyroscope(offset);
	}

  ESP_LOGI(Tag, "Gyroscope is initialized");
}

//_____________________________________________________________________________

void Gyroscope_ReadAngularVelocity(void)
{
	if(currGyro == LSM6DS3US) {
		LSM6DS3US_GetAngularVelocity(&AngularVelocity);
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_GetAngularVelocity(&AngularVelocity);
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_GetAngularVelocity(&AngularVelocity);
	}

  vmVariables.gyro[0] = AngularVelocity.X;
  vmVariables.gyro[1] = AngularVelocity.Y;
  vmVariables.gyro[2] = AngularVelocity.Z;

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

void Gyroscope_ReadAngle(void)
{
	if(currGyro == LSM6DS3US) {
		LSM6DS3US_GetAngle(Angle);
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_GetAngle(Angle);
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_GetAngle(Angle);
	}

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
	if(currGyro == LSM6DS3US) {
		LSM6DS3US_ResetAngle();
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_ResetAngle();
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_ResetAngle();
	}

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
	if(currGyro == LSM6DS3US) {
		LSM6DS3US_ResetCalibration();
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_ResetCalibration();
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_ResetCalibration();
	}
}

//_____________________________________________________________________________

void Gyroscope_SetOffset(int32_t offset)
{
	if(currGyro == LSM6DS3US) {
		LSM6DS3US_SetOffset(offset);
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_SetOffset(offset);
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_SetOffset(offset);
	}

  // Write to the settings file
  Settings_WriteOffsetGyro(offset);
}
