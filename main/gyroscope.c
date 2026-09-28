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
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

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
#include "angle_controller.h"
#include "accelerometer.h"   // for the raw acceleration used to detect the hardware version

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define STATIONARY_THR 650 // Threshold used for calibration: when value is lower than the threshold, then it means the robot is still and value can be used for calibration.

#define CALIB_TARGET_SAMPLES   52 // half a second of data @ 104 Hz ODR
#define CALIB_POLL_MS          100u // @ 104 Hz ODR we should get about 10 samples per poll
#define CALIB_MAX_TRIALS       20u  // 20 * 100 ms = 2 s upper bound
#define CALIB_SETTLE_MS        300u // Settling time before starting the acquisition

#define HW_DETECT_SAMPLES        5u    // Number of accelerometer samples averaged to detect the hardware version
#define HW_DETECT_SAMPLE_MS     10u    // Delay between two accelerometer samples
#define HW_DETECT_MIN_ABS_Z   8000     // About 0.5 g with a 2 g full scale: below this value the robot is not flat enough to be trusted

#define CONT_CALIB_WINDOW_SAMPLES   208u // 2 s of contiguous still data @ 104 Hz ODR
#define CONT_CALIB_SETTLE_SAMPLES    31u // About 300 ms discarded each time the robot becomes still
#define CONT_CALIB_MAX_GYRO_P2P     200  // Max gyroscope raw peak-to-peak per axis within a window (tune on data logged at rest)

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
bool continuousCalibrationEnabled = true;

// Continuous calibration state: accessed only by the sensors task, except
// ContCalibRestartPending which is the handshake with the other tasks.
static portMUX_TYPE ContCalibMux = portMUX_INITIALIZER_UNLOCKED;
static bool ContCalibRestartPending = true;   // Protected by ContCalibMux

static int32_t ContCalibGyroSum[3];
static int16_t ContCalibGyroMin[3];
static int16_t ContCalibGyroMax[3];
static uint16_t ContCalibNumGyroSamples = 0u;

static uint16_t ContCalibSettleSamples = 0u;

static float ContCalibBias[3];                // Filtered bias, kept in float to avoid rounding errors
static bool ContCalibBiasValid = false;

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

	if(InvertYZ)	// Hardware version 0x0D and newer have the Y and Z axes inverted
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

//! \brief Discard the current continuous calibration window.
//! \param restartSettle true when the robot was (or may have been) moving.
static void ContCalibResetWindow(bool restartSettle)
{
    for (uint8_t axis = 0u; axis < 3u; axis++)
    {
        ContCalibGyroSum[axis] = 0;
        ContCalibGyroMin[axis] = INT16_MAX;
        ContCalibGyroMax[axis] = INT16_MIN;
    }

    ContCalibNumGyroSamples = 0u;

    if (restartSettle)
    {
        ContCalibSettleSamples = 0u;
    }
}

//_____________________________________________________________________________

//! \brief Feed one FIFO batch (already in GyroBuffer) to the continuous calibration.
//!        A window is made of contiguous batches in which the motors are stopped and
//!        every gyroscope sample is below STATIONARY_THR; any other batch discards the
//!        window. A complete window is also rejected if its peak-to-peak spread is
//!        too large (robot held or carried with the motors stopped).
//! \pre   Must be called only from the sensors task.
static void ContinuousCalibrationUpdate(uint16_t numSamples)
{
    bool restart = false;
    bool isStill = true;

    // Handshake with Enable/Reset/SetCalibration, which run in other tasks
    portENTER_CRITICAL(&ContCalibMux);
    restart = ContCalibRestartPending;
    ContCalibRestartPending = false;
    portEXIT_CRITICAL(&ContCalibMux);

    if (restart)
    {
        ContCalibResetWindow(true);
        ContCalibBiasValid = false;   // The next complete window replaces the bias
    }

    if (numSamples == 0u)
    {
        return;
    }

    // Motors must be stopped, both measured speed and target: this is especially
    // useful when the robot is moving slowly
    if ((GetLeftSpeed() != 0) || (GetRightSpeed() != 0) ||
        (STM32_GetLeftMotorTarget() != 0) || (STM32_GetRightMotorTarget() != 0))
    {
        ContCalibResetWindow(true);
        return;
    }

    // Every sample of every axis of the batch must be below the threshold,
    // otherwise the whole window is discarded (no truncated average)
    for (uint16_t index = 0u; (index < numSamples) && isStill; index++)
    {
        for (uint8_t axis = 0u; axis < 3u; axis++)
        {
            if (abs(GyroBuffer[axis][index]) >= STATIONARY_THR)
            {
                isStill = false;
                break;
            }
        }
    }

    if (!isStill)
    {
        ContCalibResetWindow(true);
        return;
    }

    // Skip the first samples after the robot becomes still (residual oscillations)
    if (ContCalibSettleSamples < CONT_CALIB_SETTLE_SAMPLES)
    {
        ContCalibSettleSamples += numSamples;
        return;
    }

    // Accumulate the gyroscope batch and track its min/max for the peak-to-peak check
    for (uint8_t axis = 0u; axis < 3u; axis++)
    {
        for (uint16_t index = 0u; index < numSamples; index++)
        {
            int16_t value = GyroBuffer[axis][index];

            ContCalibGyroSum[axis] += value;

            if (value < ContCalibGyroMin[axis])
            {
                ContCalibGyroMin[axis] = value;
            }
            if (value > ContCalibGyroMax[axis])
            {
                ContCalibGyroMax[axis] = value;
            }
        }
    }
    ContCalibNumGyroSamples += numSamples;

    if (ContCalibNumGyroSamples < CONT_CALIB_WINDOW_SAMPLES)
    {
        return;
    }

    // Window complete: reject it if the peak-to-peak spread is too large
    // (robot held in hand or carried with the motors stopped)
    for (uint8_t axis = 0u; axis < 3u; axis++)
    {
        if ((ContCalibGyroMax[axis] - ContCalibGyroMin[axis]) > CONT_CALIB_MAX_GYRO_P2P)
        {
            isStill = false;
        }
    }

    if (!isStill)
    {
        ContCalibResetWindow(true);
        return;
    }

    // Update the bias: the first window after a restart replaces it, then low pass filter
    for (uint8_t axis = 0u; axis < 3u; axis++)
    {
        float average = (float)ContCalibGyroSum[axis] / (float)ContCalibNumGyroSamples;

        if (!ContCalibBiasValid)
        {
            ContCalibBias[axis] = average;
        }
        else
        {
            ContCalibBias[axis] = (0.5f * ContCalibBias[axis]) + (0.5f * average);
        }

        ZeroGyro[axis] = (int16_t)lroundf(ContCalibBias[axis]);
        //ESP_LOGI(Tag, "Index: %d, ZeroGyro: %d", axis, ZeroGyro[axis]);
    }

    ContCalibBiasValid = true;
    ContCalibResetWindow(false);   // The robot is still: no new settling needed
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
		ContinuousCalibrationUpdate(numReadSamples);
	}

	CalculateAngle(Angle, numReadSamples);

  for (uint8_t index = 0u; index < 3u; index++)
  {
    vmVariables.angle[index] = Angle[index];

    Angle_deg[index] = ((Angle[index] * 90) / rotation_angle_90_);

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

//! \brief Detect the hardware version from the accelerometer Z axis and store it in the settings.
//!        On hardware version up to 0x0C the raw Z axis reads about +16384 (1 g) when the robot is flat on
//!        its wheels, while from hardware version 0x0D the Y and Z axes are inverted, thus the raw Z
//!        axis reads about -16384. The stored version is left untouched when the reading cannot be
//!        trusted (robot tilted, held or moving) and the flash is written only when the version changes.
//! \pre   The robot MUST be still and in its normal position.
//! \pre   The caller MUST NOT hold I2CMutex (the mutex is not recursive).
static void DetectHardwareVersion(void)
{
    T_Axis acceleration;
    bool isRead = false;
    int32_t sum = 0;
    int32_t average = 0;
    uint8_t version = 0u;

    for (uint8_t index = 0u; index < HW_DETECT_SAMPLES; index++)
    {
        xSemaphoreTake(I2CMutex, portMAX_DELAY);
        isRead = Accelerometer_ReadRawAcceleration(&acceleration);
        xSemaphoreGive(I2CMutex);

        if (!isRead)
        {
            ESP_LOGW(Tag, "Hardware version detection skipped: accelerometer is not available");
            return;
        }

        sum += acceleration.Z;

        vTaskDelay(HW_DETECT_SAMPLE_MS / portTICK_PERIOD_MS);
    }

    average = sum / (int32_t)HW_DETECT_SAMPLES;

    if (abs(average) < HW_DETECT_MIN_ABS_Z)
    {
        // The robot is tilted, held or moving: the reading cannot be used to detect the hardware version
        ESP_LOGW(Tag, "Hardware version detection skipped: acc Z = %d", (int)average);
        return;
    }

    version = (average < 0) ? HARDWARE_VERSION_0D : HARDWARE_VERSION_0C;

    if (version != Settings_GetHardwareVersionSettings())
    {
        Settings_SetHardwareVersionSettings(version);   // Also updates the InvertYZ flag
        Settings_WriteHardwareVersion(version);         // Write to flash only when the version changes
        ESP_LOGI(Tag, "Hardware version updated to 0x%02X (acc Z = %d)", version, (int)average);
    }
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

	// We know that the robot is still and in its correct position, thus we can check the Z axis value
	// of the accelerometer to distinguish the hardware version. This is done before acquiring the
	// calibration samples because the sign of the Y and Z gyroscope axes depends on the hardware version.
	DetectHardwareVersion();

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

    portENTER_CRITICAL(&ContCalibMux);
    ContCalibRestartPending = true;
    portEXIT_CRITICAL(&ContCalibMux);
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
    // The window state is owned by the sensors task: only request a restart here,
    // so that a manual calibration or a sensors cycle in progress is never corrupted.
    portENTER_CRITICAL(&ContCalibMux);
    ContCalibRestartPending = true;
    continuousCalibrationEnabled = true;
    portEXIT_CRITICAL(&ContCalibMux);
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

    portENTER_CRITICAL(&ContCalibMux);
    ContCalibRestartPending = true;
    portEXIT_CRITICAL(&ContCalibMux);
}

//_____________________________________________________________________________

void Gyroscope_SaveCalibrationOffsets(void)
{
	Settings_WriteZeroOffGyro(ZeroGyro);
}
