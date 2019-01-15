//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    spi.c
//! \brief   This module provides the useful functions to use the SPI protocol
//!
//! \author  Vincent Gonet
//!
//! \version $Id: spi.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//#include <driver/spi_master.h>
#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spi.h"
#include "board.h"

#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SPI_CLK_FREQENCY_Hz    10000000//8000000

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "spi";

//static spi_device_handle_t sn74hc595;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Spi_Init(void)
{
  spi_bus_config_t bus_config =
  {
    .sclk_io_num   = LED_CLK_PIN,
    .mosi_io_num   = LED_SDI_PIN,
    .miso_io_num   = -1, // Not used
    .quadwp_io_num = -1, // Not used
    .quadhd_io_num = -1  // Not used
  };

  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &bus_config, 1));

  ESP_LOGI(Tag, "SPI is initialized");
}

//_____________________________________________________________________________

void Spi_AddDevice(spi_device_handle_t* device, int csPin)
{
  spi_device_interface_config_t dev_config =
  {
    .address_bits     = 0,
    .command_bits     = 0,
    .dummy_bits       = 0,
    .mode             = 0,
    .duty_cycle_pos   = 0,
    .cs_ena_posttrans = 0,
    .cs_ena_pretrans  = 0,
    .clock_speed_hz   = SPI_CLK_FREQENCY_Hz,
    .spics_io_num     = csPin,
    .flags            = 0,
    .queue_size       = 1,
    .pre_cb           = NULL,
    .post_cb          = NULL
  };

  ESP_LOGI(Tag, "... Adding device bus.");
  ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_config, device));
}

//_____________________________________________________________________________

void Spi_Write(spi_device_handle_t device, uint8_t* data, uint16_t size)
//void Spi_Write(uint8_t* data, uint16_t size)
{
  spi_transaction_t trans_desc =
  {
    .flags = 0,
    //.flags = SPI_TRANS_USE_TXDATA,
    .cmd = 0,
    .addr = 0,
    .length = size * 8,
    .rxlength = 0,
    .tx_buffer = data,
    .rx_buffer = NULL
  };

  //ESP_LOGI(Tag, "... Transmitting.");
  //ESP_ERROR_CHECK(spi_device_transmit(device, &trans_desc));
  ESP_ERROR_CHECK(spi_device_queue_trans(device, &trans_desc, portMAX_DELAY));
  //spi_device_polling_transmit(device, &trans_desc);
  //spi_device_transmit(device, &trans_desc);
  //spi_device_queue_trans(device, &trans_desc, portMAX_DELAY);

  //ESP_LOGI(Tag, "... Removing device.");
  //ESP_ERROR_CHECK(spi_bus_remove_device(device));

  //ESP_LOGI(Tag, "... Freeing bus.");
  //ESP_ERROR_CHECK(spi_bus_free(HSPI_HOST));
  //vTaskDelete(NULL);
}
