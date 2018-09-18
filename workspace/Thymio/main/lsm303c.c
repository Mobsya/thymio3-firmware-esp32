//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm303c.c
//! \brief   This module provides the useful functions to use the LSM303C device
//!          (3D accelerometer and 3D magnetometer)
//!
//! \author  Vincent Gonet
//!
//! \version $Id: lsm303c.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <esp_log.h>

#include "lsm303c.h"

#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ACC_SLAVE_ADDRESS          0x1Du  //!< Accelerometer slave address
#define MAG_SLAVE_ADDRESS          0x1Eu  //!< Magnetic sensor slave address

// Accelerometer Registers addresses
#define WHO_AM_I_A_REG_ADDRESS     0x0Fu  //!< Accelerometer Who_AM_I register address                            (Read only)
#define ACT_THS_A_REG_ADDRESS      0x1Eu  //!< Activity threshold register address                                (Read/Write)
#define ACT_DUR_A_REG_ADDRESS      0x1Fu  //!< Activity duration register address                                 (Read/Write)
#define CTRL_REG1_A_REG_ADDRESS    0x20u  //!< Accelerometer control 1 register address                           (Read/Write)
#define CTRL_REG2_A_REG_ADDRESS    0x21u  //!< Accelerometer control 2 register address                           (Read/Write)
#define CTRL_REG3_A_REG_ADDRESS    0x22u  //!< Accelerometer control 3 register address                           (Read/Write)
#define CTRL_REG4_A_REG_ADDRESS    0x23u  //!< Accelerometer control 4 register address                           (Read/Write)
#define CTRL_REG5_A_REG_ADDRESS    0x24u  //!< Accelerometer control 5 register address                           (Read/Write)
#define CTRL_REG6_A_REG_ADDRESS    0x25u  //!< Accelerometer control 6 register address                           (Read/Write)
#define CTRL_REG7_A_REG_ADDRESS    0x26u  //!< Accelerometer control 7 register address                           (Read/Write)
#define STATUS_REG_A_REG_ADDRESS   0x27u  //!< Accelerometer status register address                              (Read/Write)
#define OUT_X_L_A_REG_ADDRESS      0x28u  //!< Low byte of accelerometer x-axis output register address           (Read only)
#define OUT_X_H_A_REG_ADDRESS      0x29u  //!< High byte of accelerometer x-axis output register address          (Read only)
#define OUT_Y_L_A_REG_ADDRESS      0x2Au  //!< Low byte of accelerometer y-axis output register address           (Read only)
#define OUT_Y_H_A_REG_ADDRESS      0x2Bu  //!< High byte of accelerometer y-axis output register address          (Read only)
#define OUT_Z_L_A_REG_ADDRESS      0x2Cu  //!< Low byte of accelerometer z-axis output register address           (Read only)
#define OUT_Z_H_A_REG_ADDRESS      0x2Du  //!< High byte of accelerometer z-axis output register address          (Read only)
#define FIFO_CTRL_REG_ADDRESS      0x2Eu  //!< FIFO control register address                                      (Read/Write)
#define FIFO_SRC_REG_ADDRESS       0x2Fu  //!< FIFO status control register address                               (Read only)
#define IG_CFG1_A_REG_ADDRESS      0x30u  //!< Accelerometer interrupt generator 1 configuration register address (Read/Write)
#define IG_SRC1_A_REG_ADDRESS      0x31u  //!< Accelerometer interrupt generator 1 status register address        (Read only)
#define IG_THS_X1_A_REG_ADDRESS    0x32u  //!< Accelerometer interrupt generator x1 threshold register address    (Read/Write)
#define IG_THS_Y1_A_REG_ADDRESS    0x33u  //!< Accelerometer interrupt generator y1 threshold register address    (Read/Write)
#define IG_THS_Z1_A_REG_ADDRESS    0x34u  //!< Accelerometer interrupt generator z1 threshold register address    (Read/Write)
#define IG_DUR1_A_REG_ADDRESS      0x35u  //!< Accelerometer interrupt generator 1 duration register address      (Read/Write)
#define IG_CFG2_A_REG_ADDRESS      0x36u  //!< Accelerometer interrupt generator 2 configuration register address (Read/Write)
#define IG_SRC2_A_REG_ADDRESS      0x37u  //!< Accelerometer interrupt generator 2 status register address        (Read only)
#define IG_THS2_A_REG_ADDRESS      0x38u  //!< Accelerometer interrupt generator 2 threshold register address     (Read/Write)
#define IG_DUR2_A_REG_ADDRESS      0x39u  //!< Accelerometer interrupt generator 2 duration register address      (Read/Write)
#define XL_REFERENCE_REG_ADDRESS   0x3Au  //!< (Read/Write)
#define XH_REFERENCE_REG_ADDRESS   0x3Bu  //!< (Read/Write)
#define YL_REFERENCE_REG_ADDRESS   0x3Cu  //!< (Read/write)
#define YH_REFERENCE_REG_ADDRESS   0x3Du  //!< (Read/Write)
#define ZL_REFERENCE_REG_ADDRESS   0x3Eu  //!< (Read/Write)
#define ZH_REFERENCE_REG_ADDRESS   0x3Fu  //!< (Read/Write)

// Magnetic sensor Registers addresses
#define WHO_AM_I_M_REG_ADDRESS     0x0Fu  //!< Magnetic sensor Who_AM_I register address                                   (Read only)
#define CTRL_REG1_M_REG_ADDRESS    0x20u  //!< Magnetic sensor control 1 register address                                  (Read/Write)
#define CTRL_REG2_M_REG_ADDRESS    0x21u  //!< Magnetic sensor control 2 register address                                  (Read/Write)
#define CTRL_REG3_M_REG_ADDRESS    0x22u  //!< Magnetic sensor control 3 register address                                  (Read/Write)
#define CTRL_REG4_M_REG_ADDRESS    0x23u  //!< Magnetic sensor control 4 register address                                  (Read/Write)
#define CTRL_REG5_M_REG_ADDRESS    0x24u  //!< Magnetic sensor control 5 register address                                  (Read/Write)
#define STATUS_REG_M_REG_ADDRESS   0x27u  //!< Magnetic sensor status register address                                     (Read/Write)
#define OUT_X_L_M_REG_ADDRESS      0x28u  //!< Low byte of magnetic sensor X-axis output register address                  (Read only)
#define OUT_X_H_M_REG_ADDRESS      0x29u  //!< High byte of magnetic sensor X-axis output register address                 (Read only)
#define OUT_Y_L_M_REG_ADDRESS      0x2Au  //!< Low byte of magnetic sensor Y-axis output register address                  (Read only)
#define OUT_Y_H_M_REG_ADDRESS      0x2Bu  //!< High byte of magnetic sensor Y-axis output register address                 (Read only)
#define OUT_Z_L_M_REG_ADDRESS      0x2Cu  //!< Low byte of magnetic sensor Z-axis output register address                  (Read only)
#define OUT_Z_H_M_REG_ADDRESS      0x2Du  //!< High byte of magnetic sensor Z-axis output register address                 (Read only)
#define TEMP_L_M_REG_ADDRESS       0x2Eu  //!< Low byte of temperature output register address                             (Read only)
#define TEMP_H_M_REG_ADDRESS       0x2Fu  //!< High byte of temperature output register address                            (Read only)
#define INT_CFG_M_REG_ADDRESS      0x30u  //!< Magnetic sensor interrupt configuration register address                    (Read/Write)
#define INT_SRC_M_REG_ADDRESS      0x31u  //!< Magnetic sensor interrupt generator status register address                 (Read only)
#define INT_THS_L_M_REG_ADDRESS    0x32u  //!< Low byte of magnetic sensor interrupt generator threshold register address  (Read only)
#define INT_THS_H_M_REG_ADDRESS    0x33u  //!< High byte of magnetic sensor interrupt generator threshold register address (Read only)

// Manufacturer IDs
#define ACC_MANUFACTURER_ID        0x41u  //!< Accelerometer manufacturer ID
#define MAG_MANUFACTURER_ID        0x3Du  //!< Magnetic sensor manufacturer ID

// CTRL_REG1_A bits position
#define BDU_BIT_POS                   3u  //!< Position of bit BDU
#define ODR_BIT_POS                   4u  //!< Position of LSB bit ODR

#define SENSITIVITY_ACC           0.061f  //!< Linear acceleration sensitivity @FS = ±2 g in [LSB/mg]

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Acc_OutputDataRate_PowerDown,
  E_Acc_OutputDataRate_10Hz,
  E_Acc_OutputDataRate_50Hz,
  E_Acc_OutputDataRate_100Hz,
  E_Acc_OutputDataRate_200Hz,
  E_Acc_OutputDataRate_400Hz,
  E_Acc_OutputDataRate_800Hz
};
typedef uint8_t T_Acc_OutputDataRate;  //!< Accelerometer output data rate

enum
{
  E_Acc_BlockDataUpdate_Continuous,
  E_Acc_BlockDataUpdate_Ready
};
typedef uint8_t T_Acc_BlockDataUpdate;  //!< Accelerometer block data update

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "lsm303c";

static T_Acceleration Acceleration;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the accelerometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateBlockDataUpdate(T_Acc_BlockDataUpdate update);

//! \brief     Update the accelerometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateOutputDataRate(T_Acc_OutputDataRate rate);

//! \brief     Read the acceleration
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadAcceleration.svg
static void ReadAcceleration(void);

//! \brief     Read the accelerometer manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadAccManufacturerId.svg
static void ReadAccManufacturerId(uint8_t* data);

//! \brief     Read the magnetic sensor manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html ReadMagManufacturerId.svg
static void ReadMagManufacturerId(uint8_t* data);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void LSM303C_Init(void)
{
  UpdateBlockDataUpdate(E_Acc_BlockDataUpdate_Ready);
  UpdateOutputDataRate(E_Acc_OutputDataRate_100Hz);
}

//_____________________________________________________________________________

void LSM303C_CheckManufacturerId(void)
{
  uint8_t id = 0x00;

  ReadAccManufacturerId(&id);

  if (id != ACC_MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid accelerometer manufacturer ID: %d", id);
  }

  ReadMagManufacturerId(&id);

  if (id != MAG_MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid magnetic sensor manufacturer ID: %d", id);
  }
}

//_____________________________________________________________________________

T_Acceleration* LSM303C_GetAcceleration(void)
{
  ReadAcceleration();

  // TODO Acceleration.X * SENSITIVITY_ACC, Acceleration.Y * SENSITIVITY_ACC, Acceleration.Z * SENSITIVITY_ACC

  return &Acceleration;
}

//_____________________________________________________________________________

static void UpdateBlockDataUpdate(T_Acc_BlockDataUpdate update)
{
  uint8_t data = 0x00u;

  if (update <= E_Acc_BlockDataUpdate_Ready)
  {
    I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);

    data |= (update << BDU_BIT_POS);

    I2C_WriteToAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid block data update: %d", update);
  }
}

//_____________________________________________________________________________

static void UpdateOutputDataRate(T_Acc_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Acc_OutputDataRate_800Hz)
  {
    I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);

	data |= (rate << ODR_BIT_POS);

	I2C_WriteToAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void ReadAcceleration(void)
{
  uint8_t acceleration[6u];

  I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, OUT_X_L_A_REG_ADDRESS, acceleration, 6u);

  Acceleration.X = (uint16_t)((uint16_t)acceleration[1u] << 8u) | acceleration[0u];
  Acceleration.Y = (uint16_t)((uint16_t)acceleration[3u] << 8u) | acceleration[2u];
  Acceleration.Z = (uint16_t)((uint16_t)acceleration[5u] << 8u) | acceleration[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", Acceleration.X,  Acceleration.Y,  Acceleration.Z);
}

//_____________________________________________________________________________

static void ReadAccManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, WHO_AM_I_A_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadMagManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, WHO_AM_I_M_REG_ADDRESS, data, 1u);
}
