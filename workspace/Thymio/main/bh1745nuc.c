//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    bh1745nuc.c
//! \brief   This module provides the useful functions to use the color sensor BH1745NUC
//!
//! \author  Vincent Gonet
//!
//! \version $Id: color_sensor.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <esp_log.h>

#include "bh1745nuc.h"

#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ADDR_PIN_STATE                   1u  //!< ADDR pin state used to set the slave address
#define BH1745NUC_ADDRESS             0x38u  //!< Device address

#define SLAVE_ADDRESS                 (BH1745NUC_ADDRESS | ADDR_PIN_STATE)  //!< Slave address

// Registers addresses
#define SYSTEM_CONTROL_REG_ADDRESS    0x40u  //!< System control register address                (Read/Write)
#define MODE_CONTROL1_REG_ADDRESS     0x41u  //!< Function setting register address              (Read/Write)
#define MODE_CONTROL2_REG_ADDRESS     0x42u  //!< Function setting register address              (Read/Write)
#define MODE_CONTROL3_REG_ADDRESS     0x44u  //!< Function setting register address              (Read/Write)
#define RED_DATA_LSB_REG_ADDRESS      0x50u  //!< Low byte of RED register address               (Read only)
#define RED_DATA_MSB_REG_ADDRESS      0x51u  //!< High byte of RED register address              (Read only)
#define GREEN_DATA_LSB_REG_ADDRESS    0x52u  //!< Low byte of GREEN register address             (Read only)
#define GREEN_DATA_MSB_REG_ADDRESS    0x53u  //!< High byte of GREEN register address            (Read only)
#define BLUE_DATA_LSB_REG_ADDRESS     0x54u  //!< Low byte of BLUE register address              (Read only)
#define BLUE_DATA_MSB_REG_ADDRESS     0x55u  //!< High byte of BLUE register address             (Read only)
#define CLEAR_DATA_LSB_REG_ADDRESS    0x56u  //!< Low byte of CLEAR register address             (Read only)
#define CLEAR_DATA_MSB_REG_ADDRESS    0x57u  //!< High byte of CLEAR register address            (Read only)
#define DINT_DATA_LSB_REG_ADDRESS     0x58u  //!< Low byte of Internal Data register address     (Read only)
#define DINT_DATA_MSB_REG_ADDRESS     0x59u  //!< High byte of Internal Data register address    (Read only)
#define INTERRUPT_REG_ADDRESS         0x60u  //!< Interrupt setting register address             (Read/Write)
#define PERSISTENCE_REG_ADDRESS       0x61u  //!< Persistence setting register address           (Read/Write)
#define TH_LSB_REG_ADDRESS            0x62u  //!< Low byte of higher threshold register address  (Read/Write)
#define TH_MSB_REG_ADDRESS            0x63u  //!< High byte of higher threshold register address (Read/Write)
#define TL_LSB_REG_ADDRESS            0x64u  //!< Low byte of lower threshold register address   (Read/Write)
#define TL_MSB_REG_ADDRESS            0x65u  //!< High byte of lower threshold register address  (Read/Write)
#define MANUFACTURER_ID_REG_ADDRESS   0x92u  //!< Manufacturer ID register address               (Read only)

#define MANUFACTURER_ID               0xE0u  //!< Manufacturer ID

// Register bits mask
// MODE_CONTROL2 bits mask
#define ADC_GAIN_BIT_MASK             0xFCu  //!< Mask of bit ADC_GAIN

// INTERRUPT bits mask
#define INT_SOURCE_BIT_MASK           0xF3u  //!< Mask of bit INT_SOURCE

// MODE_CONTROL2 bits position
#define RGBC_EN_BIT_POS                  4u  //!< Position of bit RGBC_EN

// INTERRUPT bits position
#define INT_SOURCE_BIT_POS               2u  //!< Position of LSB bit INT_SOURCE

#define THRESHOLD_BYTE_NUM               4u  //!< Number of threshold bytes

#define TH_LSB_BYTE                   0xFFu  //!< Higher threshold low byte
#define TH_MSB_BYTE                   0xFFu  //!< Higher threshold high byte
#define TL_LSB_BYTE                   0x00u  //!< Lower threshold low byte
#define TL_MSB_BYTE                   0x00u  //!< Lower threshold high byte

#define DEFAULT_PERSISTENCE_VAL       0x01u  //!< Default persistence value
#define DEFAULT_MODE_CONTROL3_VAL     0x02u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

enum
{
  E_MeasurementTime_160ms,   //!< RGBC data are updated every 160 [ms]
  E_MeasurementTime_320ms,   //!< RGBC data are updated every 320 [ms]
  E_MeasurementTime_640ms,   //!< RGBC data are updated every 640 [ms]
  E_MeasurementTime_1280ms,  //!< RGBC data are updated every 1280 [ms]
  E_MeasurementTime_2560ms,  //!< RGBC data are updated every 2560 [ms]
  E_MeasurementTime_5120ms   //!< RGBC data are updated every 5120 [ms]
};
typedef uint8_t T_MeasurementTime;  //!< RGBC measurement time

enum
{
  E_ADCGain_1x,  //!< ADC gain is 1
  E_ADCGain_2x,  //!< ADC gain is 2
  E_ADCGain_16x  //!< ADC gain is 16
};
typedef uint8_t T_ADCGain;  //!< ADC gain

enum
{
  E_Persistence_ToggledAtEachMeasurement,    //!< Interrupt status is toggled at each measurement end
  E_Persistence_UpdateAfterEachMeasurement,  //!< Interrupt status is updated at each measurement end
  E_Persistence_UpdateAfter4,                //!< Interrupt status is updated if 4 consecutive threshold judgments are the same
  E_Persistence_UpdateAfter8                 //!< Interrupt status is updated if 8 consecutive threshold judgments are the same
};
typedef uint8_t T_Persistence;  //!< Persistence

enum
{
  E_InterruptSource_Red,    //!< Interrupt source is red channel
  E_InterruptSource_Green,  //!< Interrupt source is green channel
  E_InterruptSource_Blue,   //!< Interrupt source is blue channel
  E_InterruptSource_Clear   //!< Interrupt source is clear channel
};
typedef uint8_t T_InterruptSource;  //!< Interrupt source

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "color_sensor";

static uint8_t Threshold[THRESHOLD_BYTE_NUM] =
{
  TH_LSB_BYTE,
  TH_MSB_BYTE,
  TL_LSB_BYTE,
  TL_MSB_BYTE
};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Update the RGBC measurement time
//! \pre       None
//! \param     time The measurement time
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\UpdateMeasurementTime.svg
static void UpdateMeasurementTime(T_MeasurementTime time);

//! \brief     Update the ADC gain
//! \pre       None
//! \param     gain The selected ADC gain
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\UpdateADCGain.svg
static void UpdateADCGain(T_ADCGain gain);

//! \brief     Update the persistence
//! \pre       None
//! \param     persistence The selected persistence
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\UpdatePersistence.svg
static void UpdatePersistence(T_Persistence persistence);

//! \brief     Update the interrupt source
//! \pre       None
//! \param     source The selected interrupt source
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\UpdateInterruptSource.svg
static void UpdateInterruptSource(T_InterruptSource source);

//! \brief     Update the threshold
//! \pre       None
//! \param     threshold The selected threshold
//! \return    None
static void UpdateThreshold(uint8_t* threshold);

//! \brief     Update the Mode Control 3 register
//! \pre       None
//! \param     None
//! \return    None
static void UpdateModeControl3(void);

//! \brief     Enable the RGBC measurement
//! \pre       None
//! \param     None
//! \return    None
static void EnableMeasurement(void);

//! \brief     Enable the interrupt pin
//! \pre       None
//! \param     None
//! \return    None
static void EnableInterruptPin(void);

//! \brief     Read the RGBC illuminance
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\ReadIlluminance.svg
static void ReadIlluminance(T_Illuminance* illuminance);

//! \brief     Read the manufacturer ID
//! \pre       None
//! \param     None
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\ReadManufacturerId.svg
static void ReadManufacturerId(uint8_t* data);

#if 0
//! \brief     Read the red illuminance
//! \param     None
//! \return    None
static void ReadRedIlluminance(void);

//! \brief     Read the green illuminance
//! \pre       None
//! \param     None
//! \return    None
static void ReadGreenIlluminance(void);

//! \brief     Read the blue illuminance
//! \pre       None
//! \param     None
//! \return    None
static void ReadBlueIlluminance(void);

//! \brief     Read the clear illuminance
//! \pre       None
//! \param     None
//! \return    None
static void ReadClearIlluminance(void);

static void ReadSystemControlRegister(uint8_t* data);

static void ReadModeControl1Register(uint8_t* data);

static void ReadModeControl2Register(uint8_t* data);

static void ReadModeControl2Register(uint8_t* data);

static void ReadModeControl3Register(uint8_t* data);

static void ReadRedDataLsbRegister(uint8_t* data);

static void ReadRedDataMsbRegister(uint8_t* data);

static void ReadGreenDataLsbRegister(uint8_t* data);

static void ReadGreenDataMsbRegister(uint8_t* data);

static void ReadBlueDataLsbRegister(uint8_t* data);

static void ReadBlueDataMsbRegister(uint8_t* data);

static void ReadClearDataLsbRegister(uint8_t* data);

static void ReadClearDataMsbRegister(uint8_t* data);
#endif

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void BH1745NUC_Init(void)
{
  UpdateMeasurementTime(E_MeasurementTime_160ms);
  UpdateADCGain(E_ADCGain_1x);
  UpdatePersistence(E_Persistence_UpdateAfter4);
  UpdateInterruptSource(E_InterruptSource_Red);
  UpdateThreshold(Threshold);
  UpdateModeControl3();
  EnableMeasurement();
  EnableInterruptPin();
}

//_____________________________________________________________________________

void BH1745NUC_CheckManufacturerId(void)
{
  uint8_t id = 0x00u;

  ReadManufacturerId(&id);

  if (id != MANUFACTURER_ID)
  {
    ESP_LOGE(Tag, "Invalid manufacturer ID: %d", id);
  }
}

//_____________________________________________________________________________

void BH1745NUC_ReadRegisters(void)
{
  uint8_t table[38u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, SYSTEM_CONTROL_REG_ADDRESS, table, 38u);

  ESP_LOGE(Tag, "System Control: %d", table[0]);
  ESP_LOGE(Tag, "Mode Control 1: %d", table[1]);
  ESP_LOGE(Tag, "Mode Control 2: %d", table[2]);
  ESP_LOGE(Tag, "Mode Control 3: %d", table[4]);
  ESP_LOGE(Tag, "Red Data Lsb: %d",   table[16]);
  ESP_LOGE(Tag, "Red Data Msb: %d",   table[17]);
  ESP_LOGE(Tag, "Green Data Lsb: %d", table[18]);
  ESP_LOGE(Tag, "Green Data Msb: %d", table[19]);
  ESP_LOGE(Tag, "Blue Data Lsb: %d",  table[20]);
  ESP_LOGE(Tag, "Blue Data Msb: %d",  table[21]);
  ESP_LOGE(Tag, "Clear Data Lsb: %d", table[22]);
  ESP_LOGE(Tag, "Clear Data Msb: %d", table[23]);
  ESP_LOGE(Tag, "Dint Data Lsb: %d",  table[24]);
  ESP_LOGE(Tag, "Dint Data Msb: %d",  table[25]);
  ESP_LOGE(Tag, "Interrupt: %d",      table[32]);
  ESP_LOGE(Tag, "Persistence: %d",    table[33]);
  ESP_LOGE(Tag, "Th Lsb: %d",         table[34]);
  ESP_LOGE(Tag, "Th Msb: %d",         table[35]);
  ESP_LOGE(Tag, "Tl Lsb: %d",         table[36]);
  ESP_LOGE(Tag, "Tl Msb: %d",         table[37]);
}

//_____________________________________________________________________________

void BH1745NUC_GetIlluminance_lux(T_Illuminance* illuminance)
{
  ReadIlluminance(illuminance);

  // TODO calculate the color
}

//_____________________________________________________________________________

static void UpdateMeasurementTime(T_MeasurementTime time)
{
  uint8_t data = 0x00u;

  if (time <= E_MeasurementTime_5120ms)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL1_REG_ADDRESS, &data, 1u);

    data |= time;

    I2C_WriteToAddress(SLAVE_ADDRESS, MODE_CONTROL1_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid measurement time: %d", time);
  }
}

//_____________________________________________________________________________

static void UpdateADCGain(T_ADCGain gain)
{
  uint8_t data = 0x00u;

  if (gain <= E_ADCGain_16x)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL2_REG_ADDRESS, &data, 1u);

    data &= ADC_GAIN_BIT_MASK;
    data |= gain;

    I2C_WriteToAddress(SLAVE_ADDRESS, MODE_CONTROL2_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid ADC gain: %d", gain);
  }
}

//_____________________________________________________________________________

static void UpdatePersistence(T_Persistence persistence)
{
  uint8_t data = DEFAULT_PERSISTENCE_VAL;

  if (persistence <= E_Persistence_UpdateAfter8)
  {
    data = persistence;
    I2C_WriteToAddress(SLAVE_ADDRESS, PERSISTENCE_REG_ADDRESS, &data, 1u);
  }
}

//_____________________________________________________________________________

static void UpdateInterruptSource(T_InterruptSource source)
{
  uint8_t data = 0x00u;

  if (source <= E_InterruptSource_Clear)
  {
    I2C_ReadFromAddress(SLAVE_ADDRESS, INTERRUPT_REG_ADDRESS, &data, 1u);

    data &= INT_SOURCE_BIT_MASK;
    data |= (source << INT_SOURCE_BIT_POS);

    I2C_WriteToAddress(SLAVE_ADDRESS, INTERRUPT_REG_ADDRESS, &data, 1u);
  }
  else
  {
    ESP_LOGE(Tag, "Invalid interrupt source: %d", source);
  }
}

//_____________________________________________________________________________

static void UpdateThreshold(uint8_t* threshold)
{
  I2C_WriteToAddress(SLAVE_ADDRESS, TH_LSB_REG_ADDRESS, threshold, THRESHOLD_BYTE_NUM);
}

//_____________________________________________________________________________

static void UpdateModeControl3(void)
{
  uint8_t data = DEFAULT_MODE_CONTROL3_VAL;

  I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL3_REG_ADDRESS, &data, 1u);
}

//_____________________________________________________________________________

static void EnableMeasurement(void)
{
  uint8_t data = 0x00u;

  I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL2_REG_ADDRESS, &data, 1u);

  data |= (1u << RGBC_EN_BIT_POS);

  I2C_WriteToAddress(SLAVE_ADDRESS, MODE_CONTROL2_REG_ADDRESS, &data, 1u);
}

//_____________________________________________________________________________

static void EnableInterruptPin(void)
{
  uint8_t data = 0x00u;

  I2C_ReadFromAddress(SLAVE_ADDRESS, INTERRUPT_REG_ADDRESS, &data, 1u);

  data |= 1u;

  I2C_WriteToAddress(SLAVE_ADDRESS, INTERRUPT_REG_ADDRESS, &data, 1u);
}

//_____________________________________________________________________________

static void ReadIlluminance(T_Illuminance* illuminance)
{
  uint8_t colors[8u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, RED_DATA_LSB_REG_ADDRESS, colors, 8u);

  illuminance->Red   = (uint16_t)((uint16_t)colors[1u] << 8u) | colors[0u];
  illuminance->Green = (uint16_t)((uint16_t)colors[3u] << 8u) | colors[2u];
  illuminance->Blue  = (uint16_t)((uint16_t)colors[5u] << 8u) | colors[4u];
  illuminance->Clear = (uint16_t)((uint16_t)colors[7u] << 8u) | colors[6u];

  //ESP_LOGI(Tag, "Red: %d, Green: %d, Blue: %d, Clear: %d", Illuminance.Red, Illuminance.Green, Illuminance.Blue, Illuminance.Clear);
}

//_____________________________________________________________________________

static void ReadManufacturerId(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, MANUFACTURER_ID_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________
#if 0
static void ReadRedIlluminance(void)
{
  uint8_t red[2u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, RED_DATA_LSB_REG_ADDRESS, red, 2u);

  Illuminance.Red = (uint16_t)((uint16_t)red[1u] << 8u) | red[0u];
}

//_____________________________________________________________________________

static void ReadGreenIlluminance(void)
{
  uint8_t green[2u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, GREEN_DATA_LSB_REG_ADDRESS, green, 2u);

  Illuminance.Green = (uint16_t)((uint16_t)green[1u] << 8u) | green[0u];
}

//_____________________________________________________________________________

static void ReadBlueIlluminance(void)
{
  uint8_t blue[2u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, BLUE_DATA_LSB_REG_ADDRESS, blue, 2u);

  Illuminance.Blue = (uint16_t)((uint16_t)blue[1u] << 8u) | blue[0u];
}

//_____________________________________________________________________________

static void ReadClearIlluminance(void)
{
  uint8_t clear[2u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, CLEAR_DATA_LSB_REG_ADDRESS, clear, 2u);

  Illuminance.Clear = (uint16_t)((uint16_t)clear[1u] << 8u) | clear[0u];

  ESP_LOGI(Tag, "Clear: %d", Illuminance.Clear);
}

//_____________________________________________________________________________

static void ReadSystemControlRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, SYSTEM_CONTROL_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadModeControl1Register(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL1_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadModeControl2Register(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL2_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadModeControl3Register(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, MODE_CONTROL3_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadRedDataLsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, RED_DATA_LSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadRedDataMsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, RED_DATA_MSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadGreenDataLsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, GREEN_DATA_LSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadGreenDataMsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, GREEN_DATA_MSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadBlueDataLsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, BLUE_DATA_LSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadBlueDataMsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, BLUE_DATA_MSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadClearDataLsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, CLEAR_DATA_LSB_REG_ADDRESS, data, 1u);
}

//_____________________________________________________________________________

static void ReadClearDataMsbRegister(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, CLEAR_DATA_MSB_REG_ADDRESS, data, 1u);
}
#endif
