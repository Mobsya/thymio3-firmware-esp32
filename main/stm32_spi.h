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
//! \author  Vincent Gonet, Stefano Morgani
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
#define CLAP_THR 400

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

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

extern void STM32_SetBatteryVoltage(int16_t voltage);

//! \brief     Get the battery voltage
//! \pre       None
//! \param     None
//! \return    The battery voltage
extern int16_t STM32_GetBatteryVoltage(void);

extern void STM32_SetBatteryMotorVoltages(int16_t* buffer, uint16_t position);

extern int16_t STM32_GetBatteryMotorVoltage(void);

extern void STM32_SetInducedVoltages(int16_t* buffer, uint16_t position);

extern T_Motor STM32_GetInducedVoltage(void);

extern void STM32_SetMotorCurrents(int16_t* buffer, uint16_t position);

extern T_Motor STM32_GetMotorCurrent(void);

extern void STM32_SetPwmDutyCycles(int16_t* buffer, uint16_t position);

extern void STM32_UpdateMotorTargets(void);

extern int16_t STM32_GetLeftMotorTarget(void);

extern int16_t STM32_GetRightMotorTarget(void);

extern void STM32_SetMicrophoneIntensity(int16_t intensity);

int16_t STM32_GetMicrophoneIntensity(void);

extern void STM32_SetMicrophoneThreshold(int16_t threshold);

extern int16_t STM32_GetMicrophoneThreshold(void);

extern void STM32_SetMicrophoneMean(int16_t mean);

extern void STM32_SetProxIRValues(int16_t* buffer, uint16_t position);

//! \brief     Get the prox IR value
//! \pre       None
//! \param     None
//! \return    None
extern T_ProxIR STM32_GetProxIRValues(void);

extern void STM32_SetGroundIRValues(int16_t* buffer, uint16_t position);

extern void STM32_SetProxIRData(int16_t* buffer, uint16_t position);

extern int16_t STM32_GetProxIRTxData(void);

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

//! \brief     Check that the standby mode has been requested
//! \pre       None
//! \param     None
//! \return    True if the standby mode has been requested, false otherwise
extern bool STM32_IsStandbyRequested(void);

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

//! \brief     Get last proximity values available.
//! \pre       None
//! \param     destination buffer
//! \return    None
extern void GetProximityValues(int16_t* buffer);

//! \brief     Get last proximity value available.
//! \pre       None
//! \param     proximity id (0..6)
//! \return    proximity value (the higher the value the closer the object)
extern uint16_t GetProximityValue(uint8_t prox_id);

//! \brief     Get last ground values available.
//! \pre       None
//! \param     destination buffer
//! \return    None
extern void GetGroundValues(int16_t* buffer);

//! \brief     Get last ground value available.
//! \pre       None
//! \param     proximity id (0..1)
//! \return    proximity value (the lower the value, the darker the object)
extern uint16_t GetGroundValue(uint8_t ground_id);

//! \brief     Get last ground ambient value available.
//! \pre       None
//! \param     proximity id (0..1)
//! \return    proximity ambient value (the higher the value, the brighter the ambient light)
extern uint16_t GetGroundAmbient(uint8_t ground_id);

//! \brief     Get last ground reflected value available.
//! \pre       None
//! \param     proximity id (0..1)
//! \return    proximity reflected value (the lower the value, the darker the object)
extern uint16_t GetGroundReflected(uint8_t ground_id);

//! \brief     Set desired motors speed.
//! \pre       None
//! \param     left speed, right speed
//! \return    None
void SetMotorTargets(int16_t left, int16_t right);

//! \brief     Get measured left speed.
//! \pre       None
//! \param     None
//! \return    Left speed
int16_t GetLeftSpeed(void);

//! \brief     Get measured right speed.
//! \pre       None
//! \param     None
//! \return    Right speed
int16_t GetRightSpeed(void);

//! \brief     Get left PWM duty cycle.
//! \pre       None
//! \param     None
//! \return    Left PWM
int16_t STM32_GetLeftMotorPwm(void);

//! \brief     Get right PWM duty cycle.
//! \pre       None
//! \param     None
//! \return    Right PWM
int16_t STM32_GetRightMotorPwm(void);


#endif // STM32_SPI_H_
