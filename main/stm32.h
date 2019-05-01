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

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUTTONS_NUM   5u  //!< Number of buttons

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------
//#if 0
typedef struct
{
  int16_t LeftMotor;   //!< Correction factor of the left motor
  int16_t RightMotor;  //!< Correction factor of the right motor
} T_Settings;
//#endif

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
//! \param     settings - Settings applied to the motors
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

//! \brief     Check the ID of the STM32
//! \pre       None
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_STM32_InvalidID
extern T_Error STM32_CheckId(void);

//! \brief     Update the left motor target
//! \pre       None
//! \param     target - Target applied to the left motor
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateLeftMotorTarget.svg
extern void STM32_UpdateLeftMotorTarget(int16_t* target);

//! \brief     Update the right motor target
//! \pre       None
//! \param     target - Target applied to the right motor
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateRightMotorTarget.svg
extern void STM32_UpdateRightMotorTarget(int16_t* target);

//! \brief     Get the left motor target
//! \pre       None
//! \param     target - Target of the motors
//! \return    None
extern void STM32_GetLeftMotorTarget(int16_t* target);

//! \brief     Get the right motor target
//! \pre       None
//! \param     target - Target of the motors
//! \return    None
extern void STM32_GetRightMotorTarget(int16_t* target);

//! \brief     Read the PWM duty cycle
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadPwmDutyCycle(void);

//! \brief     Read the battery voltage
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\GetBattery.svg
extern void STM32_ReadBatteryVoltage(void);

//! \brief     Get the battery voltage
//! \pre       None
//! \param     None
//! \return    Battery voltage
extern int16_t STM32_GetBatteryVoltage(void);

//! \brief     Read the induced voltage
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadInducedVoltage(void);

//! \brief     Read the motor current
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadMotorCurrent(void);

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

//! \brief     Read the button raw data
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadButtonRawData(void);

//! \brief     Read the button mean
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadButtonMean(void);

//! \brief     Read the button noise
//! \pre       None
//! \param     None
//! \return    None
extern void STM32_ReadButtonNoise(void);

#endif // STM32_H_
