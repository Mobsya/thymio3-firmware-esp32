//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    i2c.c
//! \brief   This module provides the useful functions to use the I2C protocol
//!
//! \author  Vincent Gonet
//!
//! \version $Id: i2c.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <driver/i2c.h>
#include <esp_log.h>

#include "i2c.h"
#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ACK_CHECK_EN               0x1          //!< I2C master will check ack from slave
#define ACK_CHECK_DIS              0x0          //!< I2C master will not check ack from slave
#define ACK_VAL                    0x0          //!< I2C ack value
#define NACK_VAL                   0x1          //!< I2C nack value

#define I2C_MASTER_TX_BUF_DISABLE  0            //!< I2C master do not need buffer
#define I2C_MASTER_RX_BUF_DISABLE  0            //!< I2C master do not need buffer
#define I2C_MASTER_FREQ_HZ         100000       //!< I2C master clock frequency

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "i2c";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void I2C_Init(void)
{
  i2c_config_t conf;

  conf.mode = I2C_MODE_MASTER;
  conf.sda_io_num = SDA_PIN;
  conf.scl_io_num = SCL_PIN;
  conf.sda_pullup_en = GPIO_PULLUP_DISABLE; //GPIO_PULLUP_ENABLE;
  conf.scl_pullup_en = GPIO_PULLUP_DISABLE; //GPIO_PULLUP_ENABLE;
  conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

  ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
  ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));

  ESP_LOGI(Tag, "I2C is initialized");
}

//_____________________________________________________________________________

void I2C_WriteByte(uint8_t slaveAddress, uint8_t data)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_WRITE, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, data, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS));

  i2c_cmd_link_delete(cmd);
}

//_____________________________________________________________________________

uint8_t I2C_ReadByte(uint8_t slaveAddress, uint8_t registerAddress)
{
  uint8_t byte;

  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_READ, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_read_byte(cmd, &byte, NACK_VAL));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS));

  i2c_cmd_link_delete(cmd);

  return byte;
}

//_____________________________________________________________________________

void I2C_WriteToAddress(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t size)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_WRITE, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, regAddress, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_write(cmd, data, size, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 0));
  i2c_cmd_link_delete(cmd);
}

//_____________________________________________________________________________

void I2C_ReadFromAddressWithStop(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t size)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_WRITE, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, regAddress, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 0));
  i2c_cmd_link_delete(cmd);

  cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_READ, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_read(cmd, data, size, NACK_VAL));
  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 0));
  i2c_cmd_link_delete(cmd);
}

//_____________________________________________________________________________

void I2C_ReadFromAddress(uint8_t slaveAddress, uint8_t regAddress, uint8_t* data, uint16_t size)
{
  i2c_cmd_handle_t cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_WRITE, ACK_CHECK_EN));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, regAddress, ACK_CHECK_EN));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 0u));
  i2c_cmd_link_delete(cmd);

  cmd = i2c_cmd_link_create();

  ESP_ERROR_CHECK(i2c_master_start(cmd));
  ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (slaveAddress << 1u) | I2C_MASTER_READ, ACK_CHECK_EN));

  if (size > 1u)
  {
    ESP_ERROR_CHECK(i2c_master_read(cmd, &data[0u], (size - 1u), ACK_VAL));
    ESP_ERROR_CHECK(i2c_master_read(cmd, &data[size - 1u], 1u, NACK_VAL));
  }
  else
  {
    ESP_ERROR_CHECK(i2c_master_read(cmd, &data[0u], 1u, NACK_VAL));
  }

  ESP_ERROR_CHECK(i2c_master_stop(cmd));

  ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 0u));
  i2c_cmd_link_delete(cmd);
}
