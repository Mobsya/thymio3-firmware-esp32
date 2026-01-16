//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    tcs3701.c
//! \brief   This module provides the useful functions to use the color sensor TCS3701
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <esp_log.h>

#include "tcs3701.h"

#include "i2c.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TCS3701_ADDRESS             0x39u  //!< Device address

#define SLAVE_ADDRESS                 (TCS3701_ADDRESS)  //!< Slave address

// Registers addresses
#define ENABLE_REG        0x80
#define ATIME_REG         0x81
#define ID_REG            0x92
#define ADATA0_REG_LOW    0x95
#define ADATA0_REG_HIGH   0x96
#define ADATA1_REG_LOW    0x97
#define ADATA1_REG_HIGH   0x98
#define ADATA2_REG_LOW    0x99
#define ADATA2_REG_HIGH   0x9A
#define ADATA3_REG_LOW    0x9B
#define ADATA3_REG_HIGH   0x9C
#define AGAIN_REG         0xAA
#define CFG8_REG          0xB1
#define ASTEP_REG         0xCA
#define AZ_CONFIG_REG     0xD6

#define DEVICE_ID     0x18  //!< Device ID



//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "tcs3701";


//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Read the device ID
//! \pre       None
//! \param     data - Data read from the manufacturer ID register
//! \return    None
static void ReadDeviceId(uint8_t* data);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TCS3701_Init(void)
{
  uint8_t data = 0x00u;
  uint8_t data2[2] = {0};

  // Power ON
  I2C_ReadFromAddress(SLAVE_ADDRESS, ENABLE_REG, &data, 1u);
  data |= 1;
  I2C_WriteToAddress(SLAVE_ADDRESS, ENABLE_REG, &data, 1u);
  // Enable ALS and Wait
  //data |= 0x0A;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ENABLE_REG, &data, 1u);
  // Enable ALS
  data |= 0x02;
  I2C_WriteToAddress(SLAVE_ADDRESS, ENABLE_REG, &data, 1u);  

  // Integration time = (ATIME + 1) × (ASTEP + 1) / 360
  // Default ATIME=0, ASTEP=999 => 1x1000/360 = 2.78 ms

  //// Integration time of 50 ms: ATIME=29, ASTEP=599 => 30x600/360 = 50
  //data = 29;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ATIME_REG, &data, 1u);
  // 599
  //data2[0] = 87;
  //data2[1] = 2;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ASTEP_REG, data2, 2);

  // Integration time of 13.9 ms: ATIME=4, ASTEP=999 => 5x1000/360 = 13.9
  data = 4;
  I2C_WriteToAddress(SLAVE_ADDRESS, ATIME_REG, &data, 1u);
  // 999
  data2[0] = 231;
  data2[1] = 3;
  I2C_WriteToAddress(SLAVE_ADDRESS, ASTEP_REG, data2, 2);

  // integration of 5 ms: ATIME=4 (defualt), ASTEP=359 => 5x360/360 = 5
  //data = 4;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ATIME_REG, &data, 1u);
  // 359  
  //data2[0] = 103;
  //data2[1] = 1;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ASTEP_REG, data2, 2);

  // integration of 1 ms: ATIME=0 (defualt), ASTEP=359 => 1x360/360 = 1
  //data = 0;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ATIME_REG, &data, 1u);
  // 359    
  //data2[0] = 103;
  //data2[1] = 1;
  //I2C_WriteToAddress(SLAVE_ADDRESS, ASTEP_REG, data2, 2);

  // Autozero every 10 cycles
  //data = 10;
  //I2C_WriteToAddress(SLAVE_ADDRESS, AZ_CONFIG_REG, &data, 1u);

  // Enable automatic gain control for ALS => when enabled the sensor stop working...?
  //I2C_ReadFromAddress(SLAVE_ADDRESS, CFG8_REG, &data, 1u);
  //data |= 4;
  //I2C_WriteToAddress(SLAVE_ADDRESS, CFG8_REG, &data, 1u);

  // ALS gain defualt value is 9 => 256x
  // ALS gain 1 => 1x
  //data = 1;
  //I2C_WriteToAddress(SLAVE_ADDRESS, AGAIN_REG, &data, 1u);
  // ALS gain 7 => 64x
  data = 7;
  I2C_WriteToAddress(SLAVE_ADDRESS, AGAIN_REG, &data, 1u);
  // ALS gain 8 => 128x
  //data = 8;
  //I2C_WriteToAddress(SLAVE_ADDRESS, AGAIN_REG, &data, 1u);

  ESP_LOGI(Tag, "TCS3701 is initialized");
}

//_____________________________________________________________________________

T_Error TCS3701_CheckDeviceId(void)
{
  uint8_t id = 0x00u;
  T_Error err = E_Error_None;

  ReadDeviceId(&id);

  if (id != DEVICE_ID)
  {
    err = E_Error_Color_InvalidID;
    ESP_LOGE(Tag, "Invalid TCS3701 device ID: %d", id);
  }

  return err;
}

//_____________________________________________________________________________

void TCS3701_ReadColor(T_RawColor* raw)
{
  uint8_t colors[8u];

  I2C_ReadFromAddress(SLAVE_ADDRESS, ADATA0_REG_LOW, colors, 8u);

  raw->Clear   = (uint16_t)((uint16_t)colors[1u] << 8u) | colors[0u];
  raw->Red = (uint16_t)((uint16_t)colors[3u] << 8u) | colors[2u];
  raw->Green  = (uint16_t)((uint16_t)colors[5u] << 8u) | colors[4u];
  raw->Blue = (uint16_t)((uint16_t)colors[7u] << 8u) | colors[6u];

  //ESP_LOGI(Tag, "R: %d, G: %d, B: %d, C: %d", raw->Red, raw->Green, raw->Blue, raw->Clear);
}

//_____________________________________________________________________________

static void ReadDeviceId(uint8_t* data)
{
  I2C_ReadFromAddress(SLAVE_ADDRESS, ID_REG, data, 1u);
  ESP_LOGI(Tag, "color sensor id = %d", *data);
}
