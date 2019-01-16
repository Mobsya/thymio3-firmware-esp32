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

#include <stdbool.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUTTON_NUM   5u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int16_t LeftMotor;   //!< Correction factor of the left motor
  int16_t RightMotor;  //!< Correction factor of the right motor
} T_Settings;

typedef enum
{
  E_Button_Backward,  // Button 1
  E_Button_Left,      // Button 2
  E_Button_Center,    // Button 3
  E_Button_Forward,   // Button 4
  E_Button_Right      // Button 5
} T_Button;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the settings
//! \pre       None
//! \param     settings The settings applied to the motors
//! \return    None
extern void STM32_UpdateSettings(T_Settings settings);

//! \brief     Read the status
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadStatus(void);

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

//! \brief     Update the right motor target
//! \pre       None
//! \param     target The target applied to the right motor
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateRightMotorTarget.svg
extern void STM32_UpdateRightMotorTarget(int16_t* target);

//! \brief     Get the left motor target
//! \pre       None
//! \param     target The target of the motors
//! \return    None
extern void STM32_GetLeftMotorTarget(int16_t* target);

//! \brief     Get the right motor target
//! \pre       None
//! \param     target The target of the motors
//! \return    None
extern void STM32_GetRightMotorTarget(int16_t* target);

//! \brief     Get the left PWM duty cycle
//! \pre       None
//! \param     dutyCycle The dutyCycle of the left motor
//! \return    None
extern void STM32_GetPwmDutyCycle(int16_t* dutyCycle);

//! \brief     Read the battery voltage
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\GetBattery.svg
extern void STM32_ReadBatteryVoltage(void);

//! \brief     Get the battery voltage
//! \pre       None
//! \param     None
//! \return    None
extern int16_t STM32_GetBatteryVoltage(void);

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

//! \brief     Read the button status
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadButtonStatus(void);

//! \brief     Get the button status
//! \pre       None
//! \param     None
//! \return    None
extern uint8_t* STM32_GetButtonStatus(void);

//! \brief     Get the button raw data
//! \pre       None
//! \param     rawData The raw data of the buttons
//! \return    None
extern void STM32_GetButtonRawData(int16_t* rawData);

#endif // STM32_H_
