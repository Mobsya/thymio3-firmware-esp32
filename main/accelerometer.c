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
//! \author  Vincent Gonet, Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "accelerometer.h"

#include "aseba_esp32.h"
#include "gpio.h"
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

static const char* Tag = "accelerometer";

static T_Axis  Acceleration;

static uint8_t TapSource;
static uint8_t currAcc = LSM6DS3US;

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
	uint8_t data = 0x00u;
	I2C_ReadFromAddress(0x6B, 0x0F, &data, 1u);
	ESP_LOGI(Tag, "LSM6DS id = %x", data);
	if(data == 0x69) {
		currAcc = LSM6DS3US;
		LSM6DS3US_InitAccelerometer();
	} else if(data == 0x6A) {
		currAcc = LSM6DS3TR;
		LSM6DS3TR_InitAccelerometer();
	} else if(data == 0x6C) {
		currAcc = LSM6DS0;
		LSM6DS0_InitAccelerometer();
	}

  ESP_LOGI(Tag, "Accelerometer is initialized");
}

//_____________________________________________________________________________

void Accelerometer_ReadAcceleration(void)
{
	if(currAcc == LSM6DS3US) {
		LSM6DS3US_ReadAcceleration(&Acceleration);
	} else if(currAcc == LSM6DS3TR) {
		LSM6DS3TR_ReadAcceleration(&Acceleration);
	} else if(currAcc == LSM6DS0) {
		LSM6DS0_ReadAcceleration(&Acceleration);
	}

  vmVariables.acc[0] = Acceleration.X;
  vmVariables.acc[1] = Acceleration.Y;
  vmVariables.acc[2] = Acceleration.Z;

  //SET_EVENT(EVENT_ACC);
}

//_____________________________________________________________________________

T_Axis Accelerometer_GetAcceleration(void)
{
  return Acceleration;
}

//_____________________________________________________________________________

int16_t Accelerometer_GetAccelerationY(void)
{
  return Acceleration.Y;
}

//_____________________________________________________________________________

int16_t Accelerometer_GetAccelerationZ(void)
{
  return Acceleration.Z;
}

//_____________________________________________________________________________

void Accelerometer_ReadTapSource(void)
{
	if(currAcc == LSM6DS3US) {
		LSM6DS3US_ReadTapSource(&TapSource);
	} else if(currAcc == LSM6DS3TR) {
		LSM6DS3TR_ReadTapSource(&TapSource);
	} else if(currAcc == LSM6DS0) {
		LSM6DS0_ReadTapSource(&TapSource);
	}

  vmVariables.acc_tap = TapSource;

#if 0
  if (TapSource > 0u)
  {
    SET_EVENT(EVENT_TAP);
  }
#endif
}

//_____________________________________________________________________________

uint8_t Accelerometer_GetTapSource(void)
{
  return TapSource;
}

//_____________________________________________________________________________

bool Accelerometer_IsTapDetected(void)
{
  bool result = Gpio_IsTapDetected();

  Gpio_ClearTapStatus();

  return result;
}

//_____________________________________________________________________________

void Accelerometer_ClearTapStatus(void)
{
  Gpio_ClearTapStatus();
}

//_____________________________________________________________________________

bool Accelerometer_IsFreeFallDetected(void)
{
  bool result = Gpio_IsFreeFallDetected();

  Gpio_ClearFreeFallStatus();

  return result;
}


//_____________________________________________________________________________

T_Error Accelerometer_CheckManufacturerId(void)
{
  T_Error err = E_Error_None;

	if(currAcc == LSM6DS3US) {
		  if (LSM6DS3US_CheckManufacturerId() != E_Error_None)
		  {
		    err = E_Error_Acc_InvalidID;
		  }
	} else if(currAcc == LSM6DS3TR) {
		  if (LSM6DS3TR_CheckManufacturerId() != E_Error_None)
		  {
		    err = E_Error_Acc_InvalidID;
		  }
	} else if(currAcc == LSM6DS0) {
		  if (LSM6DS0_CheckManufacturerId() != E_Error_None)
		  {
		    err = E_Error_Acc_InvalidID;
		  }
	}

  ESP_LOGI(Tag, "Accelerometer test is done");

  return err;
}
