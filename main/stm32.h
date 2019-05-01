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

//! \brief     Update the motor left target
//! \pre       None
//! \param     target The target applied to the motor left
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateMotorLeftTarget.svg
extern void STM32_UpdateMotorLeftTarget(int16_t* target);

//! \brief     Update the motor right target
//! \pre       None
//! \param     target The target applied to the motor right
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\UpdateMotorRightTarget.svg
extern void STM32_UpdateMotorRightTarget(int16_t* target);

#endif // STM32_H_
