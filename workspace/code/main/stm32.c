//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    stm32.c
//! \brief   This module provides the useful functions to communicate with the STM32
//!
//! \author  Vincent Gonet
//!
//! \version $Id: stm32.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "stm32.h"

#include "i2c.h"

#include "aseba_esp32.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SLAVE_ADDRESS          0x04u  //!< Slave address

#define WHO_AM_I_REG_ADDRESS               0x0Fu  //!< Who_AM_I register address           (Read only)
#define LEFT_MOTOR_TARGET_REG_ADDRESS      0x10u  //!< Left Motor target register address  (Write only)
#define RIGHT_MOTOR_TARGET_REG_ADDRESS     0x11u  //!< Right Motor target register address (Write only)
#define BATTERY_VOLTAGE_REG_ADDRESS        0x12u  //!< Battery voltage register address    (Read only)
#define INDUCED_VOLTAGE_REG_ADDRESS        0x13u  //!< Induced voltage register address    (Read only)
#define MOTOR_CURRENT_REG_ADDRESS          0x14u  //!< Motor current register address      (Read only)
#define BUTTON_REG_ADDRESS                 0x15u

#define STM32_ID               0xBCu  //!< ID of the STM32

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "stm32";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Read the ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\stm32\ReadId.svg
static void ReadId(uint8_t* id);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void STM32_Init(void)
{

}

//_____________________________________________________________________________

void STM32_CheckId(void)
{
  uint8_t id = 0x00;

  ReadId(&id);

  if (id != STM32_ID)
  {
    ESP_LOGE(Tag, "Invalid ID: %d", id);
  }
}

//_____________________________________________________________________________

void STM32_UpdateLeftMotorTarget(int16_t* target)
{
  uint8_t data[2];

  data[0] = (uint8_t)(target[0]);
  data[1] = (uint8_t)((target[0]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, 2u);
}

//_____________________________________________________________________________

void STM32_GetLeftMotorTarget(int16_t* target)
{
  uint8_t data[2];

  I2C_ReadFromAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, 2u);

  target[0] = ((data[1] << 8) | data[0]);

#if 0  // TODO
    vmVariables.target[0] = target[0];
#endif
}

//_____________________________________________________________________________

void STM32_UpdateRightMotorTarget(int16_t* target)
{
  uint8_t data[2];

  data[0] = (uint8_t)(target[1]);
  data[1] = (uint8_t)((target[1]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, 2u);
}

//_____________________________________________________________________________

void STM32_GetRightMotorTarget(int16_t* target)
{
  uint8_t data[2];

  I2C_ReadFromAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, 2u);

  target[1] = ((data[1] << 8) | data[0]);

#if 0  // TODO
  vmVariables.target[1] = target[1];
#endif
}

//_____________________________________________________________________________

void STM32_GetBatteryVoltage(int16_t* voltage)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BATTERY_VOLTAGE_REG_ADDRESS, data, 4u);

  voltage[0] = ((data[1] << 8) | data[0]);
  voltage[1] = ((data[3] << 8) | data[2]);

  vmVariables.vbat[0] = voltage[0];
  vmVariables.vbat[1] = voltage[1];
}

//_____________________________________________________________________________

void STM32_GetInducedVoltage(int16_t* voltage)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, INDUCED_VOLTAGE_REG_ADDRESS, data, 4u);

  voltage[0] = ((data[1] << 8) | data[0]);
  voltage[1] = ((data[3] << 8) | data[2]);

  vmVariables.uind[0] = voltage[0];
  vmVariables.uind[1] = voltage[1];
}

//_____________________________________________________________________________

void STM32_GetMotorCurrent(int16_t* current)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, MOTOR_CURRENT_REG_ADDRESS, data, 4u);

  current[0] = ((data[1] << 8) | data[0]);
  current[1] = ((data[3] << 8) | data[2]);

  vmVariables.imot[0] = current[0];
  vmVariables.imot[1] = current[1];
}

//_____________________________________________________________________________

static void ReadId(uint8_t* id)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, id, 1u);
}
