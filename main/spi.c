//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "esp_log.h"
#include "esp_intr_alloc.h"

#include "spi.h"
#include "gpio.h"
#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define HSPI_CLK_FREQENCY_Hz    1000000  //8000000

#define VSPI_CLK_FREQENCY_Hz    1000000  //8000000

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

static spi_transaction_t transaction = // Placed in DRAM bss section
//DRAM_ATTR spi_transaction_t transaction = // Placed in DRAM data section
{
  .flags = 0,
  .cmd = 0,
  .addr = 0,
  .length = 0,
  .rxlength = 0,
  .tx_buffer = NULL,
  .rx_buffer = NULL
};

static spi_transaction_t trans_desc = // Placed in DRAM bss section
//DRAM_ATTR spi_transaction_t trans_desc = // Placed in DRAM data section
{
  .flags = 0,
  .cmd = 0,
  .addr = 0,
  .length = 0,
  .rxlength = 0,
  .tx_buffer = NULL,
  .rx_buffer = NULL
};

void Spi_InitHSPI(void)
{
  spi_bus_config_t hspi_config =
  {
    .sclk_io_num   = LED_CLK_PIN,
    .mosi_io_num   = LED_SDI_PIN,
    .miso_io_num   = -1, // Not used
    .quadwp_io_num = -1, // Not used
    .quadhd_io_num = -1,  // Not used
	.flags = SPICOMMON_BUSFLAG_MASTER, // | SPICOMMON_BUSFLAG_NATIVE_PINS,
	.intr_flags = ESP_INTR_FLAG_IRAM // SPI ISR saved in IRAM
  };

  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &hspi_config, SPI_DMA_CH_AUTO));

  ESP_LOGI(Tag, "HSPI SPI is initialized");
}

//_____________________________________________________________________________

void Spi_InitVSPI(void)
{
  spi_bus_config_t vspi_config =
  {
    .sclk_io_num   = SPI_CLK_PIN,
    .mosi_io_num   = SPI_MOSI_PIN,
    .miso_io_num   = SPI_MISO_PIN,
    .quadwp_io_num = -1, // Not used
    .quadhd_io_num = -1,  // Not used
	.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_NATIVE_PINS,
	.intr_flags = ESP_INTR_FLAG_IRAM // SPI ISR saved in IRAM
  };

  ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &vspi_config, SPI_DMA_CH_AUTO));

  ESP_LOGI(Tag, "VSPI SPI is initialized");
}

//_____________________________________________________________________________

void Spi_AddDeviceHSPI(spi_device_handle_t* device, int csPin)
{
  spi_device_interface_config_t dev_config =
  {
    .address_bits     = 0,
    .command_bits     = 0,
    .dummy_bits       = 0,
    .mode             = 0,
    .duty_cycle_pos   = 0,  // 50%
    .cs_ena_posttrans = 0,
    .cs_ena_pretrans  = 0,
    .clock_speed_hz   = HSPI_CLK_FREQENCY_Hz,
    .spics_io_num     = csPin,
    .flags            = 0, //SPI_DEVICE_NO_DUMMY
    .queue_size       = 5,
    .pre_cb           = NULL,
    .post_cb          = NULL
  };

  ESP_LOGI(Tag, "Add device on HSPI bus");
  ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &dev_config, device));
}

//_____________________________________________________________________________

void Spi_AddDeviceVSPI(spi_device_handle_t* device, int csPin)
{
  spi_device_interface_config_t dev_config =
  {
    .address_bits     = 0,
    .command_bits     = 0,
    .dummy_bits       = 0,
    .mode             = 0,
    .duty_cycle_pos   = 0,  // 50%
    .cs_ena_posttrans = 0,
    .cs_ena_pretrans  = 0,
    .clock_speed_hz   = VSPI_CLK_FREQENCY_Hz,
    .spics_io_num     = csPin,
    .flags            = 0,
    .queue_size       = 3,
    .pre_cb           = NULL,
    .post_cb          = NULL
  };

  ESP_LOGI(Tag, "Add device on VSPI bus");
  ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev_config, device));
}

//_____________________________________________________________________________

void Spi_WriteHSPI(spi_device_handle_t device, uint8_t* data, uint16_t size)
{
	trans_desc.length = size * 8;
	trans_desc.rxlength = 0;
	trans_desc.tx_buffer = data;
	trans_desc.rx_buffer = NULL;

  ESP_ERROR_CHECK(spi_device_queue_trans(device, &trans_desc, portMAX_DELAY));
}

//_____________________________________________________________________________

void Spi_CommunicateVSPI(spi_device_handle_t device, int16_t* txBuffer, int16_t* rxBuffer, uint16_t size)
{
	  transaction.length = 16 * size;
	  transaction.rxlength = 0; //16 * size; //0;
	  transaction.tx_buffer = txBuffer;
	  transaction.rx_buffer = rxBuffer;

  spi_device_transmit(device, &transaction);
}
