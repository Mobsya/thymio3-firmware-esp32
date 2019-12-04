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

//#include <driver/spi_master.h>
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "spi.h"
#include "board.h"

#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define HSPI_CLK_FREQENCY_Hz    1000000  //8000000

#define VSPI_CLK_FREQENCY_Hz    100000  //8000000

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

static void Callback_EndTransmissionVSPI(spi_transaction_t* t);

static uint8_t* Rx = NULL;
static uint16_t Cmd = 0;

static bool Ready = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Spi_InitHSPI(void)
{
  spi_bus_config_t hspi_config =
  {
    .sclk_io_num   = LED_CLK_PIN,
    .mosi_io_num   = LED_SDI_PIN,
    .miso_io_num   = -1, // Not used
    .quadwp_io_num = -1, // Not used
    .quadhd_io_num = -1  // Not used
  };

  ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &hspi_config, 1));

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
    .quadhd_io_num = -1  // Not used
  };

  ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &vspi_config, 2));

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
    .flags            = 0,
    .queue_size       = 1,
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
    .flags            = 0, //SPI_DEVICE_HALFDUPLEX,
    .queue_size       = 3,
    .pre_cb           = NULL,
    .post_cb          = NULL, //Callback_EndTransmissionVSPI
  };

  ESP_LOGI(Tag, "Add device on VSPI bus");
  ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev_config, device));
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

//_____________________________________________________________________________

//void Spi_WriteVSPI(spi_device_handle_t device, uint16_t* txBuffer, uint16_t size)
void Spi_WriteVSPI(spi_device_handle_t device, uint16_t* txBuffer, uint16_t* rxBuffer, uint16_t size)
//void Spi_Write(uint8_t* data, uint16_t size)
{
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof(transaction));
  //transaction.addr = 0x05;
  //transaction.cmd = 0x26,
  transaction.length = 16 * size;
  transaction.flags = 0;
  transaction.tx_buffer = txBuffer;
  transaction.rx_buffer = rxBuffer;

  spi_device_transmit(device, &transaction);

#if 0
  spi_transaction_t trans_desc =
  {
    .flags = 0,
    //.flags = SPI_TRANS_USE_TXDATA,
    .cmd = 0xAA,
    .addr = 0,
    .length = size * 8,
    .rxlength = 0,
    .tx_buffer = txBuffer,
    .rx_buffer = rxBuffer
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
#endif
}

//_____________________________________________________________________________

void Spi_ReadVSPI(spi_device_handle_t device, uint16_t* rxBuffer, uint16_t size)
//void Spi_ReadVSPI(spi_device_handle_t device, uint8_t* rxBuffer, uint16_t size)
{
  spi_transaction_t transaction;

  memset(&transaction, 0, sizeof(transaction));
  transaction.length = 16 * size;
  transaction.flags = 0;
  transaction.rx_buffer = rxBuffer;

  spi_device_transmit(device, &transaction);
}

//_____________________________________________________________________________

void Spi_ClearReady(void)
{
  Ready = false;
}


//_____________________________________________________________________________

bool Spi_IsRxBufferReady(void)
{
  return Ready;
}

//_____________________________________________________________________________
#if 0
uint8_t* lcd_get_id(spi_device_handle_t device, uint8_t* rxBuffer, uint16_t size)
{
  //get_id cmd
  //lcd_cmd(spi, 0x04);
#if 0
  spi_transaction_t trans_desc =
  {
    .flags = 0,
    //.flags = SPI_TRANS_USE_TXDATA,
    .cmd = 0xAA,
    .addr = 0,
    .length = size * 8,
    .rxlength = 0,
    .tx_buffer = NULL,
    .rx_buffer = rxBuffer
  };
#endif
  spi_transaction_t t;
  memset(&t, 0, sizeof(t));
  t.length = 8 * size;
  t.flags = 0;//SPI_TRANS_USE_RXDATA;
  t.user = (void*)1;
  t.rx_buffer = rxBuffer;

  //esp_err_t ret = spi_device_polling_transmit(spi, &t);
  //spi_device_queue_trans(device, &t, portMAX_DELAY);
  spi_device_transmit(device, &t);


  return (uint8_t*)t.rx_buffer;
}
#endif
//_____________________________________________________________________________

static void Callback_EndTransmissionVSPI(spi_transaction_t* t)
{
  if (!Ready)
  {
    Rx = (uint8_t*)t->rx_buffer;
    Cmd = t->cmd;
  }

  Ready = true;
}

