//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm6ds3us.c
//! \brief   This module provides the useful functions to use the LSM6DS3US device
//!          (3D accelerometer and 3D gyroscope)
//!
//! \author  Vincent Gonet
//!
//! \version $Id: lsm6ds3us.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <esp_log.h>

#include "lsm6ds3us.h"

#include "board.h"
#include "gpio.h"
#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SA0_PIN_STATE                           1u  //!< ADDR pin state used to set the slave address
#define LSM6DS3US_ADDRESS                    0x6Au  //!< Device address

#define SLAVE_ADDRESS                 (LSM6DS3US_ADDRESS | SA0_PIN_STATE)  //!< Slave address

// Registers addresses
#define FUNC_CFG_ACCESS_REG_ADDRESS          0x01u  //!< Enable embedded functions register address                             (Read/Write)
#define SENSOR_SYNC_TIME_FRAME_REG_ADDRESS   0x04u  //!< Sensor synchronization time frame register address                     (Read/Write)
#define FIFO_CTRL1_REG_ADDRESS               0x06u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL2_REG_ADDRESS               0x07u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL3_REG_ADDRESS               0x08u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL4_REG_ADDRESS               0x09u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL5_REG_ADDRESS               0x0Au  //!< FIFO control register address                                          (Read/Write)
#define ORIENT_CFG_G_REG_ADDRESS             0x0Bu  //!< Angular rate sensor sign and orientation register address              (Read/Write)
#define INT1_CTRL_REG_ADDRESS                0x0Du  //!< INT1 pad control register address                                      (Read/Write)
#define INT2_CTRL_REG_ADDRESS                0x0Eu  //!< INT2 pad control register address                                      (Read/Write)
#define WHO_AM_I_REG_ADDRESS                 0x0Fu  //!< Who_AM_I register address                                              (Read only)
#define CTRL1_XL_REG_ADDRESS                 0x10u  //!< Linear acceleration sensor control register 1 address                  (Read/Write)
#define CTRL2_G_REG_ADDRESS                  0x11u  //!< Angular rate sensor control register 2 address                         (Read/Write)
#define CTRL3_C_REG_ADDRESS                  0x12u  //!< Control register 3 address                                             (Read/Write)
#define CTRL4_C_REG_ADDRESS                  0x13u  //!< Control register 4 address                                             (Read/Write)
#define CTRL5_C_REG_ADDRESS                  0x14u  //!< Control register 5 address                                             (Read/Write)
#define CTRL6_C_REG_ADDRESS                  0x15u  //!< Angular rate sensor control register 6 address                         (Read/Write)
#define CTRL7_G_REG_ADDRESS                  0x16u  //!< Angular rate sensor control register 7 address                         (Read/Write)
#define CTRL8_XL_REG_ADDRESS                 0x17u  //!< Linear acceleration sensor control register 8 address                  (Read/Write)
#define CTRL9_XL_REG_ADDRESS                 0x18u  //!< Linear acceleration sensor control register 9 address                  (Read/Write)
#define CTRL10_C_REG_ADDRESS                 0x19u  //!< Control register 10 address                                            (Read/Write)
#define MASTER_CONFIG_REG_ADDRESS            0x1Au  //!< Master configuration register address                                  (Read/Write)
#define WAKE_UP_SRC_REG_ADDRESS              0x1Bu  //!< Wake up interrupt source register address                              (Read only)
#define TAP_SRC_REG_ADDRESS                  0x1Cu  //!< Tap source register address                                            (Read only)
#define D6D_SRC_REG_ADDRESS                  0x1Du  //!< Portrait, landscape, face-up and face-down source register address     (Read only)
#define STATUS_REG_ADDRESS                   0x1Eu  //!< Status register address                                                (Read only)
#define OUT_TEMP_L_REG_ADDRESS               0x20u  //!< Temperature data output register address                               (Read only)
#define OUT_TEMP_H_REG_ADDRESS               0x21u  //!< Temperature data output register address                               (Read only)
#define OUTX_L_G_REG_ADDRESS                 0x22u  //!< Low byte of angular rate X-axis output register address                (Read only)
#define OUTX_H_G_REG_ADDRESS                 0x23u  //!< High byte of angular rate X-axis output register address               (Read only)
#define OUTY_L_G_REG_ADDRESS                 0x24u  //!< Low byte of angular rate Y-axis output register address                (Read only)
#define OUTY_H_G_REG_ADDRESS                 0x25u  //!< High byte of angular rate Y-axis output register address               (Read only)
#define OUTZ_L_G_REG_ADDRESS                 0x26u  //!< Low byte of angular rate Z-axis output register address                (Read only)
#define OUTZ_H_G_REG_ADDRESS                 0x27u  //!< High byte of angular rate Z-axis output register address               (Read only)
#define OUTX_L_XL_REG_ADDRESS                0x28u  //!< Low byte of linear acceleration sensor X-axis output register address  (Read only)
#define OUTX_H_XL_REG_ADDRESS                0x29u  //!< High byte of linear acceleration sensor X-axis output register address (Read only)
#define OUTY_L_XL_REG_ADDRESS                0x2Au  //!< Low byte of linear acceleration sensor Y-axis output register address  (Read only)
#define OUTY_H_XL_REG_ADDRESS                0x2Bu  //!< High byte of linear acceleration sensor Y-axis output register address (Read only)
#define OUTZ_L_XL_REG_ADDRESS                0x2Cu  //!< Low byte of linear acceleration sensor Z-axis output register address  (Read only)
#define OUTZ_H_XL_REG_ADDRESS                0x2Du  //!< High byte of linear acceleration sensor Z-axis output register address (Read only)
// TODO last registers

// Manufacturer ID
#define MANUFACTURER_ID                      0x69u  //!< Manufacturer ID

// Register bits mask
// CTRL1_XL bits mask
#define ACC_ODR_XL_BIT_MASK                  0x0Fu  //!< Mask of bit ODR_XL

// CTRL2_G bits mask
#define GYR_ODR_G_BIT_MASK                   0x0Fu  //!< Mask of bit ODR_G

// Register bits position
// CTRL1_XL bits position
#define ACC_ODR_XL_BIT_POS                      4u  //!< Position of LSB bit ODR_XL

// CTRL2_G bits position
#define GYR_ODR_G_BIT_POS                       4u  //!< Position of LSB bit ODR_G

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Acc_OutputDataRate_PowerDown,
  E_Acc_OutputDataRate_12_5Hz,
  E_Acc_OutputDataRate_26Hz,
  E_Acc_OutputDataRate_52Hz,
  E_Acc_OutputDataRate_104Hz,
  E_Acc_OutputDataRate_208Hz,
  E_Acc_OutputDataRate_416Hz,
  E_Acc_OutputDataRate_833Hz,
  E_Acc_OutputDataRate_1660Hz,
  E_Acc_OutputDataRate_3330Hz,
  E_Acc_OutputDataRate_6660Hz
};
typedef uint8_t T_Acc_OutputDataRate;  //!< Accelerometer output data rate

enum
{
  E_Gyro_OutputDataRate_PowerDown,
  E_Gyro_OutputDataRate_12_5Hz,
  E_Gyro_OutputDataRate_26Hz,
  E_Gyro_OutputDataRate_52Hz,
  E_Gyro_OutputDataRate_104Hz,
  E_Gyro_OutputDataRate_208Hz,
  E_Gyro_OutputDataRate_416Hz,
  E_Gyro_OutputDataRate_833Hz,
  E_Gyro_OutputDataRate_1660Hz
};
typedef uint8_t T_Gyro_OutputDataRate;  //!< Gyroscope output data rate


//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "lsm6ds3us";

static const T_GpioPinConfig PinConfig = {ACC_INT_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the accelerometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateAccOutputDataRate(T_Acc_OutputDataRate rate);

//! \brief     Read the acceleration
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadAcceleration.svg
static void ReadAcceleration(T_Axis* acceleration);

//! \brief     Read the angular position
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadAngle.svg
static void ReadAngularPosition(T_Axis* angularPosition);

//! \brief     Update the gyroscope output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateGyroOutputDataRate(T_Gyro_OutputDataRate rate);

//! \brief     Read the manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadAccManufacturerId.svg
static void ReadManufacturerId(uint8_t* data);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Accelerometer
//-----------------------------------------------------------------------------

void LSM6DS3US_InitAccelerometer(void)
{
  Gpio_ConfigurePin(&PinConfig);
  UpdateAccOutputDataRate(E_Acc_OutputDataRate_104Hz);
}

//_____________________________________________________________________________

void LSM6DS3US_GetAcceleration(T_Axis* acceleration)
{
  ReadAcceleration(acceleration);

  // TODO ConvertAcceleration(int16_t input);
}

//_____________________________________________________________________________

static void UpdateAccOutputDataRate(T_Acc_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Acc_OutputDataRate_6660Hz)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, CTRL1_XL_REG_ADDRESS, &data, 1u);

    data &= ACC_ODR_XL_BIT_MASK;
    data |= (rate << ACC_ODR_XL_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, CTRL1_XL_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid accelerometer output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void ReadAcceleration(T_Axis* acceleration)
{
  uint8_t acc[6u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, OUTX_L_XL_REG_ADDRESS, acc, 6u);

  acceleration->X = (int16_t)((uint16_t)acc[1u] << 8u) | acc[0u];
  acceleration->Y = (int16_t)((uint16_t)acc[3u] << 8u) | acc[2u];
  acceleration->Z = (int16_t)((uint16_t)acc[5u] << 8u) | acc[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", Acceleration.X,  Acceleration.Y,  Acceleration.Z);
}

//_____________________________________________________________________________

#if 0
static void ConvertAcceleration(int16_t input)
{
  if ((input & U16_BIT15) == U16_BIT15)
  {
    output = ~input + 1u;
  }
  else
  {
    output = input;
  }
}
#endif

//-----------------------------------------------------------------------------
// Gyroscope
//-----------------------------------------------------------------------------

void LSM6DS3US_InitGyroscope(void)
{
  UpdateGyroOutputDataRate(E_Gyro_OutputDataRate_104Hz);
}

//_____________________________________________________________________________

void LSM6DS3US_GetAngularPosition(T_Axis* angularPosition)
{
  ReadAngularPosition(angularPosition);

  // TODO Calculate angular position
}

//_____________________________________________________________________________

static void UpdateGyroOutputDataRate(T_Gyro_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Gyro_OutputDataRate_1660Hz)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);

    data &= GYR_ODR_G_BIT_MASK;
    data |= (rate << GYR_ODR_G_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid gyroscope output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void ReadAngularPosition(T_Axis* angularPosition)
{
  uint8_t position[6u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, OUTX_L_G_REG_ADDRESS, position, 6u);

  angularPosition->X = (int16_t)((uint16_t)position[1u] << 8u) | position[0u];
  angularPosition->Y = (int16_t)((uint16_t)position[3u] << 8u) | position[2u];
  angularPosition->Z = (int16_t)((uint16_t)position[5u] << 8u) | position[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", AngularPosition.X, AngularPosition.Y,  AngularPosition.Z);
}

//-----------------------------------------------------------------------------
// Common
//-----------------------------------------------------------------------------

void LSM6DS3US_CheckManufacturerId(void)
{
  uint8_t id = 0x00;

  ReadManufacturerId(&id);

  if (id != MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid manufacturer ID: %d", id);
  }
}

//_____________________________________________________________________________

static void ReadManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, data, 1u);
}
