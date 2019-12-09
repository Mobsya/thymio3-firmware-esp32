//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    stm32.c
//! \brief   This module provides the useful functions to communicate with the STM32 by I2C
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "stm32_spi.h"

#include "aseba_esp32.h"
#include "board.h"
#include "spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SLAVE_ADDRESS                       0x04u  //!< Slave address

#define STM32_ID                            0xBCu  //!< ID of the STM32

// Status register bit mask
#define USB_CABLE_IS_PRESENT_BIT_MASK       0x01u
#define USB_PORT_IS_OPEN_BIT_MASK           0x02u
#define MODE_UPDATE_BIT_MASK                0x04u
#define READY_TO_SWITCH_OFF_BIT_MASK        0x08u
#define OK_TO_SWITCH_OFF_BIT_MASK           0x10u

// Status register bit position
#define USB_CABLE_IS_PRESENT_BIT_POS       0u  // This bit is set by the STM32
#define USB_PORT_IS_OPEN_BIT_POS           1u  // This bit is set by the STM32
#define MODE_UPDATE_BIT_POS                2u  // This bit is set by the STM32
#define READY_TO_SWITCH_OFF_BIT_POS        3u  // This bit is set by the STM32
#define OK_TO_SWITCH_OFF_BIT_POS           4u  // This bit is set by the ESP32

#define SETTINGS_MESSAGE_LENGTH                4u  //!< Settings message length in bytes
#define STATUS_MESSAGE_LENGTH                  1u  //!< Status message length in bytes
#define WHO_AM_I_MESSAGE_LENGTH                1u  //!< WHO_AM_I message length in bytes
#define LEFT_MOTOR_TARGET_MESSAGE_LENGTH       2u  //!< Left motor target message length in bytes
#define RIGHT_MOTOR_TARGET_MESSAGE_LENGTH      2u  //!< Right motor target message length in bytes
#define BATTERY_MOTOR_VOLTAGE_MESSAGE_LENGTH   4u  //!< Battery motor voltage message length in bytes
#define INDUCED_VOLTAGE_MESSAGE_LENGTH         4u  //!< Induced voltage message length in bytes
#define MOTOR_CURRENT_MESSAGE_LENGTH           4u  //!< Motor current message length in bytes
#define PWM_DUTY_CYCLE_MESSAGE_LENGTH          4u  //!< PWM duty cycle message length in bytes
#define BATTERY_VOLTAGE_MESSAGE_LENGTH         2u  //!< Battery voltage message length in bytes
#define PROX_IR_VALUE_MESSAGE_LENGTH          14u  //!< Prox IR value message length in bytes
#define GROUND_IR_VALUE_MESSAGE_LENGTH        12u  //!< Ground IR value message length in bytes
#define GROUND_IR_LEDS_MESSAGE_LENGTH          4u  //!< Ground IR LEDs message length in bytes
#define SOUND_MESSAGE_LENGTH                   6u  //!< Sound message length in bytes
#define BEHAVIOR_STATUS_MESSAGE_LENGTH         2u  //!< Behavior status message length in bytes

#define PROX_IR_SENSORS_NUM      7u  //!< Number of proximity IR sensors

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details Motor selection
enum
{
  E_Motor_Left,  //!< Left motor
  E_Motor_Right  //!< Right motor
};

enum
{
  E_ProxIR_FrontLeft,
  E_ProxIR_FrontLeftCenter,
  E_ProxIR_FrontCenter,
  E_ProxIR_FrontRightCenter,
  E_ProxIR_FrontRight,
  E_ProxIR_BackLeft,
  E_ProxIR_BackRight
};

enum
{
  E_GroundIR_Left,
  E_GroundIR_Right
};

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "stm32_spi";

static uint16_t Status = 0u;

static T_Motor Vind;
static T_Motor VbatMotor;
static T_Motor DutyCycle;
static T_Motor Current;
static T_Motor Target;

static T_ProxIR ProxIR;
//static int16_t ProxIR[PROX_IR_SENSORS_NUM] = {0, 0, 0, 0, 0, 0, 0};

static T_GroundIR GroundIR;

static T_Settings Settings;

static int16_t Vbat = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void STM32_Init(void)
{
  Vind.Left = 0;
  Vind.Right = 0;

  VbatMotor.Left = 0;
  VbatMotor.Right = 0;

  DutyCycle.Left = 0;
  DutyCycle.Right = 0;

  Current.Left = 0;
  Current.Right = 0;

  Settings.LeftMotor  = 256;
  Settings.RightMotor = 256;
}

//_____________________________________________________________________________

void STM32_SetStatus(uint16_t status)
{
  Status = status;
}

//_____________________________________________________________________________

uint16_t STM32_GetStatus(void)
{
  return Status;
}

//_____________________________________________________________________________

void STM32_UpdateSettings(void)
{
  Settings.LeftMotor  = vmVariables.settings[0];
  Settings.RightMotor = vmVariables.settings[1];
}

//_____________________________________________________________________________

void STM32_SetLeftMotorSettings(int16_t settings)
{
  Settings.LeftMotor = settings;
}

//_____________________________________________________________________________

void STM32_SetRightMotorSettings(int16_t settings)
{
  Settings.RightMotor = settings;
}

//_____________________________________________________________________________

int16_t STM32_GetLeftMotorSettings(void)
{
  return Settings.LeftMotor;
}

//_____________________________________________________________________________

int16_t STM32_GetRightMotorSettings(void)
{
  return Settings.RightMotor;
}

//_____________________________________________________________________________

void STM32_SetBatteryVoltage(int16_t voltage)
{
  Vbat = voltage;
  vmVariables.vbat = Vbat;
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryVoltage(void)
{
  return Vbat;
}

//_____________________________________________________________________________

void STM32_SetLeftBatteryMotorVoltage(int16_t voltage)
{
  VbatMotor.Left = voltage;
  vmVariables.vbat_motor[E_Motor_Left] = VbatMotor.Left;
}

//_____________________________________________________________________________

void STM32_SetRightBatteryMotorVoltage(int16_t voltage)
{
  VbatMotor.Right = voltage;
  vmVariables.vbat_motor[E_Motor_Right] = VbatMotor.Right;
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryMotorVoltage(void)
{
  return (VbatMotor.Left + VbatMotor.Right);
}

//_____________________________________________________________________________

void STM32_SetLeftInducedVoltage(int16_t voltage)
{
  Vind.Left = voltage;
  vmVariables.uind[E_Motor_Left] = Vind.Left;
}

//_____________________________________________________________________________

void STM32_SetRightInducedVoltage(int16_t voltage)
{
  Vind.Right = voltage;
  vmVariables.uind[E_Motor_Right] = Vind.Right;
}

//_____________________________________________________________________________

T_Motor STM32_GetInducedVoltage(void)
{
  return Vind;
}

//_____________________________________________________________________________

void STM32_SetLeftMotorCurrent(int16_t current)
{
  Current.Left = current;
  vmVariables.imot[E_Motor_Left] = Current.Left;
}

//_____________________________________________________________________________

void STM32_SetRightMotorCurrent(int16_t current)
{
  Current.Right = current;
  vmVariables.imot[E_Motor_Right] = Current.Right;
}

//_____________________________________________________________________________

T_Motor STM32_GetMotorCurrent(void)
{
  return Current;
}

//_____________________________________________________________________________

void STM32_SetLeftPwmDutyCycle(int16_t dutycycle)
{
  DutyCycle.Left = dutycycle;
  vmVariables.pwm[E_Motor_Left] = DutyCycle.Left;
}

//_____________________________________________________________________________

void STM32_SetRightPwmDutyCycle(int16_t dutycycle)
{
  DutyCycle.Right = dutycycle;
  vmVariables.pwm[E_Motor_Right] = DutyCycle.Right;
}

//_____________________________________________________________________________

void STM32_UpdateMotorTargets(void)
{
  Target.Left  = vmVariables.target[0];
  Target.Right = vmVariables.target[1];

  //ESP_LOGE(Tag, "%d %d", Target.Left, Target.Right);
}

//_____________________________________________________________________________

int16_t STM32_GetLeftMotorTarget(void)
{
  return Target.Left;
}

//_____________________________________________________________________________

int16_t STM32_GetRightMotorTarget(void)
{
  return Target.Right;
}

//_____________________________________________________________________________

void STM32_SetSoundLevel(int16_t level)
{
  vmVariables.sound_level = level;
}

//_____________________________________________________________________________

void STM32_SetSoundThreshold(int16_t threshold)
{
  vmVariables.sound_tresh = threshold;
}

//_____________________________________________________________________________

int16_t STM32_GetSoundThreshold(void)
{
  return vmVariables.sound_tresh;
}

//_____________________________________________________________________________

void STM32_SetSoundMean(int16_t mean)
{
  vmVariables.sound_mean = mean;
}

//_____________________________________________________________________________

void STM32_SetProxIRValues(int16_t* buffer, uint16_t position)
{
  //memcpy(ProxIR, buffer + position, PROX_IR_SENSORS_NUM);
  //memcpy(vmVariables.prox, ProxIR, PROX_IR_SENSORS_NUM);

  ProxIR.FrontLeft        = buffer[position];
  ProxIR.FrontLeftCenter  = buffer[position + 1u];
  ProxIR.FrontCenter      = buffer[position + 2u];
  ProxIR.FrontRightCenter = buffer[position + 3u];
  ProxIR.FrontRight       = buffer[position + 4u];
  ProxIR.BackLeft         = buffer[position + 5u];
  ProxIR.BackRight        = buffer[position + 6u];

  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
	vmVariables.prox[index] = buffer[position + index];
  }

#if 0
  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
	ProxIR[index] = buffer[index + position];
	vmVariables.prox[index] = ProxIR[index];
  }
#endif
}

//_____________________________________________________________________________

T_ProxIR STM32_GetProxIRValues(void)
{
  return ProxIR;
}

//_____________________________________________________________________________

void STM32_SetGroundIRValues(int16_t* buffer, uint16_t position)
{
  GroundIR.LeftAmbiant    = buffer[position];
  GroundIR.RightAmbiant   = buffer[position + 1u];
  GroundIR.LeftReflected  = buffer[position + 2u];
  GroundIR.RightReflected = buffer[position + 3u];
  GroundIR.LeftDelta      = buffer[position + 4u];
  GroundIR.RightDelta     = buffer[position + 5u];

  vmVariables.ground_ambiant[E_GroundIR_Right]   = GroundIR.LeftAmbiant;
  vmVariables.ground_ambiant[E_GroundIR_Left]    = GroundIR.RightAmbiant;
  vmVariables.ground_reflected[E_GroundIR_Right] = GroundIR.LeftReflected;
  vmVariables.ground_reflected[E_GroundIR_Left]  = GroundIR.RightReflected;
  vmVariables.ground_delta[E_GroundIR_Right]     = GroundIR.LeftDelta;
  vmVariables.ground_delta[E_GroundIR_Left]      = GroundIR.RightDelta;
}

//_____________________________________________________________________________
#if 0
void STM32_SetFrontLeftProxIRValue(int16_t value)
{
  ProxIR.FrontLeft = value;
  vmVariables.prox[E_ProxIR_FrontLeft] = ProxIR.FrontLeft;
}

//_____________________________________________________________________________

void STM32_SetFrontLeftCenterProxIRValue(int16_t value)
{
  ProxIR.FrontLeftCenter = value;
  vmVariables.prox[E_ProxIR_FrontLeftCenter] = ProxIR.FrontLeftCenter;
}

//_____________________________________________________________________________

void STM32_SetFrontCenterProxIRValue(int16_t value)
{
  ProxIR.FrontCenter = value;
  vmVariables.prox[E_ProxIR_FrontCenter] = ProxIR.FrontCenter;
}

//_____________________________________________________________________________

void STM32_SetFrontRightCenterProxIRValue(int16_t value)
{
  ProxIR.FrontRightCenter = value;
  vmVariables.prox[E_ProxIR_FrontRightCenter] = ProxIR.FrontRightCenter;
}

//_____________________________________________________________________________

void STM32_SetFrontRightProxIRValue(int16_t value)
{
  ProxIR.FrontRight = value;
  vmVariables.prox[E_ProxIR_FrontRight] = ProxIR.FrontRight;
}

//_____________________________________________________________________________

void STM32_SetBackLeftProxIRValue(int16_t value)
{
  ProxIR.BackLeft = value;
  vmVariables.prox[E_ProxIR_BackLeft] = ProxIR.BackLeft;
}

//_____________________________________________________________________________

void STM32_SetBackRightProxIRValue(int16_t value)
{
  ProxIR.BackRight = value;
  vmVariables.prox[E_ProxIR_BackRight] = ProxIR.BackRight;
}
#endif
//_____________________________________________________________________________

bool STM32_IsUSBCablePresent(void)
{
  return ((Status & (1u << USB_CABLE_IS_PRESENT_BIT_POS)) == USB_CABLE_IS_PRESENT_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsUSBPortOpen(void)
{
  return ((Status & (1u << USB_PORT_IS_OPEN_BIT_POS)) == USB_PORT_IS_OPEN_BIT_MASK);
  //return false;  // FIXME temporary used to run Aseba with WIFI
  //return true;  // FIXME temporary used to run Aseba with UART
}

//_____________________________________________________________________________

bool STM32_IsModeUpdateRequested(void)
{
  return ((Status & (1u << MODE_UPDATE_BIT_POS)) == MODE_UPDATE_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsReadyToSwitchOff(void)
{
  return ((Status & (1u << READY_TO_SWITCH_OFF_BIT_POS)) == READY_TO_SWITCH_OFF_BIT_MASK);
}

//_____________________________________________________________________________

void STM32_AllowToSwitchOff(void)
{
  Status |= (1u << OK_TO_SWITCH_OFF_BIT_POS);
}

//_____________________________________________________________________________

bool STM32_IsAllowedToSwitchOff(void)
{
  return ((Status & (1u << OK_TO_SWITCH_OFF_BIT_POS)) == OK_TO_SWITCH_OFF_BIT_MASK);
}
