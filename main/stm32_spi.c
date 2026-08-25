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
//! \author  Vincent Gonet, Stefano Morgani
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
#include "settings.h"
#include "aseba_esp32.h"
#include "pins_def.h"
#include "spi.h"
#include "angle_controller.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SLAVE_ADDRESS                       0x04u  //!< Slave address

#define STM32_ID                            0xBCu  //!< ID of the STM32

// Status register bit mask
#define USB_CABLE_IS_PRESENT_BIT_MASK       0x01u  //!< Bit mask of USB_CABLE_IS_PRESENT
#define USB_PORT_IS_OPEN_BIT_MASK           0x02u  //!< Bit mask of USB_PORT_IS_OPEN
#define STANDBY_REQUESTED_BIT_MASK          0x04u  //!< Bit mask of STANDBY_REQUESTED
#define OK_TO_SWITCH_OFF_BIT_MASK           0x08u  //!< Bit mask of OK_TO_SWITCH_OFF

// Status register bit position
#define USB_CABLE_IS_PRESENT_BIT_POS           0u  //!< This bit is set by the STM32
#define USB_PORT_IS_OPEN_BIT_POS               1u  //!< This bit is set by the STM32
#define STANDBY_REQUESTED_BIT_POS              2u  //!< This bit is set by the STM32
#define OK_TO_SWITCH_OFF_BIT_POS               3u  //!< This bit is set by the ESP32

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

#define GROUND_MAX_VALUE 1023

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
static int16_t groundsBlack[2];
static int16_t groundsWhite[2];
static int16_t groundsDeltaCalib[2];

static int16_t Vbat = 0;
static uint8_t clap_count = 0;

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

  Settings_GetGroundBlackSettings(groundsBlack);
  Settings_GetGroundWhiteSettings(groundsWhite);
  groundsDeltaCalib[0] = groundsWhite[0] - groundsBlack[0];
  if(groundsDeltaCalib[0] < 1)
  {
    groundsDeltaCalib[0] = 1;
  }
  if(groundsDeltaCalib[0] > GROUND_MAX_VALUE)
  {
    groundsDeltaCalib[0] = GROUND_MAX_VALUE;
  }
  groundsDeltaCalib[1] = groundsWhite[1] - groundsBlack[1];
  if(groundsDeltaCalib[1] < 1)
  {
    groundsDeltaCalib[1] = 1;
  }
  if(groundsDeltaCalib[1] > GROUND_MAX_VALUE)
  {
    groundsDeltaCalib[1] = GROUND_MAX_VALUE;
  }
}

//_____________________________________________________________________________

void STM32_SetStatus(uint16_t status)
{
  // Update only the bits handled by the STM32
  status &= 0xFFF7;
  Status |= status;
}

//_____________________________________________________________________________

uint16_t STM32_GetStatus(void)
{
  return Status;
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

void STM32_SetBatteryMotorVoltages(int16_t* buffer, uint16_t position)
{
  VbatMotor.Left  = buffer[position];
  VbatMotor.Right = buffer[position + 1u];

  vmVariables.vbat_motor[E_Motor_Left]  = VbatMotor.Left;
  vmVariables.vbat_motor[E_Motor_Right] = VbatMotor.Right;
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryMotorVoltage(void)
{
  return (VbatMotor.Left + VbatMotor.Right);
}

//_____________________________________________________________________________

void STM32_SetInducedVoltages(int16_t* buffer, uint16_t position)
{
  Vind.Left  = buffer[position];
  Vind.Right = buffer[position + 1u];

  vmVariables.uind[E_Motor_Left]  = Vind.Left;
  vmVariables.uind[E_Motor_Right] = Vind.Right;
}

//_____________________________________________________________________________

T_Motor STM32_GetInducedVoltage(void)
{
  return Vind;
}

//_____________________________________________________________________________

void STM32_SetMotorCurrents(int16_t* buffer, uint16_t position)
{
  Current.Left  = buffer[position];
  Current.Right = buffer[position + 1u];

  vmVariables.imot[E_Motor_Left] = Current.Left;
  vmVariables.imot[E_Motor_Right] = Current.Right;
}

//_____________________________________________________________________________

T_Motor STM32_GetMotorCurrent(void)
{
  return Current;
}

//_____________________________________________________________________________

void STM32_SetPwmDutyCycles(int16_t* buffer, uint16_t position)
{
  DutyCycle.Left  = buffer[position];
  DutyCycle.Right = buffer[position + 1u];

  vmVariables.pwm[E_Motor_Left]  = DutyCycle.Left;
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

int16_t STM32_GetLeftMotorPwm(void)
{
  return DutyCycle.Left;
}

//_____________________________________________________________________________

int16_t STM32_GetRightMotorPwm(void)
{
  return DutyCycle.Right;
}

//_____________________________________________________________________________

void STM32_SetMicrophoneIntensity(int16_t intensity)
{
  vmVariables.micro_intensity = intensity;
  if(clap_count == 0) {
    if(intensity > CLAP_THR) {
      SET_EVENT(EVENT_MIC);
      //ESP_LOGE(Tag, "clap");
      clap_count = 4; // Avoid many events for the same clap. 4 cycles means 4*20ms=80ms
    }
  } else {
    clap_count--;
  }
}

//_____________________________________________________________________________

int16_t STM32_GetMicrophoneIntensity(void)
{
  return vmVariables.micro_intensity;
}

//_____________________________________________________________________________

void STM32_SetMicrophoneThreshold(int16_t threshold)
{
  vmVariables.micro_tresh = threshold;
}

//_____________________________________________________________________________

int16_t STM32_GetMicrophoneThreshold(void)
{
  return vmVariables.micro_tresh;
}

//_____________________________________________________________________________

void STM32_SetMicrophoneMean(int16_t mean)
{
  vmVariables.micro_mean = mean;
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

void STM32_SetGroundRange(int16_t* white, int16_t* black)
{
  memcpy(groundsWhite, white, 4);
  memcpy(groundsBlack, black, 4);
  groundsDeltaCalib[0] = groundsWhite[0] - groundsBlack[0];
  if(groundsDeltaCalib[0] < 1)
  {
    groundsDeltaCalib[0] = 1;
  }
  if(groundsDeltaCalib[0] > GROUND_MAX_VALUE)
  {
    groundsDeltaCalib[0] = GROUND_MAX_VALUE;
  }
  groundsDeltaCalib[1] = groundsWhite[1] - groundsBlack[1];
  if(groundsDeltaCalib[1] < 1)
  {
    groundsDeltaCalib[1] = 1;
  }
  if(groundsDeltaCalib[1] > GROUND_MAX_VALUE)
  {
    groundsDeltaCalib[1] = GROUND_MAX_VALUE;
  }
}

//_____________________________________________________________________________

void STM32_SetGroundIRValues(int16_t* buffer, uint16_t position)
{
  static int32_t temp = 0;
  GroundIR.LeftAmbiant    = buffer[position];
  GroundIR.RightAmbiant   = buffer[position + 1u];
  GroundIR.LeftReflected  = buffer[position + 2u];
  GroundIR.RightReflected = buffer[position + 3u];
  GroundIR.LeftDelta      = GroundIR.LeftReflected - GroundIR.LeftAmbiant; //buffer[position + 4u]; // This is the calibrated delta from the STM32
  if(GroundIR.LeftDelta < 0)
  {
    GroundIR.LeftDelta = 0;
  }
  GroundIR.RightDelta     = GroundIR.RightReflected - GroundIR.RightAmbiant; //buffer[position + 5u]; // This is the calibrated delta from the STM32
  if(GroundIR.RightDelta < 0)
  {
    GroundIR.RightDelta = 0;
  }
  //ESP_LOGI(Tag, "1) amb=%d, refl=%d, delta=%d (%d), groundsBlack=%d, groundsDeltaCalib=%d", GroundIR.LeftAmbiant, GroundIR.LeftReflected, GroundIR.LeftDelta, buffer[position + 4u], groundsBlack[0], groundsDeltaCalib[0]);
  // Put ground values in the range 0...GROUND_MAX_VALUE: (ground value - "on air offset") / "delta calib" * GROUND_MAX_VALUE
  temp = GroundIR.LeftDelta - groundsBlack[0];
  if(temp < 0)
  {
    GroundIR.LeftDelta = 0; // Or update the ground black calibration??
  }
  else
  {
    temp = temp*GROUND_MAX_VALUE/groundsDeltaCalib[0];
    GroundIR.LeftDelta = temp;
    if(GroundIR.LeftDelta > GROUND_MAX_VALUE)
    {
      GroundIR.LeftDelta = GROUND_MAX_VALUE;
    }
  }
  //ESP_LOGI(Tag, "2) amb=%d, refl=%d, delta=%d (%d), groundsBlack=%d, groundsDeltaCalib=%d", GroundIR.LeftAmbiant, GroundIR.LeftReflected, GroundIR.LeftDelta, buffer[position + 4u], groundsBlack[0], groundsDeltaCalib[0]);

  temp = GroundIR.RightDelta - groundsBlack[1];
  if(temp < 0)
  {
    GroundIR.RightDelta = 0;
  }
  else
  {
    temp = temp*GROUND_MAX_VALUE/groundsDeltaCalib[1];
    GroundIR.RightDelta = temp;
    if(GroundIR.RightDelta > GROUND_MAX_VALUE)
    {
      GroundIR.RightDelta = GROUND_MAX_VALUE;
    }    
  }

  vmVariables.ground_ambiant[E_GroundIR_Left]   = GroundIR.LeftAmbiant;
  vmVariables.ground_ambiant[E_GroundIR_Right]    = GroundIR.RightAmbiant;
  vmVariables.ground_reflected[E_GroundIR_Left] = GroundIR.LeftReflected;
  vmVariables.ground_reflected[E_GroundIR_Right]  = GroundIR.RightReflected;
  vmVariables.ground_delta[E_GroundIR_Left]     = GroundIR.LeftDelta;
  vmVariables.ground_delta[E_GroundIR_Right]      = GroundIR.RightDelta;
}

//_____________________________________________________________________________

void STM32_SetProxIRData(int16_t* buffer, uint16_t position)
{
  for (uint8_t index = 0u; index < PROX_IR_SENSORS_NUM; index++)
  {
    vmVariables.sensor_data[index] = buffer[position + index];
  }
}

//_____________________________________________________________________________

int16_t STM32_GetProxIRTxData(void)
{
  return vmVariables.ir_tx_data;
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
	//ESP_LOGE(Tag, "status=%x", Status);
  return ((Status & (1u << USB_CABLE_IS_PRESENT_BIT_POS)) == USB_CABLE_IS_PRESENT_BIT_MASK);
}

//_____________________________________________________________________________

bool STM32_IsUSBPortOpen(void)
{
	//ESP_LOGE(Tag, "status=%x", Status);
  return ((Status & (1u << USB_PORT_IS_OPEN_BIT_POS)) == USB_PORT_IS_OPEN_BIT_MASK);
  //return false;  // FIXME temporary used to run Aseba with WIFI
  //return true;  // FIXME temporary used to run Aseba with UART
}

//_____________________________________________________________________________

bool STM32_IsStandbyRequested(void)
{
  return ((Status & (1u << STANDBY_REQUESTED_BIT_POS)) == STANDBY_REQUESTED_BIT_MASK);
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

//_____________________________________________________________________________

void GetProximityValues(int16_t* buffer) {
  buffer[0] = ProxIR.FrontLeft;
  buffer[1] = ProxIR.FrontLeftCenter;
  buffer[2] = ProxIR.FrontCenter;
  buffer[3] = ProxIR.FrontRightCenter;
  buffer[4] = ProxIR.FrontRight;
  buffer[5] = ProxIR.BackLeft;
  buffer[6] = ProxIR.BackRight;
}

//_____________________________________________________________________________

uint16_t GetProximityValue(uint8_t prox_id) {
  switch(prox_id) {
    case 0:
      return ProxIR.FrontLeft;
      break;
    case 1:
      return ProxIR.FrontLeftCenter;
      break;
    case 2:
      return ProxIR.FrontCenter;
      break;
    case 3:
      return ProxIR.FrontRightCenter;
      break;
    case 4:
      return ProxIR.FrontRight;
      break;
    case 5:
      return ProxIR.BackLeft;
      break;
    case 6:
      return ProxIR.BackRight;
      break; 
    default:
      return 0;                                   
  }
}

//_____________________________________________________________________________

void GetGroundValues(int16_t* buffer) {
  buffer[0] = GroundIR.LeftDelta;
  buffer[1] = GroundIR.RightDelta;
}

//_____________________________________________________________________________

uint16_t GetGroundValue(uint8_t ground_id) {
  switch(ground_id) {
    case 0:
      return GroundIR.LeftDelta;
      break;
    case 1:
      return GroundIR.RightDelta;
      break;
    default:
      return 0;    
  }
}

//_____________________________________________________________________________

void GetGroundAmbients(int16_t* buffer) {
  buffer[0] = GroundIR.LeftAmbiant;
  buffer[1] = GroundIR.RightAmbiant;
}

//_____________________________________________________________________________

uint16_t GetGroundAmbient(uint8_t ground_id) {
  switch(ground_id) {
    case 0:
      return GroundIR.LeftAmbiant;
      break;
    case 1:
      return GroundIR.RightAmbiant;
      break;
    default:
      return 0;    
  }
}

//_____________________________________________________________________________

void GetGroundReflecteds(int16_t* buffer) {
  buffer[0] = GroundIR.LeftReflected;
  buffer[1] = GroundIR.RightReflected;
}

//_____________________________________________________________________________

uint16_t GetGroundReflected(uint8_t ground_id) {
  switch(ground_id) {
    case 0:
      return GroundIR.LeftReflected;
      break;
    case 1:
      return GroundIR.RightReflected;
      break;
    default:
      return 0;    
  }
}

//_____________________________________________________________________________

void SetMotorTargets(int16_t left, int16_t right)
{
  Target.Left  = left;
  Target.Right = right;
  if((Target.Left == 0) && (Target.Right == 0)) {
    AngleController_Stop();
  }
}

//_____________________________________________________________________________

int16_t GetLeftSpeed(void) {
  return Vind.Left;  
}

//_____________________________________________________________________________

int16_t GetRightSpeed(void) {
  return Vind.Right;
}
