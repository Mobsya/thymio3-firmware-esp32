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

#define SLAVE_ADDRESS                   0x04u  //!< Slave address

#define SETTINGS_REG_ADDRESS            0x0Du  //!< Settings register address           (Read/Write)
#define STATUS_REG_ADDRESS              0x0Eu  //!< Status register address             (Read/Write)
#define WHO_AM_I_REG_ADDRESS            0x0Fu  //!< WHO_AM_I register address           (Read only)
#define LEFT_MOTOR_TARGET_REG_ADDRESS   0x10u  //!< Left motor target register address  (Read/Write)
#define RIGHT_MOTOR_TARGET_REG_ADDRESS  0x11u  //!< Right motor target register address (Read/Write)
#define BATTERY_VOLTAGE_REG_ADDRESS     0x12u  //!< Battery voltage register address    (Read only)
#define INDUCED_VOLTAGE_REG_ADDRESS     0x13u  //!< Induced voltage register address    (Read only)
#define MOTOR_CURRENT_REG_ADDRESS       0x14u  //!< Motor current register address      (Read only)
#define PWM_DUTY_CYCLE_REG_ADDRESS      0x15u  //!< PWM duty cycle register address     (Read only)
#define BUTTON_STATUS_REG_ADDRESS       0x16u  //!< Button status register address      (Read only)
#define BUTTON_RAW_DATA_REG_ADDRESS     0x17u  //!< Button raw data register address    (Read only)

#define STM32_ID                        0xBCu  //!< ID of the STM32

// Status register bit mask
#define USB_CABLE_IS_PRESENT_BIT_MASK   0x01u
#define USB_PORT_IS_OPEN_BIT_MASK       0x02u
#define READY_TO_SWITCH_OFF_BIT_MASK    0x04u

// Status register bit position
#define USB_CABLE_IS_PRESENT_BIT_POS       0u  // This bit is set by the STM32
#define USB_PORT_IS_OPEN_BIT_POS           1u  // This bit is set by the STM32
#define READY_TO_SWITCH_OFF_BIT_POS        2u  // This bit is set by the STM32
#define OK_TO_SWITCH_OFF_BIT_POS           3u  // This bit is set by the ESP32

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

static uint8_t Status = 0u;

static int16_t ButtonRaw[BUTTON_NUM] = {0, 0, 0, 0, 0};
static uint8_t ButtonStatus[BUTTON_NUM] = {0u, 0u, 0u, 0u, 0u};

static int16_t Vbat[2] = {0, 0};
static int16_t Vind[2] = {0, 0};
static int16_t DutyCycle[2] = {0, 0};
static int16_t Current[2] = {0, 0};

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

void STM32_UpdateSettings(T_Settings settings)
{
  uint8_t data[4];

  data[0] = (uint8_t)(settings.LeftMotor);
  data[1] = (uint8_t)((settings.LeftMotor) >> 8);
  data[2] = (uint8_t)(settings.RightMotor);
  data[3] = (uint8_t)((settings.RightMotor) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, SETTINGS_REG_ADDRESS, data, 4u);
}

//_____________________________________________________________________________

void STM32_ReadStatus(void)
{
  uint8_t data;

  I2C_ReadFromAddress(SLAVE_ADDRESS, STATUS_REG_ADDRESS, &data, 1u);

  Status = data;
}

//_____________________________________________________________________________

bool STM32_IsUSBCablePresent(void)
{
  return ((Status & (1 << USB_CABLE_IS_PRESENT_BIT_POS)) == USB_CABLE_IS_PRESENT_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsUSBPortOpen(void)
{
  return ((Status & (1 << USB_PORT_IS_OPEN_BIT_POS)) == USB_PORT_IS_OPEN_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsReadyToSwitchOff(void)
{
  return ((Status & (1 << READY_TO_SWITCH_OFF_BIT_POS)) == READY_TO_SWITCH_OFF_BIT_MASK);
}

//_____________________________________________________________________________

void STM32_AllowToSwitchOff(void)
{
  uint8_t data = (Status | (1 << OK_TO_SWITCH_OFF_BIT_POS));

  I2C_WriteToAddress(SLAVE_ADDRESS, STATUS_REG_ADDRESS, &data, 1u);
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

void STM32_UpdateRightMotorTarget(int16_t* target)
{
  uint8_t data[2];

  data[0] = (uint8_t)(target[1]);
  data[1] = (uint8_t)((target[1]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, 2u);
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

void STM32_ReadPwmDutyCycle(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, PWM_DUTY_CYCLE_REG_ADDRESS, data, 4u);

  DutyCycle[0] = ((data[1] << 8) | data[0]);
  DutyCycle[1] = ((data[3] << 8) | data[2]);

  vmVariables.pwm[0] = DutyCycle[0];
  vmVariables.pwm[1] = DutyCycle[1];
}

//_____________________________________________________________________________

void STM32_ReadBatteryVoltage(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BATTERY_VOLTAGE_REG_ADDRESS, data, 4u);

  Vbat[0] = ((data[1] << 8) | data[0]);
  Vbat[1] = ((data[3] << 8) | data[2]);

  vmVariables.vbat[0] = Vbat[0];
  vmVariables.vbat[1] = Vbat[1];
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryVoltage(void)
{
  return (Vbat[0] + Vbat[1]);
}

//_____________________________________________________________________________

void STM32_ReadInducedVoltage(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, INDUCED_VOLTAGE_REG_ADDRESS, data, 4u);

  Vind[0] = ((data[1] << 8) | data[0]);
  Vind[1] = ((data[3] << 8) | data[2]);

  vmVariables.uind[0] = Vind[0];
  vmVariables.uind[1] = Vind[1];
}

//_____________________________________________________________________________

void STM32_ReadMotorCurrent(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, MOTOR_CURRENT_REG_ADDRESS, data, 4u);

  Current[0] = ((data[1] << 8) | data[0]);
  Current[1] = ((data[3] << 8) | data[2]);

  vmVariables.imot[0] = Current[0];
  vmVariables.imot[1] = Current[1];
}

//_____________________________________________________________________________

void STM32_ReadButtonStatus(void)
{
  uint8_t data;

  I2C_ReadFromAddress(SLAVE_ADDRESS, BUTTON_STATUS_REG_ADDRESS, &data, 1u);

  for (int16_t index = 0; index < 5; index++)
  {
    ButtonStatus[index] = ((data & (1 << index)) >> index);

    if (ButtonStatus[index] != vmVariables.buttons_state[index])
    {
      SET_EVENT(index);
    }

    vmVariables.buttons_state[index] = (int16_t)ButtonStatus[index];
  }
}

//_____________________________________________________________________________

uint8_t* STM32_GetButtonStatus(void)
{
  return ButtonStatus;
}

//_____________________________________________________________________________

void STM32_ReadButtonRawData(void)
{
  uint8_t data[10];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BUTTON_RAW_DATA_REG_ADDRESS, data, 10u);

  ButtonRaw[0] = ((data[1] << 8) | data[0]);
  ButtonRaw[1] = ((data[3] << 8) | data[2]);
  ButtonRaw[2] = ((data[5] << 8) | data[4]);
  ButtonRaw[3] = ((data[7] << 8) | data[6]);
  ButtonRaw[4] = ((data[9] << 8) | data[8]);

  vmVariables.buttons[0] = ButtonRaw[0];
  vmVariables.buttons[1] = ButtonRaw[1];
  vmVariables.buttons[2] = ButtonRaw[2];
  vmVariables.buttons[3] = ButtonRaw[3];
  vmVariables.buttons[4] = ButtonRaw[4];
}

//_____________________________________________________________________________

static void ReadId(uint8_t* id)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, id, 1u);
}
