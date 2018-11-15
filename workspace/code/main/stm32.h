//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: stm32.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef STM32_H_
#define STM32_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

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
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the xxx
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_Init(void);

//! \brief     Check the ID of the STM32
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_CheckId(void);

//! \brief     Update the left motor target
//! \pre       None
//! \param     target The target applied to the left motor
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateLeftMotorTarget.svg
extern void STM32_UpdateLeftMotorTarget(int16_t* target);

//! \brief     Get the left motor target
//! \pre       None
//! \param     target The target of the motors
//! \return    None
extern void STM32_GetLeftMotorTarget(int16_t* target);

//! \brief     Update the right motor target
//! \pre       None
//! \param     target The target applied to the right motor
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateRightMotorTarget.svg
extern void STM32_UpdateRightMotorTarget(int16_t* target);

//! \brief     Get the right motor target
//! \pre       None
//! \param     target The target of the motors
//! \return    None
extern void STM32_GetRightMotorTarget(int16_t* target);

//! \brief     Get the battery voltage
//! \pre       None
//! \param     voltage The battery voltage read by the ADC
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\GetBattery.svg
extern void STM32_GetBatteryVoltage(int16_t* voltage);

//! \brief     Get the induced voltage
//! \pre       None
//! \param     voltage The induced voltage read by the ADC
//! \return    None
extern void STM32_GetInducedVoltage(int16_t* voltage);

//! \brief     Get the motor current
//! \pre       None
//! \param     current The motor current calculated by the motor controller
//! \return    None
extern void STM32_GetMotorCurrent(int16_t* current);

#endif // STM32_H_
