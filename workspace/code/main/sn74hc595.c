//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sn74hc595.c
//! \brief   This module provides the useful functions to use the SN74HC595
//!          8-Bit shift registers with 3-state output registers
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sn74hc595.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <esp_log.h>

#include "sn74hc595.h"

#include "board.h"
#if 0
#include "gpio.h"  // Not used in this project
#endif
#include "spi.h"

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

static const char* Tag = "sn74hc595";

#if 0  // Not used in this project
// Declaration of non-SPI pin
static const T_GpioPinConfig PinConfig[MAX_SN74HC595_PIN] =
{
// PinNumber     Mode               Resistor             Level            Interrupt
  {LED_OE_PIN,   E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {LED_RCLK_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable}
};
#endif

static spi_device_handle_t ShiftRegisters;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void SN74HC595_Init(void)
{
#if 0  // Not used in this project
  uint8_t index;

  for (index = 0u; index < MAX_SN74HC595_PIN; index++)
  {
    Gpio_ConfigurePin(&PinConfig[index]);
  }
#endif

  Spi_Init();
  Spi_AddDevice(&ShiftRegisters, LED_CS_PIN);

  //ESP_LOGI(Tag, "SN74HC595 are initialized");
}

//_____________________________________________________________________________

void SN74HC595_DisableOutputs(void)
{
  // OE = High
  // In Thymio, the OE is controlled through the VA_ENABLE_PIN
  //Gpio_SetPinLevel(LED_OE_PIN, E_GpioLevel_High);
}

//_____________________________________________________________________________

void SN74HC595_EnableOutputs(void)
{
  // OE = Low
  // In Thymio, the OE is controlled through the VA_ENABLE_PIN
  //Gpio_SetPinLevel(LED_OE_PIN, E_GpioLevel_Low);
}

//_____________________________________________________________________________

void SN74HC595_ClearShiftRegister(void)
{
  // /SRCLR = Low (not available)
}

//_____________________________________________________________________________

void SN74HC595_Fill(uint8_t* data, uint16_t size)
{
  // Just to try
  //Gpio_SetPinLevel(LED_RCLK_PIN, E_GpioLevel_Low);
  //ESP_LOGI(Tag, "data = %d, size = %d", *data, size);

  // SRCLK = Rising edge, /SRCLR = High
  Spi_Write(ShiftRegisters, data, size);
}

//_____________________________________________________________________________

void SN74HC595_StoreShiftRegisterData(void)
{
  // RCLK = Rising edge
  //Gpio_SetPinLevel(LED_RCLK_PIN, E_GpioLevel_High);
}
