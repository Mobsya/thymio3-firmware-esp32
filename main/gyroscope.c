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
#include <string.h>

#include "esp_log.h"

#include "gyroscope.h"
#include "common.h"
#include "aseba_esp32.h"
#include "settings.h"
#include "i2c.h"
#include "stm32_spi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "sensors.h"   // for I2CMutex

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define STATIONARY_THR 650 // Threshold used for calibration: when value is lower than the threshold, then it means the robot is still and value can be used for calibration.

#define CALIB_TARGET_SAMPLES   52 // half a second of data @ 104 Hz ODR
#define CALIB_POLL_MS          100u // @ 104 Hz ODR we should get about 10 samples per poll
#define CALIB_MAX_TRIALS       20u  // 20 * 100 ms = 2 s upper bound
#define CALIB_SETTLE_MS        300u // Settling time before starting the acquisition

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
static int32_t Angle[3];
//static float AngleFloat[3];
static int16_t Angle_deg[3];

static uint8_t currGyro = LSM6DS3US;

int16_t GyroBuffer[3][GYRO_BUFFER_SIZE];

int32_t ZeroGyroSum[3] = {0, 0, 0};
uint16_t ZeroGyroNumSamples[3] = {0, 0, 0};
int16_t ZeroGyro[3] = {0, 0, 0};

int32_t Mul = 0;
int32_t Div = 1;
int32_t Offset = 0;

bool calibrationInProgress = false;
bool continuousCalibrationEnabled = false;

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
  int16_t offset = Settings_GetOffsetGyroSettings();

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
	} else {
		currGyro = GYRO_NOT_AVAILABLE;
	}

  ESP_LOGI(Tag, "Gyroscope is initialized");
}

//_____________________________________________________________________________

void Gyroscope_ReadAngularVelocity(void)
{
	if(currGyro == GYRO_NOT_AVAILABLE) {
		AngularVelocity.X = 0;
		AngularVelocity.Y = 0;
		AngularVelocity.Z = 0;
		vmVariables.gyro[0] = 0;
  		vmVariables.gyro[1] = 0;
  		vmVariables.gyro[2] = 0;
		return;
	} else if(currGyro == LSM6DS3US) {
		LSM6DS3US_ReadAngularVelocity(&AngularVelocity);
	} else if(currGyro == LSM6DS3TR) {
		LSM6DS3TR_ReadAngularVelocity(&AngularVelocity);
	} else if(currGyro == LSM6DS0) {
		LSM6DS0_ReadAngularVelocity(&AngularVelocity);
	}

	if(HARDWARE_VERSION >= 0x0D)
	{
		AngularVelocity.Y = -AngularVelocity.Y;
		AngularVelocity.Z = -AngularVelocity.Z;
	}	

  vmVariables.gyro[0] = AngularVelocity.X;
  vmVariables.gyro[1] = AngularVelocity.Y;
  vmVariables.gyro[2] = AngularVelocity.Z;

  //SET_EVENT(EVENT_GYRO);
}

//_____________________________________________________________________________

T_Axis Gyroscope_GetAngularVelocity(void)
{
  return AngularVelocity;
}

//_____________________________________________________________________________

static void CalculateAngle(int32_t* angle, uint16_t number)
{
  int32_t sum[3] = {0, 0, 0};
  int64_t gyroCorr[3] = {0, 0, 0};
  //ESP_LOGI(Tag, "number=%d", number);
  
  if (number > GYRO_BUFFER_SIZE)   // Safety measure
  {
    number = GYRO_BUFFER_SIZE;
  }

  for (uint8_t axis = 0u; axis < 3u; axis++)
  {
    for (uint16_t index = 0u; index < number; index++)
    {
      sum[axis] += GyroBuffer[axis][index];
    }

    gyroCorr[axis] = (sum[axis] - (number * ZeroGyro[axis]));
    angle[axis] += (((Mul + Offset) * gyroCorr[axis]) / Div);
	//AngleFloat[axis] += ((((float)Mul + (float)Offset) * ((float)gyroCorr[axis])/2.0) / (float)Div); // Divided by 2 when using 250 dps for gyro configuration
    //ESP_LOGI(Tag, "x=%d, y=%d, z=%d", angle[0], angle[1], angle[2]);
	//Angle_deg[axis] = gyroCorr[axis]*250*number/28571/104;
  }
}

//_____________________________________________________________________________

//! \brief Read the gyroscope FIFO into GyroBuffer.
//! \pre   The caller MUST already hold I2CMutex.
static uint16_t ReadBufferedSamplesUnlocked(void)
{
    uint16_t num = 0;

    if (currGyro == LSM6DS3US) {
        num = LSM6DS3US_ReadBufferedAngularPosition();
    } else if (currGyro == LSM6DS3TR) {
        num = LSM6DS3TR_ReadBufferedAngularPosition();
    } else if (currGyro == LSM6DS0) {
        num = LSM6DS0_ReadBufferedAngularPosition();
    }

    if (num > GYRO_BUFFER_SIZE) { // Safety measure
        num = GYRO_BUFFER_SIZE;
    }

    return num;
}

//_____________________________________________________________________________

//! \brief Same as ReadBufferedSamplesUnlocked(), taking I2CMutex internally.
//! \pre   The caller MUST NOT hold I2CMutex (the mutex is not recursive).
static uint16_t ReadBufferedSamplesLocked(void)
{
    uint16_t num;

    xSemaphoreTake(I2CMutex, portMAX_DELAY);
    num = ReadBufferedSamplesUnlocked();
    xSemaphoreGive(I2CMutex);

    return num;
}

//_____________________________________________________________________________

static void ReinitCurrentGyroLocked(void)
{
    // Recovers the sensor FIFO after an overrun or a pattern desync
    int16_t offset = Settings_GetOffsetGyroSettings();

    xSemaphoreTake(I2CMutex, portMAX_DELAY);

    if (currGyro == LSM6DS3US) {
        LSM6DS3US_InitGyroscope(offset);
    } else if (currGyro == LSM6DS3TR) {
        LSM6DS3TR_InitGyroscope(offset);
    } else if (currGyro == LSM6DS0) {
        LSM6DS0_InitGyroscope(offset);
    }

    xSemaphoreGive(I2CMutex);
}

//_____________________________________________________________________________

void Gyroscope_ReadAngle(void)
{
	uint16_t numReadSamples = 0;

	if(calibrationInProgress) {	// The buffered data are needed for gyro calibration
		return;
	}

	if(currGyro == GYRO_NOT_AVAILABLE) {
		Angle[0] = 0;
		Angle[1] = 0;
		Angle[2] = 0;
		vmVariables.angle[0] = 0;
  		vmVariables.angle[1] = 0;
  		vmVariables.angle[2] = 0;
		return;
	}

	// Called from the sensors task, which already holds I2CMutex
	numReadSamples = ReadBufferedSamplesUnlocked();

	if(continuousCalibrationEnabled) {
		if(GetLeftSpeed()==0 && GetRightSpeed()==0 && STM32_GetLeftMotorTarget()==0 && STM32_GetRightMotorTarget()==0) { // Avoid calibrating when the robot is moving, this is especially useful when robot is moving slowly
			for (uint8_t axis = 0u; axis < 3u; axis++)
			{
				for (uint16_t index = 0u; index < numReadSamples; index++)
				{
					if(abs(GyroBuffer[axis][index]) < STATIONARY_THR) {
						ZeroGyroSum[axis] += GyroBuffer[axis][index];
						ZeroGyroNumSamples[axis]++;
					}
				}
			}

			if ((ZeroGyroNumSamples[0] >= 208) && (ZeroGyroNumSamples[1] >= 208) && (ZeroGyroNumSamples[2] >= 208))	// With ODR=104 hz, then we get at least 2 seconds of data for calibration
			{
				for (uint8_t axis = 0u; axis < 3u; axis++)
				{
					if(ZeroGyro[axis] == 0) { // First time take the average of the raw values
						ZeroGyro[axis] = ZeroGyroSum[axis]/ZeroGyroNumSamples[axis];
					} else { // Then apply a low pass filter (0.5*prev + 0.5*new)
						ZeroGyro[axis] = (ZeroGyro[axis]>>1) + ((ZeroGyroSum[axis]/ZeroGyroNumSamples[axis])>>1);
					}
					ZeroGyroSum[axis] = 0;
					ZeroGyroNumSamples[axis] = 0;
					//ESP_LOGE(Tag, "Index: %d, ZeroGyro: %d", axis, ZeroGyro[axis]);
				}
			}
		}
	}

	CalculateAngle(Angle, numReadSamples);

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

int32_t Gyroscope_GetAngleZ(void)
{
  return Angle[2];
}

//_____________________________________________________________________________

//float Gyroscope_GetAngleZFloat(void)
//{
//  return AngleFloat[2];
//}

//_____________________________________________________________________________

int16_t Gyroscope_GetAngleZ_deg(void)
{
  return Angle_deg[2];
}

//_____________________________________________________________________________

void Gyroscope_ResetAngle(void)
{
  for (uint8_t i = 0u; i < 3u; i++)
  {
	Angle[i] = 0;
	Angle_deg[i] = 0;
	//AngleFloat[i] = 0.0;
  }

  vmVariables.angle[0] = 0;
  vmVariables.angle[1] = 0;
  vmVariables.angle[2] = 0;

  vmVariables.angle_deg[0] = 0;
  vmVariables.angle_deg[1] = 0;
  vmVariables.angle_deg[2] = 0;
}

//_____________________________________________________________________________

bool Gyroscope_Calibrate(void) {
	uint16_t numSamplesCalib = 0;
	uint16_t numReadSamples = 0;
	uint8_t trials = 0;

    if (currGyro == GYRO_NOT_AVAILABLE) {
        ESP_LOGE(Tag, "Manual calibration refused: gyroscope not available");
        return false;
    }

	if(continuousCalibrationEnabled) {	// Do not mix continuous calibration with manual calibration
		numSamplesCalib = 0;
		ESP_LOGW(Tag, "Manual calibration refused: continuous calibration is enabled");
		return false;
	}

    if (I2CMutex == NULL) { // Sensors_Init() did not run or the I2C bus failed to start
        ESP_LOGE(Tag, "Manual calibration refused: I2C mutex is not available");
        return false;
    }	

	ZeroGyroSum[0] = 0;
	ZeroGyroSum[1] = 0;
	ZeroGyroSum[2] = 0;

    // Raise the flag inside the mutex so that a sensors cycle already in progress
    // completes before Gyroscope_ReadAngle() starts skipping the FIFO read.
    xSemaphoreTake(I2CMutex, portMAX_DELAY);
    calibrationInProgress = true;
    xSemaphoreGive(I2CMutex);

    // Drop everything acquired before this call: the robot may still have been
    // moving, and a flash write may have left the FIFO in overrun.
    (void)ReadBufferedSamplesLocked();
    vTaskDelay(CALIB_SETTLE_MS / portTICK_PERIOD_MS);
    (void)ReadBufferedSamplesLocked();

	while(1) {

        numReadSamples = ReadBufferedSamplesLocked();
		ESP_LOGE(Tag, "numReadSamples=%d", numReadSamples);
        if (numReadSamples == 0u) {
            continue;
        }

		for (uint8_t axis = 0u; axis < 3u; axis++)
		{
			for (uint16_t index = 0u; index < numReadSamples; index++)
			{
				ZeroGyroSum[axis] += GyroBuffer[axis][index];
			}
		}

		numSamplesCalib += numReadSamples;

		if (numSamplesCalib >= CALIB_TARGET_SAMPLES)
		{
			for (uint8_t axis = 0u; axis < 3u; axis++)
			{
				ZeroGyro[axis] = ZeroGyroSum[axis]/numSamplesCalib;
				ESP_LOGE(Tag, "Index : %d, ZeroGyro: %d", axis, ZeroGyro[axis]);
			}

			numSamplesCalib = 0u;
			calibrationInProgress = false;
			return true;
		}

		vTaskDelay(CALIB_POLL_MS / portTICK_PERIOD_MS);
		trials++;
		if(trials >= CALIB_MAX_TRIALS) {
			ESP_LOGE(Tag, "Cannot calibrate gyro");
			ReinitCurrentGyroLocked(); // The FIFO is very likely stuck
			break; // We don't get 16 samples in 500 ms, it means something goes wrong so exit avoiding an infinite blocking loop.
		}
	}
	calibrationInProgress = false;
	return false;
}

//_____________________________________________________________________________

void Gyroscope_ResetCalibration(void)
{
	for (uint8_t i = 0u; i < 3u; i++) {
		ZeroGyro[i] = 0;
		ZeroGyroSum[i] = 0;
		ZeroGyroNumSamples[i] = 0;
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

//_____________________________________________________________________________

void Gyroscope_EnableContinuousCalib(void) {
	for (uint8_t i = 0u; i < 3u; i++) {
		ZeroGyroSum[i] = 0;
		ZeroGyroNumSamples[i] = 0;
  	}
	continuousCalibrationEnabled = true;
}

//_____________________________________________________________________________

void Gyroscope_DisableContinuousCalib(void) {
	continuousCalibrationEnabled = false;
}

//_____________________________________________________________________________

void Gyroscope_GetCalibration(int16_t* values){
	memcpy(values, ZeroGyro, 6);
}

//_____________________________________________________________________________

void Gyroscope_SetCalibration(int16_t* values){
	memcpy(ZeroGyro, values, 6);
}

//_____________________________________________________________________________

void Gyroscope_SaveCalibrationOffsets(void)
{
	Settings_WriteZeroOffGyro(ZeroGyro);
}
