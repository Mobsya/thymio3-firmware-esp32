//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    stm32.h
//! \brief   This module provides the useful functions to communicate with the STM32
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef STM32_SPI_H_
#define STM32_SPI_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int16_t LeftMotor;   //!< Correction factor of the left motor
  int16_t RightMotor;  //!< Correction factor of the right motor
} T_Settings;
//#endif

typedef struct
{
  int16_t Left;
  int16_t Right;
} T_Motor;  //!< Motor description

typedef struct
{
  int16_t FrontLeft;
  int16_t FrontLeftCenter;
  int16_t FrontCenter;
  int16_t FrontRightCenter;
  int16_t FrontRight;
  int16_t BackLeft;
  int16_t BackRight;
} T_ProxIR;  //!< Proximity IR sensor description

typedef struct
{
  int16_t LeftAmbiant;
  int16_t RightAmbiant;
  int16_t LeftReflected;
  int16_t RightReflected;
  int16_t LeftDelta;
  int16_t RightDelta;
} T_GroundIR;  //!< Ground IR sensor description

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the STM32
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_Init(void);

extern void STM32_SetStatus(uint16_t status);

extern uint16_t STM32_GetStatus(void);

extern void STM32_UpdateSettings(void);

extern void STM32_SetLeftMotorSettings(int16_t settings);

extern void STM32_SetRightMotorSettings(int16_t settings);

extern int16_t STM32_GetLeftMotorSettings(void);

extern int16_t STM32_GetRightMotorSettings(void);

extern void STM32_SetBatteryVoltage(int16_t voltage);

//! \brief     Get the battery voltage
//! \pre       None
//! \param     None
//! \return    The battery voltage
extern int16_t STM32_GetBatteryVoltage(void);

extern void STM32_SetLeftBatteryMotorVoltage(int16_t voltage);

extern void STM32_SetRightBatteryMotorVoltage(int16_t voltage);

extern int16_t STM32_GetBatteryMotorVoltage(void);

extern void STM32_SetLeftInducedVoltage(int16_t voltage);

extern void STM32_SetRightInducedVoltage(int16_t voltage);

extern T_Motor STM32_GetInducedVoltage(void);

extern void STM32_SetLeftMotorCurrent(int16_t current);

extern void STM32_SetRightMotorCurrent(int16_t current);

extern T_Motor STM32_GetMotorCurrent(void);

extern void STM32_SetLeftPwmDutyCycle(int16_t dutycycle);

extern void STM32_SetRightPwmDutyCycle(int16_t dutycycle);

extern void STM32_UpdateMotorTargets(void);

extern int16_t STM32_GetLeftMotorTarget(void);

extern int16_t STM32_GetRightMotorTarget(void);

extern void STM32_SetSoundLevel(int16_t level);

extern void STM32_SetSoundThreshold(int16_t threshold);

extern int16_t STM32_GetSoundThreshold(void);

extern void STM32_SetSoundMean(int16_t mean);

extern void STM32_SetProxIRValues(int16_t* buffer, uint16_t position);

//! \brief     Get the prox IR value
//! \pre       None
//! \param     None
//! \return    None
extern T_ProxIR STM32_GetProxIRValues(void);

extern void STM32_SetGroundIRValues(int16_t* buffer, uint16_t position);

#if 0
extern void STM32_SetFrontLeftProxIRValue(int16_t value);

extern void STM32_SetFrontLeftCenterProxIRValue(int16_t value);

extern void STM32_SetFrontCenterProxIRValue(int16_t value);

extern void STM32_SetFrontRightCenterProxIRValue(int16_t value);

extern void STM32_SetFrontRightProxIRValue(int16_t value);

extern void STM32_SetBackLeftProxIRValue(int16_t value);

extern void STM32_SetBackRightProxIRValue(int16_t value);
#endif
//! \brief     Check that the USB cable is present
//! \pre       None
//! \param     None
//! \return    True if the USB cable is present, false otherwise
extern bool STM32_IsUSBCablePresent(void);

//! \brief     Check that the USB port is open
//! \pre       None
//! \param     None
//! \return    True if the USB port is open, false otherwise
extern bool STM32_IsUSBPortOpen(void);

//! \brief     Check that a mode update has been requested
//! \pre       None
//! \param     None
//! \return    True if a mode update has been requested, false otherwise
extern bool STM32_IsModeUpdateRequested(void);

//! \brief     Check that the STM32 is ready to switch off
//! \pre       None
//! \param     None
//! \return    True if the STM32 is ready to switch off, false otherwise
extern bool STM32_IsReadyToSwitchOff(void);

//! \brief     Allow the STM32 to switch off (sleep mode)
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_AllowToSwitchOff(void);

//! \brief     Check that the STM32 is allowed to switch off
//! \pre       None
//! \param     None
//! \return    True if the STM32 is allowed to switch off, false otherwise
extern bool STM32_IsAllowedToSwitchOff(void);

#endif // STM32_SPI_H_
