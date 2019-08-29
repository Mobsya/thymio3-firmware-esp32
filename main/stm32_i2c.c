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

#include "esp_log.h"

#include "stm32_i2c.h"

#include "aseba_esp32.h"
#include "board.h"
#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SLAVE_ADDRESS                       0x04u  //!< Slave address

#define SETTINGS_REG_ADDRESS                0x01u  //!< Settings register address           (Read/Write)
#define STATUS_REG_ADDRESS                  0x02u  //!< Status register address             (Read/Write)
#define WHO_AM_I_REG_ADDRESS                0x03u  //!< WHO_AM_I register address           (Read only)
#define LEFT_MOTOR_TARGET_REG_ADDRESS       0x04u  //!< Left motor target register address  (Read/Write)
#define RIGHT_MOTOR_TARGET_REG_ADDRESS      0x05u  //!< Right motor target register address (Read/Write)
#define BATTERY_MOTOR_VOLTAGE_REG_ADDRESS   0x06u  //!< Battery voltage register address    (Read only)
#define INDUCED_VOLTAGE_REG_ADDRESS         0x07u  //!< Induced voltage register address    (Read only)
#define MOTOR_CURRENT_REG_ADDRESS           0x08u  //!< Motor current register address      (Read only)
#define PWM_DUTY_CYCLE_REG_ADDRESS          0x09u  //!< PWM duty cycle register address     (Read only)
#define BATTERY_VOLTAGE_REG_ADDRESS         0x0Au  //!< Battery voltage register address    (Read only)
#define PROX_IR_VALUE_REG_ADDRESS           0x0Bu  //!< Prox IR value register address      (Read only)
#define GROUND_IR_VALUE_REG_ADDRESS         0x0Cu  //!< Ground IR value register address    (Read only)
#define GROUND_IR_LEDS_REG_ADDRESS          0x0Du  //!< Ground IR LEDs register address     (Read/Write)
#define MICROPHONE_VOLTAGE_REG_ADDRESS      0x0Eu  //!< Microphone voltage register address (Read only)

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
#define MICROPHONE_VOLTAGE_MESSAGE_LENGTH      2u  //!< Battery voltage message length in bytes

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details Motor selection
typedef enum
{
  E_Motor_Left,  //!< Left motor
  E_Motor_Right  //!< Right motor
} T_Motor;

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

static const char* Tag = "stm32";

static uint8_t Status = 0u;

static int16_t VbatMotor[MOTORS_NUM] = {0, 0};
static int16_t Vind[MOTORS_NUM]      = {0, 0};
static int16_t DutyCycle[MOTORS_NUM] = {0, 0};
static int16_t Current[MOTORS_NUM]   = {0, 0};

static int16_t ProxIRValue[PROX_IR_SENSORS_NUM] = {0, 0, 0, 0, 0, 0, 0};

static int16_t GroundIRAmbient[GROUND_IR_SENSORS_NUM]   =  {0, 0};
static int16_t GroundIRReflected[GROUND_IR_SENSORS_NUM] =  {0, 0};
static int16_t GroundIRDelta[GROUND_IR_SENSORS_NUM]     =  {0, 0};

static int16_t Vbat = 0;

static int16_t Microphone = 0;

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

void STM32_UpdateBehaviorStatus(uint16_t status)
{

}

//_____________________________________________________________________________

void STM32_UpdateProxIRLedsBrightness(uint16_t l0, uint16_t l1, uint16_t l2, uint16_t l3,
                                      uint16_t l4, uint16_t l5, uint16_t l6, uint16_t l7)
{

}

//_____________________________________________________________________________

void STM32_UpdateMicrophoneLedBrightness(uint16_t brightness)
{

}

//_____________________________________________________________________________

void STM32_UpdateSettings(T_Settings settings)
{
  uint8_t data[SETTINGS_MESSAGE_LENGTH];

  data[0] = (uint8_t)(settings.LeftMotor);
  data[1] = (uint8_t)((settings.LeftMotor) >> 8);
  data[2] = (uint8_t)(settings.RightMotor);
  data[3] = (uint8_t)((settings.RightMotor) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, SETTINGS_REG_ADDRESS, data, SETTINGS_MESSAGE_LENGTH);
}

//_____________________________________________________________________________

void STM32_ReadStatus(void)
{
  uint8_t data;

  static uint8_t oldStatus = 0x00;

  I2C_ReadFromAddress(SLAVE_ADDRESS, STATUS_REG_ADDRESS, &data, STATUS_MESSAGE_LENGTH);

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
  //return true;  // FIXME temporary used to run Aseba with UART
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
  uint8_t data[LEFT_MOTOR_TARGET_MESSAGE_LENGTH];

  data[0] = (uint8_t)(target[E_Motor_Left]);
  data[1] = (uint8_t)((target[E_Motor_Left]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, LEFT_MOTOR_TARGET_MESSAGE_LENGTH);
}

//_____________________________________________________________________________

void STM32_UpdateRightMotorTarget(int16_t* target)
{
  uint8_t data[RIGHT_MOTOR_TARGET_MESSAGE_LENGTH];

  data[0] = (uint8_t)(target[E_Motor_Right]);
  data[1] = (uint8_t)((target[E_Motor_Right]) >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, RIGHT_MOTOR_TARGET_MESSAGE_LENGTH);
}

//_____________________________________________________________________________

void STM32_UpdateGroundIRLedsBrightness(int16_t* brightness)
{
  uint8_t data[GROUND_IR_LEDS_MESSAGE_LENGTH];

  data[0] = (uint8_t)(brightness[0]);
  data[1] = (uint8_t)(brightness[0] >> 8);
  data[2] = (uint8_t)(brightness[1]);
  data[3] = (uint8_t)(brightness[1] >> 8);

  I2C_WriteToAddress(SLAVE_ADDRESS, GROUND_IR_LEDS_REG_ADDRESS, data, GROUND_IR_LEDS_MESSAGE_LENGTH);
}

//_____________________________________________________________________________

void STM32_GetLeftMotorTarget(int16_t* target)
{
  uint8_t data[LEFT_MOTOR_TARGET_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, LEFT_MOTOR_TARGET_REG_ADDRESS, data, LEFT_MOTOR_TARGET_MESSAGE_LENGTH);

  target[E_Motor_Left] = ((data[1] << 8) | data[0]);

#if 0  // TODO
  vmVariables.target[E_Motor_Left] = target[E_Motor_Left];
#endif
}

//_____________________________________________________________________________

void STM32_GetRightMotorTarget(int16_t* target)
{
  uint8_t data[RIGHT_MOTOR_TARGET_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, RIGHT_MOTOR_TARGET_REG_ADDRESS, data, RIGHT_MOTOR_TARGET_MESSAGE_LENGTH);

  target[E_Motor_Right] = ((data[1] << 8) | data[0]);

#if 0  // TODO
  vmVariables.target[E_Motor_Right] = target[E_Motor_Right];
#endif
}

//_____________________________________________________________________________

void STM32_ReadPwmDutyCycle(void)
{
  uint8_t data[PWM_DUTY_CYCLE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, PWM_DUTY_CYCLE_REG_ADDRESS, data, PWM_DUTY_CYCLE_MESSAGE_LENGTH);

  DutyCycle[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  DutyCycle[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.pwm[E_Motor_Left]  = DutyCycle[E_Motor_Left];
  vmVariables.pwm[E_Motor_Right] = DutyCycle[E_Motor_Right];
}

//_____________________________________________________________________________

void STM32_ReadBatteryMotorVoltage(void)
{
  uint8_t data[BATTERY_MOTOR_VOLTAGE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BATTERY_MOTOR_VOLTAGE_REG_ADDRESS, data, BATTERY_MOTOR_VOLTAGE_MESSAGE_LENGTH);

  VbatMotor[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  VbatMotor[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.vbat_motor[E_Motor_Left]  = VbatMotor[E_Motor_Left];
  vmVariables.vbat_motor[E_Motor_Right] = VbatMotor[E_Motor_Right];
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryMotorVoltage(void)
{
  return (VbatMotor[E_Motor_Left] + VbatMotor[E_Motor_Right]);
}

//_____________________________________________________________________________

void STM32_ReadInducedVoltage(void)
{
  uint8_t data[INDUCED_VOLTAGE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, INDUCED_VOLTAGE_REG_ADDRESS, data, INDUCED_VOLTAGE_MESSAGE_LENGTH);

  Vind[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  Vind[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.uind[E_Motor_Left]  = Vind[E_Motor_Left];
  vmVariables.uind[E_Motor_Right] = Vind[E_Motor_Right];

  SET_EVENT(EVENT_MOTOR);
}

//_____________________________________________________________________________

void STM32_ReadMotorCurrent(void)
{
  uint8_t data[MOTOR_CURRENT_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, MOTOR_CURRENT_REG_ADDRESS, data, MOTOR_CURRENT_MESSAGE_LENGTH);

  Current[E_Motor_Left]  = ((data[1] << 8) | data[0]);
  Current[E_Motor_Right] = ((data[3] << 8) | data[2]);

  vmVariables.imot[E_Motor_Left]  = Current[E_Motor_Left];
  vmVariables.imot[E_Motor_Right] = Current[E_Motor_Right];
}

//_____________________________________________________________________________

void STM32_ReadBatteryVoltage(void)
{
  uint8_t data[BATTERY_VOLTAGE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BATTERY_VOLTAGE_REG_ADDRESS, data, BATTERY_VOLTAGE_MESSAGE_LENGTH);

  Vbat = ((data[1] << 8) | data[0]);

  vmVariables.vbat = Vbat;
}

//_____________________________________________________________________________

int16_t STM32_GetBatteryVoltage(void)
{
  return Vbat;
}

//_____________________________________________________________________________

void STM32_ReadMicrophoneVoltage(void)
{
  uint8_t data[MICROPHONE_VOLTAGE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, MICROPHONE_VOLTAGE_REG_ADDRESS, data, MICROPHONE_VOLTAGE_MESSAGE_LENGTH);

  Microphone = ((data[1] << 8) | data[0]);

  vmVariables.microphone = Microphone;
}

//_____________________________________________________________________________

void STM32_ReadProxIRValue(void)
{
  uint8_t data[PROX_IR_VALUE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, PROX_IR_VALUE_REG_ADDRESS, data, PROX_IR_VALUE_MESSAGE_LENGTH);

  ProxIRValue[E_ProxIR_FrontLeft]        =  ((data[1] << 8) | data[0]);
  ProxIRValue[E_ProxIR_FrontLeftCenter]  =  ((data[3] << 8) | data[2]);
  ProxIRValue[E_ProxIR_FrontCenter]      =  ((data[5] << 8) | data[4]);
  ProxIRValue[E_ProxIR_FrontRightCenter] =  ((data[7] << 8) | data[6]);
  ProxIRValue[E_ProxIR_FrontRight]       =  ((data[9] << 8) | data[8]);
  ProxIRValue[E_ProxIR_BackLeft]         = ((data[11] << 8) | data[10]);
  ProxIRValue[E_ProxIR_BackRight]        = ((data[13] << 8) | data[12]);

  vmVariables.prox[E_ProxIR_FrontLeft]        = ProxIRValue[E_ProxIR_FrontLeft];
  vmVariables.prox[E_ProxIR_FrontLeftCenter]  = ProxIRValue[E_ProxIR_FrontLeftCenter];
  vmVariables.prox[E_ProxIR_FrontCenter]      = ProxIRValue[E_ProxIR_FrontCenter];
  vmVariables.prox[E_ProxIR_FrontRightCenter] = ProxIRValue[E_ProxIR_FrontRightCenter];
  vmVariables.prox[E_ProxIR_FrontRight]       = ProxIRValue[E_ProxIR_FrontRight];
  vmVariables.prox[E_ProxIR_BackLeft]         = ProxIRValue[E_ProxIR_BackLeft];
  vmVariables.prox[E_ProxIR_BackRight]        = ProxIRValue[E_ProxIR_BackRight];
}

//_____________________________________________________________________________

void STM32_ReadGroundIRValue(void)
{
  uint8_t data[GROUND_IR_VALUE_MESSAGE_LENGTH];

  I2C_ReadFromAddress(SLAVE_ADDRESS, GROUND_IR_VALUE_REG_ADDRESS, data, GROUND_IR_VALUE_MESSAGE_LENGTH);

  GroundIRAmbient[E_GroundIR_Right]   =  ((data[1] << 8) | data[0]);
  GroundIRAmbient[E_GroundIR_Left]    =  ((data[3] << 8) | data[2]);
  GroundIRReflected[E_GroundIR_Right] =  ((data[5] << 8) | data[4]);
  GroundIRReflected[E_GroundIR_Left]  =  ((data[7] << 8) | data[6]);
  GroundIRDelta[E_GroundIR_Right]     =  ((data[9] << 8) | data[8]);
  GroundIRDelta[E_GroundIR_Left]      =  ((data[11] << 8) | data[10]);

  vmVariables.ground_ambiant[E_GroundIR_Right]   = GroundIRAmbient[E_GroundIR_Right];
  vmVariables.ground_ambiant[E_GroundIR_Left]    = GroundIRAmbient[E_GroundIR_Left];
  vmVariables.ground_reflected[E_GroundIR_Right] = GroundIRReflected[E_GroundIR_Right];
  vmVariables.ground_reflected[E_GroundIR_Left]  = GroundIRReflected[E_GroundIR_Left];
  vmVariables.ground_delta[E_GroundIR_Right]     = GroundIRDelta[E_GroundIR_Right];
  vmVariables.ground_delta[E_GroundIR_Left]      = GroundIRDelta[E_GroundIR_Left];
}

//_____________________________________________________________________________
#if 0
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
#endif
//_____________________________________________________________________________

static void ReadId(uint8_t* id)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, id, WHO_AM_I_MESSAGE_LENGTH);
}
