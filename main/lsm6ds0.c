//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    lsm6ds0.c
//! \brief   This module provides the useful functions to use the LSM6DS0 device
//!          (3D accelerometer and 3D gyroscope)
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "esp_log.h"

#include "lsm6ds0.h"

#include "i2c.h"
#include "imu_common.h"
#include "gyroscope.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SA0_PIN_STATE                           1u  //!< ADDR pin state used to set the slave address
#define LSM6DS0_ADDRESS                    0x6Au  //!< Device address

#define SLAVE_ADDRESS                 (LSM6DS0_ADDRESS | SA0_PIN_STATE)  //!< Slave address

// Registers addresses
#define FUNC_CFG_ACCESS_REG_ADDRESS          0x01u  //!< Enable embedded functions register address                             (Read/Write)
#define SENSOR_SYNC_TIME_FRAME_REG_ADDRESS   0x04u  //!< Sensor synchronization time frame register address                     (Read/Write)
#define FIFO_CTRL1_REG_ADDRESS               0x07u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL2_REG_ADDRESS               0x08u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL3_REG_ADDRESS               0x09u  //!< FIFO control register address                                          (Read/Write)
#define FIFO_CTRL4_REG_ADDRESS               0x0Au  //!< FIFO control register address                                          (Read/Write)
#define COUNTER_BDR_REG1_ADDRESS             0x0Bu  //!< Counter batch data rate register 1                                          (Read/Write)
#define COUNTER_BDR_REG2_ADDRESS         	 0x0Cu  //!< Counter batch data rate register 2 								(Read/Write)
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
#define FIFO_STATUS1_REG_ADDRESS             0x3Au  //!< FIFO status control register 1 address                                 (Read only)
#define FIFO_STATUS2_REG_ADDRESS             0x3Bu  //!< FIFO status control register 2 address                                 (Read only)
#define FIFO_STATUS3_REG_ADDRESS             0x3Cu  //!< FIFO status control register 3 address                                 (Read only)
#define FIFO_STATUS4_REG_ADDRESS             0x3Du  //!< FIFO status control register 4 address                                 (Read only)
#define FIFO_DATA_OUT_L_REG_ADDRESS          0x79u  //!< Low byte of FIFO data output register address                          (Read only)
#define FIFO_DATA_OUT_H_REG_ADDRESS          0x7Au  //!< High byte of FIFO data output register address                         (Read only)
#define TAP_CFG0_REG_ADDRESS				 0x56u	//!< Tap recognition configuration register address                         (Read/Write)
#define TAP_CFG1_REG_ADDRESS				 0x57u	//!< Tap recognition configuration register address                         (Read/Write)
#define TAP_CFG2_REG_ADDRESS                 0x58u  //!< Tap recognition configuration register address                         (Read/Write)
#define TAP_THS_6D_REG_ADDRESS               0x59u  //!< Portrait/landscape position and tap threshold register address         (Read/Write)
#define INT_DUR2_REG_ADDRESS                 0x5Au  //!< Tap recognition register address                                       (Read/Write)
#define FREE_FALL_REG_ADDRESS                0x5Du  //!< Free-Fall register address                                             (Read/Write)
#define MD1_CFG_REG_ADDRESS                  0x5Eu  //!< Routing on INT1 register address                                       (Read/Write)
#define MD2_CFG_REG_ADDRESS                  0x5Fu  //!< Routing on INT2 register address                                       (Read/Write)

// Manufacturer ID
#define MANUFACTURER_ID                      0x6Au  //!< Manufacturer ID

// Register bits mask
// CTRL1_XL bits mask
#define ACC_ODR_XL_BIT_MASK                  0x0Fu  //!< Mask of bit ODR_XL

// CTRL2_G bits mask
#define GYR_ODR_G_BIT_MASK                   0x0Fu  //!< Mask of bit ODR_G
#define GYR_FS_G_BIT_MASK                    0xF3u  //!< Mask of bit FS_G

// CTRL10_C bits mask
#define GYR_EN_G_BIT_MASK                    0x07u  //!< Mask of bit EN_G (+ 2bits MSB)

// TAP_CFG bits mask
#define ACC_TAP_EN_BIT_MASK                  0xF1u  //!< Mask of bit TAP_x_EN

// TAP_THS_6D bits mask
#define TAP_THS_BIT_MASK                     0xE0u  //!< Mask of bit TAP_THS

// FIFO_CTRL5 bits mask
#define FIFO_MODE_BIT_MASK                   0xF0u  //!< Mask of bit FIFO_MODE
#define BDR_GY_FIFO_BIT_MASK                 0x0Fu  //!< Mask of bit BDR_GY

// Register bits position
// CTRL1_XL bits position
#define ACC_ODR_XL_BIT_POS                      4u  //!< Position of LSB bit ODR_XL

// CTRL2_G bits position
#define GYR_ODR_G_BIT_POS                       4u  //!< Position of LSB bit ODR_G
#define GYR_FS_G_BIT_POS                        2u  //!< Position of LSB bit FS_G

// CTRL10_C bits position
#define GYR_EN_G_BIT_POS                        3u  //!< Position of LSB bit of EN_G

// TAP_CFG bits position
#define ACC_TAP_EN_BIT_POS                      1u  //!< Position of LSB bit TAP_x_EN

// FIFO_CTRL3 bits position
#define DEC_FIFO_GYRO_BIT_POS                   6u  //!< Position of LSB bit DEC_FIFO_GYRO

// FIFO_CTRL5 bits position
#define ODR_FIFO_BIT_POS                        4u  //!< Position of LSB bit ODR_FIFO

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
  E_Acc_Tap_DisableAll = 0x00,
  E_Acc_Tap_EnableZ    = 0x01,
  E_Acc_Tap_EnableY    = 0x02,
  E_Acc_Tap_EnableYZ   = 0x03,
  E_Acc_Tap_EnableX    = 0x04,
  E_Acc_Tap_EnableXZ   = 0x05,
  E_Acc_Tap_EnableXY   = 0x06,
  E_Acc_Tap_EnableAll  = 0x07
};
typedef uint8_t T_Acc_Tap;  //!< Accelerometer tap recognition

enum
{
  E_Acc_FreeFallThreshold_156g,
  E_Acc_FreeFallThreshold_219g,
  E_Acc_FreeFallThreshold_250g,
  E_Acc_FreeFallThreshold_312g,
  E_Acc_FreeFallThreshold_344g,
  E_Acc_FreeFallThreshold_406g,
  E_Acc_FreeFallThreshold_469g,
  E_Acc_FreeFallThreshold_500g
};
typedef uint8_t T_Acc_FreeFallThreshold;  //!< Accelerometer tap recognition

enum
{
  E_Acc_TapThreshold_Low     = 0x01,
  E_Acc_TapThreshold_MidLow  = 0x08,
  E_Acc_TapThreshold_Mid     = 0x10,
  E_Acc_TapThreshold_MidHigh = 0x18,
  E_Acc_TapThreshold_High    = 0x1F
};
typedef uint8_t T_Acc_TapThreshold;  //!< Accelerometer tap threshold

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
  E_Gyro_OutputDataRate_1660Hz,
  E_Gyro_OutputDataRate_3330Hz,
  E_Gyro_OutputDataRate_6660Hz
};
typedef uint8_t T_Gyro_OutputDataRate;  //!< Gyroscope output data rate

enum
{
  E_Gyro_FullScale_250dps,
  E_Gyro_FullScale_500dps,
  E_Gyro_FullScale_1000dps,
  E_Gyro_FullScale_2000dps
};
typedef uint8_t T_Gyro_FullScale;  //!< Gyroscope output full-scale

enum
{
  E_Gyro_Disable_All,
  E_Gyro_Enable_X,
  E_Gyro_Enable_Y,
  E_Gyro_Enable_XY,
  E_Gyro_Enable_Z,
  E_Gyro_Enable_ZX,
  E_Gyro_Enable_ZY,
  E_Gyro_Enable_All
};
typedef uint8_t T_Gyro_EnableAxis;  //!< Gyroscope enable/disable axis

enum
{
  E_Decimation_None,
  E_Decimation_1,
  E_Decimation_2,
  E_Decimation_3,
  E_Decimation_4,
  E_Decimation_8,
  E_Decimation_16,
  E_Decimation_32
};
typedef uint8_t T_Decimation;  //!< FIFO decimation setting

enum
{
  E_FifoMode_Bypass             = 0x00,  // FIFO disabled
  E_FifoMode_Fifo               = 0x01,  // Stops collecting data when FIFO is full
  E_FifoMode_ContinuousToFifo   = 0x03,  // Continuous mode until trigger is deasserted, then FIFO mode
  E_FifoMode_BypassToContinuous = 0x04,  // Bypass mode until trigger is deasserted, then Continuous mode
  E_FifoMode_Continuous         = 0x06   // If the FIFO is full, the new sample overwrites the older one
};
typedef uint8_t T_FifoMode;  //!< FIFO mode selection

enum
{
  E_Fifo_OutputDataRate_Disable,
  E_Fifo_OutputDataRate_12_5Hz,
  E_Fifo_OutputDataRate_26Hz,
  E_Fifo_OutputDataRate_52Hz,
  E_Fifo_OutputDataRate_104Hz,
  E_Fifo_OutputDataRate_208Hz,
  E_Fifo_OutputDataRate_416Hz,
  E_Fifo_OutputDataRate_833Hz,
  E_Fifo_OutputDataRate_1660Hz,
  E_Fifo_OutputDataRate_3330Hz,
  E_Fifo_OutputDataRate_6660Hz
};
typedef uint8_t T_Fifo_OutputDataRate;  //!< FIFO output data rate

enum
{
  E_Interrupt_INT1,
  E_Interrupt_INT2
};
typedef uint8_t T_Interrupt;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "LSM6DS0";

extern int16_t GyroBuffer[3][GYRO_BUFFER_SIZE];

extern int32_t Mul;
extern int32_t Div;
extern int32_t Offset;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the accelerometer output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateAccOutputDataRate(T_Acc_OutputDataRate rate);

//! \brief     Configure the INT1 interrupt
//! \pre       None
//! \param     None
//! \return    None
static void ConfigureINT1(uint8_t interrupt);

//! \brief     Configure the INT2 interrupt
//! \pre       None
//! \param     None
//! \return    None
static void ConfigureINT2(uint8_t interrupt);

//! \brief     Configure the accelerometer tap recognition
//! \pre       None
//! \param     None
//! \return    None
static void ConfigureTap(T_Acc_Tap config);

//! \brief     Update the accelerometer tap threshold
//! \pre       None
//! \param     None
//! \return    None
static void UpdateTapThreshold(T_Acc_TapThreshold threshold);

//! \brief     Configure the free-fall detection
//! \pre       None
//! \param     None
//! \return    None
static void UpdateFreeFall(T_Acc_FreeFallThreshold threshold, uint8_t duration);

//! \brief     Set the integrator factors used to calculate the angle
//! \pre       None
//! \param     None
//! \return    None
static void SetIntegratorFactors(T_Gyro_OutputDataRate rate);

//! \brief     Update the gyroscope output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateGyroOutputDataRate(T_Gyro_OutputDataRate rate);

//! \brief     Update the gyroscope full-scale
//! \pre       None
//! \param     None
//! \return    None
static void UpdateGyroFullScale(T_Gyro_FullScale scale);

//! \brief     Update the FIFO mode
//! \pre       None
//! \param     None
//! \return    None
static void UpdateFifoMode(T_FifoMode mode);

//! \brief     Get the FIFO mode
//! \pre       None
//! \param     None
//! \return    None
static T_FifoMode GetFifoMode(void);

//! \brief     Update the FIFO threshold
//! \pre       None
//! \param     None
//! \return    None
static void UpdateFifoThreshold(uint16_t threshold);

//! \brief     Update the FIFO output data rate
//! \pre       None
//! \param     None
//! \return    None
static void UpdateFifoOutputDataRate(T_Fifo_OutputDataRate rate);

static void EnableGyroDataReadyInterrupt(T_Interrupt interrupt);

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

void LSM6DS0_InitAccelerometer(void)
{
  for (uint8_t index = 0u; index < 2u; index++)
  {
    Gpio_ConfigurePin(&PinConfig[index]);
  }

  UpdateAccOutputDataRate(E_Acc_OutputDataRate_416Hz);

  ConfigureINT1(0x10); // Enable free-fall interrupt on INT1
  ConfigureINT2(0x40); // Enable single-tap interrupt on INT2

  ConfigureTap(E_Acc_Tap_EnableAll);
  UpdateTapThreshold(E_Acc_TapThreshold_Mid);

  UpdateFreeFall(E_Acc_FreeFallThreshold_312g, 6);

  ESP_LOGI(Tag, "LSM6DS0 accelerometer is initialized");
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

static void ConfigureINT1(uint8_t interrupt)
{
  uint8_t data = interrupt;

  I2C_WriteToAddress(SLAVE_ADDRESS, MD1_CFG_REG_ADDRESS, &data, 1u);
}

//_____________________________________________________________________________

static void ConfigureINT2(uint8_t interrupt)
{
  uint8_t data = interrupt;

  I2C_WriteToAddress(SLAVE_ADDRESS, MD2_CFG_REG_ADDRESS, &data, 1u);
}

//_____________________________________________________________________________

static void ConfigureTap(T_Acc_Tap config)
{
  uint8_t data = 0x00u;

  if (config <= E_Acc_Tap_EnableAll)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, TAP_CFG0_REG_ADDRESS, &data, 1u);

    data &= ACC_TAP_EN_BIT_MASK;
    data |= (config << ACC_TAP_EN_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, TAP_CFG0_REG_ADDRESS, &data, 1u);

    // TODO check if needed
    data = 0x06;
    I2C_WriteToAddress(SLAVE_ADDRESS, INT_DUR2_REG_ADDRESS, &data, 1u);
  }
}

//_____________________________________________________________________________

static void UpdateTapThreshold(T_Acc_TapThreshold threshold)
{
  uint8_t data = 0x00u;

  if (threshold <= E_Acc_TapThreshold_High)
  {
	// X axis threshold
	I2C_ReadFromAddress(SLAVE_ADDRESS, TAP_CFG1_REG_ADDRESS, &data, 1u);
	data &= TAP_THS_BIT_MASK;
	data |= threshold;
	I2C_WriteToAddress(SLAVE_ADDRESS, TAP_CFG1_REG_ADDRESS, &data, 1u);

	// Y axis threshold
	I2C_ReadFromAddress(SLAVE_ADDRESS, TAP_CFG2_REG_ADDRESS, &data, 1u);
	data &= TAP_THS_BIT_MASK;
	data |= threshold;
	I2C_WriteToAddress(SLAVE_ADDRESS, TAP_CFG2_REG_ADDRESS, &data, 1u);

	// Z axis threshold
    I2C_ReadFromAddress(SLAVE_ADDRESS, TAP_THS_6D_REG_ADDRESS, &data, 1u);
    data &= TAP_THS_BIT_MASK;
    data |= threshold;
    I2C_WriteToAddress(SLAVE_ADDRESS, TAP_THS_6D_REG_ADDRESS, &data, 1u);

  }
}

//_____________________________________________________________________________

static void UpdateFreeFall(T_Acc_FreeFallThreshold threshold, uint8_t duration)
{
  uint8_t data = 0x00u;

  if (threshold <= E_Acc_FreeFallThreshold_500g)
  {
    if (duration > 31)
    {
      duration = 31;
    }

    data = (duration << 3);
    data |= threshold;

    I2C_WriteToAddress(SLAVE_ADDRESS, FREE_FALL_REG_ADDRESS, &data, 1u);
  }
}

//_____________________________________________________________________________

void LSM6DS0_ReadAcceleration(T_Axis* acceleration)
{
  uint8_t acc[6u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, OUTX_L_XL_REG_ADDRESS, acc, 6u);

  acceleration->X = (int16_t)((uint16_t)acc[1u] << 8u) | acc[0u];
  acceleration->Y = (int16_t)((uint16_t)acc[3u] << 8u) | acc[2u];
  acceleration->Z = (int16_t)((uint16_t)acc[5u] << 8u) | acc[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", acceleration->X, acceleration->Y, acceleration->Z);
}

//_____________________________________________________________________________

void LSM6DS0_ReadTapSource(uint8_t* source)
{
  uint8_t src;

  I2C_ReadFromAddress(SLAVE_ADDRESS, TAP_SRC_REG_ADDRESS, &src, 1u);

  *source = src;
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

void LSM6DS0_InitGyroscope(int16_t offset)
{
	uint8_t data = 0x00u;

	// Set BDU flag
	I2C_ReadFromAddress(SLAVE_ADDRESS, CTRL3_C_REG_ADDRESS, &data, 1u);
	data |= 0x40;
	I2C_WriteToAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);

	UpdateFifoMode(E_FifoMode_Bypass); // Set bypass mode during FIFO configuration
	UpdateFifoOutputDataRate(E_Fifo_OutputDataRate_104Hz);
	//UpdateFifoThreshold(1500);

	UpdateGyroOutputDataRate(E_Gyro_OutputDataRate_104Hz);
	UpdateGyroFullScale(E_Gyro_FullScale_500dps);

	Offset = offset;

	ESP_LOGI(Tag, "LSM6DS0 gyroscope is initialized with offset = %d", Offset);
}

//_____________________________________________________________________________

void LSM6DS0_SetOffset(int32_t offset)
{
  Offset = offset;
  ESP_LOGI(Tag, "Set Offset: %d", Offset);
}

//_____________________________________________________________________________

static void UpdateGyroOutputDataRate(T_Gyro_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Gyro_OutputDataRate_6660Hz)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);

    data &= GYR_ODR_G_BIT_MASK;
    data |= (rate << GYR_ODR_G_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);
    SetIntegratorFactors(rate);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid gyroscope output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void UpdateGyroFullScale(T_Gyro_FullScale scale)
{
  uint8_t data = 0x00u;

  if (scale <= E_Gyro_FullScale_2000dps)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);

    data &= GYR_FS_G_BIT_MASK;
    data |= (scale << GYR_FS_G_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, CTRL2_G_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid gyroscope full scale: %d", scale);
  }
}


//_____________________________________________________________________________

static void UpdateFifoMode(T_FifoMode mode)
{
  uint8_t data = 0x00u;

  if (mode <= E_FifoMode_Continuous)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_CTRL4_REG_ADDRESS, &data, 1u);

    data &= FIFO_MODE_BIT_MASK;
    data |= mode;

    I2C_WriteToAddress(SLAVE_ADDRESS, FIFO_CTRL4_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid FIFO mode: %d", mode);
  }
}

//_____________________________________________________________________________

static T_FifoMode GetFifoMode(void)
{
  uint8_t data = 0x00u;

  I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_CTRL4_REG_ADDRESS, &data, 1u);

  return (T_FifoMode)(data & 0x07);
}

//_____________________________________________________________________________


static void UpdateFifoThreshold(uint16_t threshold)
{
  uint8_t data = 0x00u;
  uint8_t limit[2];

  if (threshold < 512)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_CTRL2_REG_ADDRESS, &data, 1u);
    data &= 0xD6;
    data |= ((threshold & 0x0100) >> 8);

    limit[0] = (threshold & 0x00FF);
    limit[1] = data;

    I2C_WriteToAddress(SLAVE_ADDRESS, FIFO_CTRL1_REG_ADDRESS, limit, 2u);
  }
}

//_____________________________________________________________________________

static void UpdateFifoOutputDataRate(T_Fifo_OutputDataRate rate)
{
  uint8_t data = 0x00u;

  if (rate <= E_Fifo_OutputDataRate_6660Hz)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_CTRL3_REG_ADDRESS, &data, 1u);

    data &= BDR_GY_FIFO_BIT_MASK;
    data |= (rate << ODR_FIFO_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, FIFO_CTRL3_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid gyroscope output data rate: %d", rate);
  }
}

//_____________________________________________________________________________

static void EnableGyroDataReadyInterrupt(T_Interrupt interrupt)
{
  uint8_t data = 0x02u;

  switch (interrupt)
  {
    case E_Interrupt_INT1:
      I2C_WriteToAddress(SLAVE_ADDRESS, INT1_CTRL_REG_ADDRESS, &data, 1u);
      break;

    case E_Interrupt_INT2:
      I2C_WriteToAddress(SLAVE_ADDRESS, INT2_CTRL_REG_ADDRESS, &data, 1u);
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

void LSM6DS0_ReadAngularVelocity(T_Axis* angularVelocity)
{
  uint8_t velocity[6u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, OUTX_L_G_REG_ADDRESS, velocity, 6u);

  angularVelocity->X = (int16_t)((uint16_t)velocity[1u] << 8u) | velocity[0u];
  angularVelocity->Y = (int16_t)((uint16_t)velocity[3u] << 8u) | velocity[2u];
  angularVelocity->Z = (int16_t)((uint16_t)velocity[5u] << 8u) | velocity[4u];

  //ESP_LOGI(Tag, "X: %d, Y: %d, Z: %d", angularVelocity->X, angularVelocity->Y, angularVelocity->Z);
}

//_____________________________________________________________________________

uint16_t LSM6DS0_ReadBufferedAngularPosition(void)
{
  uint8_t temp_data[7u] = {0u};
  uint16_t length = 0x00u;

  uint8_t i = 0u;
  uint8_t j = 0u;
  uint8_t k = 0u;

  uint16_t numSamples = 0u;

  uint8_t len[2] = {0u, 0u};
  uint8_t pat[2] = {0u, 0u};

  static bool first = false;

  if (!first)
  {
    UpdateFifoMode(E_FifoMode_Continuous);
    first = true;
  }

  I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_STATUS1_REG_ADDRESS, len, 2u);

  length = (uint16_t)((uint16_t)(len[1] & 0x03) << 8) | len[0];
  //ESP_LOGD(Tag, "length=%d", length);

  if (length > 0u)
  {
    numSamples = length; // Fifo format: TAG + 6 bytes for the XYZ samples.
    //ESP_LOGD(Tag, "numSamples=%d", numSamples);

    if (numSamples >= 3*GYRO_BUFFER_SIZE) {
    	numSamples = 3*GYRO_BUFFER_SIZE;
    }

    for (uint16_t index = 0u; index < numSamples; index++)
    {
    	I2C_ReadFromAddress(SLAVE_ADDRESS, FIFO_DATA_OUT_L_REG_ADDRESS, temp_data, 7u);
    	GyroBuffer[0][i] = (int16_t)((uint16_t)temp_data[1u] << 8u) | temp_data[0u];
    	GyroBuffer[1][i] = (int16_t)((uint16_t)temp_data[3u] << 8u) | temp_data[2u];
    	GyroBuffer[2][i] = (int16_t)((uint16_t)temp_data[5u] << 8u) | temp_data[4u];
    	i++;
    }
  }

  return (numSamples);  // Number of XYZ samples
}

//_____________________________________________________________________________

static void SetIntegratorFactors(T_Gyro_OutputDataRate rate)
{
  switch (rate)
  {
    case E_Gyro_OutputDataRate_PowerDown:
      // Do nothing
      break;

    case E_Gyro_OutputDataRate_12_5Hz:
      Mul = 7 * 512; //7 * 512
      Div = 1125 * 12.5;  // 1125 * 12.5
      break;

    case E_Gyro_OutputDataRate_26Hz:
      Mul = 7 * 256; //7 * 256
      Div = 1125 * 13;  // 1125 * 13
      break;

    case E_Gyro_OutputDataRate_52Hz:
      Mul = 7 * 128; //7 * 128
      Div = 1125 * 13;  // 1125 * 13
      break;

    case E_Gyro_OutputDataRate_104Hz:
      Mul = 7 * 64; //7 * 64
      Div = 1125 * 13;  // 1125 * 13
      break;

    case E_Gyro_OutputDataRate_208Hz:
      Mul = 7 * 32; //7 * 32
      Div = 1125 * 13;  // 1125 * 13
      break;

    case E_Gyro_OutputDataRate_416Hz:
      Mul = 7 * 16; //7 * 16
      Div = 1125 * 13;  // 1125 * 13
      break;

    case E_Gyro_OutputDataRate_833Hz:
      Mul = 7 * 512; //7 * 512
      Div = 1125 * 833;  // 1125 * 833
      break;

    case E_Gyro_OutputDataRate_1660Hz:
      Mul = 7 * 128; //7 * 128
      Div =  1125 * 415;  // 1125 * 415
      break;
//TODO add new clock calculation for 3330 and 6660
    default:
      // Do nothing
      break;
  }
}

//-----------------------------------------------------------------------------
// Common
//-----------------------------------------------------------------------------

T_Error LSM6DS0_CheckManufacturerId(void)
{
  uint8_t id = 0x00;
  T_Error err = E_Error_None;

  ReadManufacturerId(&id);

  if (id != MANUFACTURER_ID)
  {
    err = E_Error_Acc_InvalidID;
    ESP_LOGE(Tag, "Invalid manufacturer ID: %d", id);
  }

  return err;
}

//_____________________________________________________________________________

static void ReadManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, WHO_AM_I_REG_ADDRESS, data, 1u);
}
