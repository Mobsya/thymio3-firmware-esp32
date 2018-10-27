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

// Slave address
#define ACC_SLAVE_ADDRESS          0x1Du  //!< Accelerometer slave address
#define MAG_SLAVE_ADDRESS          0x1Eu  //!< Magnetometer slave address

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

// Magnetometer Registers addresses
#define WHO_AM_I_M_REG_ADDRESS     0x0Fu  //!< Magnetometer Who_AM_I register address                                   (Read only)
#define CTRL_REG1_M_REG_ADDRESS    0x20u  //!< Magnetometer control 1 register address                                  (Read/Write)
#define CTRL_REG2_M_REG_ADDRESS    0x21u  //!< Magnetometer control 2 register address                                  (Read/Write)
#define CTRL_REG3_M_REG_ADDRESS    0x22u  //!< Magnetometer control 3 register address                                  (Read/Write)
#define CTRL_REG4_M_REG_ADDRESS    0x23u  //!< Magnetometer control 4 register address                                  (Read/Write)
#define CTRL_REG5_M_REG_ADDRESS    0x24u  //!< Magnetometer control 5 register address                                  (Read/Write)
#define STATUS_REG_M_REG_ADDRESS   0x27u  //!< Magnetometer status register address                                     (Read/Write)
#define OUT_X_L_M_REG_ADDRESS      0x28u  //!< Low byte of magnetometer X-axis output register address                  (Read only)
#define OUT_X_H_M_REG_ADDRESS      0x29u  //!< High byte of magnetometer X-axis output register address                 (Read only)
#define OUT_Y_L_M_REG_ADDRESS      0x2Au  //!< Low byte of magnetometer Y-axis output register address                  (Read only)
#define OUT_Y_H_M_REG_ADDRESS      0x2Bu  //!< High byte of magnetometer Y-axis output register address                 (Read only)
#define OUT_Z_L_M_REG_ADDRESS      0x2Cu  //!< Low byte of magnetometer Z-axis output register address                  (Read only)
#define OUT_Z_H_M_REG_ADDRESS      0x2Du  //!< High byte of magnetometer Z-axis output register address                 (Read only)
#define TEMP_L_M_REG_ADDRESS       0x2Eu  //!< Low byte of temperature output register address                             (Read only)
#define TEMP_H_M_REG_ADDRESS       0x2Fu  //!< High byte of temperature output register address                            (Read only)
#define INT_CFG_M_REG_ADDRESS      0x30u  //!< Magnetometer interrupt configuration register address                    (Read/Write)
#define INT_SRC_M_REG_ADDRESS      0x31u  //!< Magnetometer interrupt generator status register address                 (Read only)
#define INT_THS_L_M_REG_ADDRESS    0x32u  //!< Low byte of Magnetometer interrupt generator threshold register address  (Read only)
#define INT_THS_H_M_REG_ADDRESS    0x33u  //!< High byte of Magnetometer interrupt generator threshold register address (Read only)

// Manufacturer IDs
#define ACC_MANUFACTURER_ID        0x41u  //!< Accelerometer manufacturer ID
#define MAG_MANUFACTURER_ID        0x3Du  //!< Magnetometer manufacturer ID

// Register bits mask
// CTRL_REG1_A bits mask
#define ACC_BDU_BIT_MASK           0xF7u  //!< Mask of bit BDU
#define ACC_ODR_BIT_MASK           0x8Fu  //!< Mask of bit ODR

// CTRL_REG1_M bits mask
#define MAG_DO_BIT_MASK            0xE3u  //!< Mask of bit DO
#define MAG_OM_BIT_MASK            0x9Fu  //!< Mask of bit OM

// CTRL_REG2_M bits mask
#define MAG_FS_BIT_MASK            0x9Fu  //!< Mask of bit FS

// CTRL_REG3_M bits mask
#define MAG_MD_BIT_MASK            0xFCu  //!< Mask of bit MD

// CTRL_REG4_M bits mask
#define MAG_OMZ_BIT_MASK           0xF3u  //!< Mask of bit OMZ

// CTRL_REG5_M bits mask
#define MAG_BDU_BIT_MASK           0xBFu  //!< Mask of bit BDU

// Register bits position
// CTRL_REG1_A bits position
#define ACC_BDU_BIT_POS               3u  //!< Position of bit BDU
#define ACC_ODR_BIT_POS               4u  //!< Position of LSB bit ODR

// CTRL_REG1_M bits position
#define MAG_DO_BIT_POS                2u  //!< Position of LSB bit DO
#define MAG_OM_BIT_POS                5u  //!< Position of LSB bit OM

// CTRL_REG2_M bits position
#define MAG_FS_BIT_POS                5u  //!< Position of LSB bit FS

// CTRL_REG4_M bits position
#define MAG_OMZ_BIT_POS               2u  //!< Position of LSB bit OMZ

// CTRL_REG5_M bits position
#define MAG_BDU_BIT_POS               6u  //!< Position of bit BDU

#define SENSITIVITY_ACC           0.061f  //!< Linear acceleration sensitivity @FS = ±2 g in [LSB/mg]

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_Acc_BlockDataUpdate_Continuous,
  E_Acc_BlockDataUpdate_Ready
};
typedef uint8_t T_Acc_BlockDataUpdate;  //!< Accelerometer block data update

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
  E_Mag_BlockDataUpdate_Continuous,
  E_Mag_BlockDataUpdate_Ready
};
typedef uint8_t T_Mag_BlockDataUpdate;  //!< Magnetometer block data update

enum
{
  E_Mag_OutputDataRate_0_625Hz,
  E_Mag_OutputDataRate_1_25Hz,
  E_Mag_OutputDataRate_2_5Hz,
  E_Mag_OutputDataRate_5Hz,
  E_Mag_OutputDataRate_10Hz,
  E_Mag_OutputDataRate_20Hz,
  E_Mag_OutputDataRate_40Hz,
  E_Mag_OutputDataRate_80Hz
};
typedef uint8_t T_Mag_OutputDataRate;  //!< Magnetometer output data rate

enum
{
  E_Mag_OperatingMode_ContinuousConv,
  E_Mag_OperatingMode_SingleConv,
  E_Mag_OperatingMode_PowerDown
};
typedef uint8_t T_Mag_OperatingMode;

enum
{
  E_Mag_OperativeMode_LowPower,
  E_Mag_OperativeMode_MediumPerf,
  E_Mag_OperativeMode_HighPerf,
  E_Mag_OperativeMode_UltraHighPerf
};
typedef uint8_t T_Mag_OperativeMode;

enum
{
  E_Mag_FullScaleConfig_NotUsed,
  E_Mag_FullScaleConfig_16gauss = 3
};
typedef uint8_t T_Mag_FullScaleConfig;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "lsm303c";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the accelerometer block data update
//! \pre       None
//! \param     None
//! \return    None
static void UpdateAccBlockDataUpdate(T_Acc_BlockDataUpdate update);

//! \brief     Update the accelerometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateAccOutputDataRate(T_Acc_OutputDataRate rate);

//! \brief     Read the acceleration
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\lsm303c\ReadAcceleration.svg
static void ReadAcceleration(T_Acc_Axis* acceleration);

//! \brief     Read the accelerometer manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\lsm303c\ReadAccManufacturerId.svg
static void ReadAccManufacturerId(uint8_t* data);

//! \brief     Update the magnetometer block data update
//! \param     None
//! \return    None
static void UpdateMagBlockDataUpdate(T_Mag_BlockDataUpdate update);

//! \brief     Update the magnetometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateMagOutputDataRate(T_Mag_OutputDataRate rate);

//! \brief     Update the magnetometer system operating mode
//! \pre       None
//! \param     None
//! \return    None
static void UpdateMagSystemOperatingMode(T_Mag_OperatingMode mode);

//! \brief     Update the magnetometer X and Y axes operative mode
//! \pre       None
//! \param     None
//! \return    None
static void UpdateMagXYOperativeMode(T_Mag_OperativeMode mode);

//! \brief     Update the magnetometer X and Y axes operative mode
//! \pre       None
//! \param     None
//! \return    None
static void UpdateMagZOperativeMode(T_Mag_OperativeMode mode);

//! \brief     Update the magnetometer full scale configuration
//! \pre       None
//! \param     None
//! \return    None
static void UpdateMagFullScaleConfig(T_Mag_FullScaleConfig config);

//! \brief     Read the magnetic field
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\lsm303c\ReadMagneticField.svg TODO
static void ReadMagneticField(T_Mag_Axis* magneticField);

//! \brief     Read the magnetometer manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\lsm303c\ReadMagManufacturerId.svg
static void ReadMagManufacturerId(uint8_t* data);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Accelerometer
//-----------------------------------------------------------------------------

void LSM303C_InitAccelerometer(void)
{
  UpdateAccBlockDataUpdate(E_Acc_BlockDataUpdate_Ready);
  UpdateAccOutputDataRate(E_Acc_OutputDataRate_100Hz);
}

//_____________________________________________________________________________

void LSM303C_CheckAccManufacturerId(void)
{
  uint8_t id = 0x00;

  ReadAccManufacturerId(&id);

  if (id != ACC_MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid accelerometer manufacturer ID: %d", id);
  }
}

//_____________________________________________________________________________

void LSM303C_GetAcceleration(T_Acc_Axis* acceleration)
{
  ReadAcceleration(acceleration);

  // TODO Acceleration.X * SENSITIVITY_ACC, Acceleration.Y * SENSITIVITY_ACC, Acceleration.Z * SENSITIVITY_ACC
}

//_____________________________________________________________________________

static void UpdateAccBlockDataUpdate(T_Acc_BlockDataUpdate update)
{
  uint8_t data = 0x00u;

  if (update <= E_Acc_BlockDataUpdate_Ready)
  {
    I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);

    data &= ACC_BDU_BIT_MASK;
    data |= (update << ACC_BDU_BIT_POS);

    I2C_WriteToAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid accelerometer block data update: %d", update);
  }
}

//_____________________________________________________________________________

static void UpdateAccOutputDataRate(T_Acc_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Acc_OutputDataRate_800Hz)
  {
    I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);

    data &= ACC_ODR_BIT_MASK;
    data |= (rate << ACC_ODR_BIT_POS);

    I2C_WriteToAddress(ACC_SLAVE_ADDRESS, CTRL_REG1_A_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid accelerometer output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void ReadAcceleration(T_Acc_Axis* acceleration)
{
  uint8_t acc[6u];

  I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, OUT_X_L_A_REG_ADDRESS, acc, 6u);

  acceleration->X = (uint16_t)((uint16_t)acc[1u] << 8u) | acc[0u];
  acceleration->Y = (uint16_t)((uint16_t)acc[3u] << 8u) | acc[2u];
  acceleration->Z = (uint16_t)((uint16_t)acc[5u] << 8u) | acc[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", Acceleration.X,  Acceleration.Y,  Acceleration.Z);
}

//_____________________________________________________________________________

static void ReadAccManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(ACC_SLAVE_ADDRESS, WHO_AM_I_A_REG_ADDRESS, data, 1u);
}

//-----------------------------------------------------------------------------
// Magnetometer (compass)
//-----------------------------------------------------------------------------

void LSM303C_InitMagnetometer(void)
{
  UpdateMagBlockDataUpdate(E_Mag_BlockDataUpdate_Ready);
  UpdateMagOutputDataRate(E_Mag_OutputDataRate_40Hz);
  UpdateMagSystemOperatingMode(E_Mag_OperatingMode_ContinuousConv);
  UpdateMagXYOperativeMode(E_Mag_OperativeMode_HighPerf);
  UpdateMagZOperativeMode(E_Mag_OperativeMode_HighPerf);
  UpdateMagFullScaleConfig(E_Mag_FullScaleConfig_16gauss);
}

//_____________________________________________________________________________

void LSM303C_CheckMagManufacturerId(void)
{
  uint8_t id = 0x00;

  ReadMagManufacturerId(&id);

  if (id != MAG_MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid magnetometer manufacturer ID: %d", id);
  }
}

//_____________________________________________________________________________

void LSM303C_GetMagneticField(T_Mag_Axis* field)
{
  ReadMagneticField(field);

  // TODO MagneticField.X * SENSITIVITY_ACC, MagneticField.Y * SENSITIVITY_ACC, MagneticField.Z * SENSITIVITY_ACC
}

//_____________________________________________________________________________

static void UpdateMagBlockDataUpdate(T_Mag_BlockDataUpdate update)
{
  uint8_t data = 0x00u;

  if (update <= E_Mag_BlockDataUpdate_Ready)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG5_M_REG_ADDRESS, &data, 1u);

    data &= MAG_BDU_BIT_MASK;
    data |= (update << MAG_BDU_BIT_POS);

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG5_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer block data update: %d", update);
  }
}

//_____________________________________________________________________________

static void UpdateMagOutputDataRate(T_Mag_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Mag_OutputDataRate_80Hz)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG1_M_REG_ADDRESS, &data, 1u);

    data &= MAG_DO_BIT_MASK;
    data |= (rate << MAG_DO_BIT_POS);

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG1_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void UpdateMagSystemOperatingMode(T_Mag_OperatingMode mode)
{
  uint8_t data = 0x00u;

  if (mode <= E_Mag_OperatingMode_PowerDown)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG3_M_REG_ADDRESS, &data, 1u);

    data &= MAG_MD_BIT_MASK;
    data |= mode;

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG3_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer system operating mode: %d", mode);
  }
}

//_____________________________________________________________________________

static void UpdateMagXYOperativeMode(T_Mag_OperativeMode mode)
{
  uint8_t data = 0x00u;

  if (mode <= E_Mag_OperativeMode_UltraHighPerf)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG1_M_REG_ADDRESS, &data, 1u);

    data &= MAG_OM_BIT_MASK;
    data |= (mode << MAG_OM_BIT_POS);

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG1_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer X and Y axes operative mode: %d", mode);
  }
}

//_____________________________________________________________________________

static void UpdateMagZOperativeMode(T_Mag_OperativeMode mode)
{
  uint8_t data = 0x00u;

  if (mode <= E_Mag_OperativeMode_UltraHighPerf)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG4_M_REG_ADDRESS, &data, 1u);

    data &= MAG_OMZ_BIT_MASK;
    data |= (mode << MAG_OMZ_BIT_POS);

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG4_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer Z axes operative mode: %d", mode);
  }
}

//_____________________________________________________________________________

static void UpdateMagFullScaleConfig(T_Mag_FullScaleConfig config)
{
  uint8_t data = 0x00u;

  if (config <= E_Mag_FullScaleConfig_16gauss)
  {
    I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, CTRL_REG2_M_REG_ADDRESS, &data, 1u);

    data &= MAG_FS_BIT_MASK;
    data |= (config << MAG_FS_BIT_POS);

    I2C_WriteToAddress(MAG_SLAVE_ADDRESS, CTRL_REG2_M_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid magnetometer full scale configuration: %d", config);
  }
}

//_____________________________________________________________________________

static void ReadMagneticField(T_Mag_Axis* magneticField)
{
  uint8_t field[6u];

  I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, OUT_X_L_M_REG_ADDRESS, field, 6u);

  magneticField->X = (uint16_t)((uint16_t)field[1u] << 8u) | field[0u];
  magneticField->Y = (uint16_t)((uint16_t)field[3u] << 8u) | field[2u];
  magneticField->Z = (uint16_t)((uint16_t)field[5u] << 8u) | field[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", MagneticField.X,  MagneticField.Y,  MagneticField.Z);
}

//_____________________________________________________________________________

static void ReadMagManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(MAG_SLAVE_ADDRESS, WHO_AM_I_M_REG_ADDRESS, data, 1u);
}
