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
#define BUTTON_MEAN_REG_ADDRESS         0x18u  //!< Button mean register address        (Read only)
#define BUTTON_NOISE_REG_ADDRESS        0x19u  //!< Button noise register address       (Read only)

#define STM32_ID                        0xBCu  //!< ID of the STM32

// Status register bit mask
#define USB_CABLE_IS_PRESENT_BIT_MASK   0x01u
#define USB_PORT_IS_OPEN_BIT_MASK       0x02u
#define MODE_UPDATE_BIT_MASK            0x04u
#define READY_TO_SWITCH_OFF_BIT_MASK    0x08u
#define OK_TO_SWITCH_OFF_BIT_MASK       0x10u
#define GPIO0_PIN_MODE_BIT_MASK         0x20u

// Status register bit position
#define USB_CABLE_IS_PRESENT_BIT_POS       0u  // This bit is set by the STM32
#define USB_PORT_IS_OPEN_BIT_POS           1u  // This bit is set by the STM32
#define MODE_UPDATE_BIT_POS                2u  // This bit is set by the STM32
#define READY_TO_SWITCH_OFF_BIT_POS        3u  // This bit is set by the STM32
#define OK_TO_SWITCH_OFF_BIT_POS           4u  // This bit is set by the ESP32
#define GPIO0_PIN_MODE_BIT_POS             5u  // This bit is set by the STM32

#define MOTORS_NUM                         2u  //!< Number of motors

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details Motor selection
typedef enum
{
  E_Motor_Left,  //!< Left motor
  E_Motor_Right  //!< Right motor
} T_Motor;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "stm32";

static uint8_t Status = 0u;

static int16_t ButtonRaw[BUTTONS_NUM]    = {0, 0, 0, 0, 0};
static int16_t ButtonMean[BUTTONS_NUM]   = {0, 0, 0, 0, 0};
static int16_t ButtonNoise[BUTTONS_NUM]  = {0, 0, 0, 0, 0};
static uint8_t ButtonStatus[BUTTONS_NUM] = {0u, 0u, 0u, 0u, 0u};

static int16_t Vbat[MOTORS_NUM]      = {0, 0};
static int16_t Vind[MOTORS_NUM]      = {0, 0};
static int16_t DutyCycle[MOTORS_NUM] = {0, 0};
static int16_t Current[MOTORS_NUM]   = {0, 0};

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

  static uint8_t oldStatus = 0x00;

  I2C_ReadFromAddress(SLAVE_ADDRESS, STATUS_REG_ADDRESS, &data, 1u);

  Status = data;

  if (oldStatus != Status)
  {
    oldStatus = Status;
  }
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

bool STM32_IsModeUpdateRequested(void)
{
  return ((Status & (1 << MODE_UPDATE_BIT_POS)) == MODE_UPDATE_BIT_MASK);
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

bool STM32_IsAllowedToSwitchOff(void)
{
  return ((Status & (1 << OK_TO_SWITCH_OFF_BIT_POS)) == OK_TO_SWITCH_OFF_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsGpio0InNormalMode(void)
{
  // 'True' means that the GPIO0 is in input (STM32) so it is possible to drive this pin for the ESP32
  return ((Status & (1 << GPIO0_PIN_MODE_BIT_POS)) == GPIO0_PIN_MODE_BIT_MASK);
}
//_____________________________________________________________________________

T_Error STM32_CheckId(void)
{
  uint8_t id = 0x00;
  T_Error err = E_Error_None;

  ReadId(&id);

  if (id != STM32_ID)
  {
    err = E_Error_STM32_InvalidID;
    ESP_LOGE(Tag, "Invalid ID: %d", id);
  }

  return err;
}

//_____________________________________________________________________________

void STM32_UpdateLeftMotorTarget(int16_t* target)
{
  uint8_t data[2];

  data[0] = (uint8_t)(target[E_Motor_Left]);
  data[1] = (uint8_t)((target[E_Motor_Left]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, 2u);
}

//_____________________________________________________________________________

void STM32_UpdateRightMotorTarget(int16_t* target)
{
  uint8_t data[2];

  data[0] = (uint8_t)(target[E_Motor_Right]);
  data[1] = (uint8_t)((target[E_Motor_Right]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, 2u);
}

//_____________________________________________________________________________

void STM32_GetLeftMotorTarget(int16_t* target)
{
  uint8_t data[2];

  I2C_ReadFromAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, 2u);

  target[E_Motor_Left] = ((data[1] << 8) | data[0]);

#if 0  // TODO
  vmVariables.target[E_Motor_Left] = target[E_Motor_Left];
#endif
}

//_____________________________________________________________________________

void STM32_GetRightMotorTarget(int16_t* target)
{
  uint8_t data[2];

  I2C_ReadFromAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, 2u);

  target[E_Motor_Right] = ((data[1] << 8) | data[0]);

#if 0  // TODO
  vmVariables.target[E_Motor_Right] = target[E_Motor_Right];
#endif
}

//_____________________________________________________________________________

void STM32_ReadPwmDutyCycle(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, PWM_DUTY_CYCLE_REG_ADDRESS, data, 4u);

  DutyCycle[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  DutyCycle[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.pwm[E_Motor_Left]  = DutyCycle[E_Motor_Left];
  vmVariables.pwm[E_Motor_Right] = DutyCycle[E_Motor_Right];
}

//_____________________________________________________________________________

void STM32_ReadBatteryVoltage(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BATTERY_VOLTAGE_REG_ADDRESS, data, 4u);

  Vbat[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  Vbat[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.vbat[E_Motor_Left]  = Vbat[E_Motor_Left];
  vmVariables.vbat[E_Motor_Right] = Vbat[E_Motor_Right];
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryVoltage(void)
{
  return (Vbat[E_Motor_Left] + Vbat[E_Motor_Right]);
}

//_____________________________________________________________________________

void STM32_ReadInducedVoltage(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, INDUCED_VOLTAGE_REG_ADDRESS, data, 4u);

  Vind[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  Vind[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.uind[E_Motor_Left]  = Vind[E_Motor_Left];
  vmVariables.uind[E_Motor_Right] = Vind[E_Motor_Right];

  SET_EVENT(EVENT_MOTOR);
}

//_____________________________________________________________________________

void STM32_ReadMotorCurrent(void)
{
  uint8_t data[4];

  I2C_ReadFromAddress(SLAVE_ADDRESS, MOTOR_CURRENT_REG_ADDRESS, data, 4u);

  Current[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  Current[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.imot[E_Motor_Left]  = Current[E_Motor_Left];
  vmVariables.imot[E_Motor_Right] = Current[E_Motor_Right];
}

//_____________________________________________________________________________

void STM32_ReadButtonStatus(void)
{
  uint8_t data;

  I2C_ReadFromAddress(SLAVE_ADDRESS, BUTTON_STATUS_REG_ADDRESS, &data, 1u);

  for (int16_t button = 0; button < BUTTONS_NUM; button++)
  {
    ButtonStatus[button] = ((data & (1 << button)) >> button);

    if (ButtonStatus[button] != vmVariables.buttons_state[button])
    {
      SET_EVENT(button);
    }

    vmVariables.buttons_state[button] = (int16_t)ButtonStatus[button];
  }

  SET_EVENT(EVENT_BUTTONS);
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

  ButtonRaw[E_Button_Backward] = ((data[1] << 8) | data[0]);
  ButtonRaw[E_Button_Left]     = ((data[3] << 8) | data[2]);
  ButtonRaw[E_Button_Center]   = ((data[5] << 8) | data[4]);
  ButtonRaw[E_Button_Forward]  = ((data[7] << 8) | data[6]);
  ButtonRaw[E_Button_Right]    = ((data[9] << 8) | data[8]);

  vmVariables.buttons[E_Button_Backward] = ButtonRaw[E_Button_Backward];
  vmVariables.buttons[E_Button_Left]     = ButtonRaw[E_Button_Left];
  vmVariables.buttons[E_Button_Center]   = ButtonRaw[E_Button_Center];
  vmVariables.buttons[E_Button_Forward]  = ButtonRaw[E_Button_Forward];
  vmVariables.buttons[E_Button_Right]    = ButtonRaw[E_Button_Right];
}

//_____________________________________________________________________________

void STM32_ReadButtonMean(void)
{
  uint8_t data[10];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BUTTON_MEAN_REG_ADDRESS, data, 10u);

  ButtonMean[E_Button_Backward] = ((data[1] << 8) | data[0]);
  ButtonMean[E_Button_Left]     = ((data[3] << 8) | data[2]);
  ButtonMean[E_Button_Center]   = ((data[5] << 8) | data[4]);
  ButtonMean[E_Button_Forward]  = ((data[7] << 8) | data[6]);
  ButtonMean[E_Button_Right]    = ((data[9] << 8) | data[8]);

  vmVariables.buttons_mean[E_Button_Backward] = ButtonMean[E_Button_Backward];
  vmVariables.buttons_mean[E_Button_Left]     = ButtonMean[E_Button_Left];
  vmVariables.buttons_mean[E_Button_Center]   = ButtonMean[E_Button_Center];
  vmVariables.buttons_mean[E_Button_Forward]  = ButtonMean[E_Button_Forward];
  vmVariables.buttons_mean[E_Button_Right]    = ButtonMean[E_Button_Right];
}

//_____________________________________________________________________________

void STM32_ReadButtonNoise(void)
{
  uint8_t data[10];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BUTTON_NOISE_REG_ADDRESS, data, 10u);

  ButtonNoise[E_Button_Backward] = ((data[1] << 8) | data[0]);
  ButtonNoise[E_Button_Left]     = ((data[3] << 8) | data[2]);
  ButtonNoise[E_Button_Center]   = ((data[5] << 8) | data[4]);
  ButtonNoise[E_Button_Forward]  = ((data[7] << 8) | data[6]);
  ButtonNoise[E_Button_Right]    = ((data[9] << 8) | data[8]);

  vmVariables.buttons_noise[E_Button_Backward] = ButtonNoise[E_Button_Backward];
  vmVariables.buttons_noise[E_Button_Left]     = ButtonNoise[E_Button_Left];
  vmVariables.buttons_noise[E_Button_Center]   = ButtonNoise[E_Button_Center];
  vmVariables.buttons_noise[E_Button_Forward]  = ButtonNoise[E_Button_Forward];
  vmVariables.buttons_noise[E_Button_Right]    = ButtonNoise[E_Button_Right];
}

//_____________________________________________________________________________

static void ReadId(uint8_t* id)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, id, 1u);
}
