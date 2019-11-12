//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    gpio.h
//! \brief   This module provides the useful functions to use the GPIO
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef GPIO_H_
#define GPIO_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details The mode of the GPIO
typedef enum
{
  E_GpioMode_Input,
  E_GpioMode_Output,
  E_GpioMode_OutputOpenDrain,
  E_GpioMode_InputOutput,
  E_GpioMode_InputOutputOpenDrain
} T_GpioMode;

//! \details The resistor of the GPIO
typedef enum
{
  E_GpioResistor_None,
  E_GpioResistor_PullUp,
  E_GpioResistor_PullDown
} T_GpioResistor;

//! \details The level of the GPIO
typedef enum
{
  E_GpioLevel_Low,
  E_GpioLevel_High
} T_GpioLevel;

//! \details The interrupt of the GPIO
typedef enum
{
  E_GpioInterrupt_Disable,
  E_GpioInterrupt_RisingEdge,
  E_GpioInterrupt_FallingEdge,
  E_GpioInterrupt_AnyEdge,
  E_GpioInterrupt_LowLevel,
  E_GpioInterrupt_HighLevel
} T_GpioInterrupt;

//! \details GPIO pin configuration
typedef struct
{
  uint16_t        pinNumber;  //!< Pin number
  T_GpioMode      mode;       //!< Pin mode
  T_GpioResistor  resistor;   //!< Pin resistor
  T_GpioLevel     level;      //!< Pin output level
  T_GpioInterrupt interrupt;  //!< Interrupt
} T_GpioPinConfig;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the GPIO
//! \pre       None
//! \param     None
//! \return    None
extern void Gpio_Init(void);

//! \brief     Configure a GPIO.
//! \pre       First initialize the GPIO
//! \param     config - Configuration of the pin
//! \return    None
extern void Gpio_ConfigurePin(const T_GpioPinConfig* config);

//! \brief     Set the pin level of a GPIO
//! \pre       First configure the GPIO
//! \param     pinNumber - Pin number
//! \param     level - Logical level (low or high) applied to the pin
//! \return    None
extern void Gpio_SetPinLevel(uint16_t pinNumber, T_GpioLevel level);

//! \brief     Toggle the pin level of a GPIO
//! \pre       First configure the GPIO
//! \param     pinNumber - Pin number
//! \return    None
extern void Gpio_TogglePinLevel(uint16_t pinNumber);

//! \brief     Get the pin level of a GPIO
//! \pre       First configure the GPIO
//! \param     pinNumber - Pin number
//! \return    Logical level of a GPIO
extern T_GpioLevel Gpio_GetPinLevel(uint16_t pinNumber);

extern bool Gpio_IsFreeFallDetected(void);

extern void Gpio_ClearFreeFallStatus(void);

extern bool Gpio_IsTapDetected(void);

extern void Gpio_ClearTapStatus(void);

#endif // GPIO_H_
