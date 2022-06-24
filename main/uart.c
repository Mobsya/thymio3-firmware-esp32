//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    uart.c
//! \brief   This module provides the useful functions to use the UART
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"

#include "uart.h"

#include "pins_def.h"
#include "stm32_spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define UART_NUM          UART_NUM_0

#define UART_BAUDRATE        115200u

#define RX_BUFFER_SIZE         2048u
#define TX_BUFFER_SIZE         2048u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "uart";

//static QueueHandle_t UartQueue;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void UART_Init(void)
{
  uart_config_t config =
  {
    .baud_rate = UART_BAUDRATE,
    .data_bits = UART_DATA_8_BITS,
    .parity    = UART_PARITY_DISABLE,
    .stop_bits = UART_STOP_BITS_1,
    .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
  };

  ESP_ERROR_CHECK(uart_param_config(UART_NUM, &config));
  ESP_ERROR_CHECK(uart_set_pin(UART_NUM, TXD_ESP32_PIN, RXD_ESP32_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
  //uart_driver_install(UART_NUM, RX_BUFFER_SIZE * 2, 0, 0, NULL, 0);
  ESP_ERROR_CHECK(uart_driver_install(UART_NUM, RX_BUFFER_SIZE, TX_BUFFER_SIZE, 0, NULL, 0));
  //uart_driver_install(UART_NUM, RX_BUFFER_SIZE * 2, TX_BUFFER_SIZE * 2, 20, &UartQueue, 0);

  uart_flush(UART_NUM);  // FIXME In conflict with I2S



  //ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &config));
  //ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, SPI_MOSI_PIN, SPI_MISO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

  //ESP_ERROR_CHECK(uart_driver_install(UART_NUM_1, RX_BUFFER_SIZE, TX_BUFFER_SIZE, 0, NULL, 0));

  ESP_LOGI(Tag, "UART is initialized");
}

//_____________________________________________________________________________

void UART_Flush(void)
{
  //ESP_LOGI(Tag, "UART is flushed");
  uart_flush(UART_NUM);
  //uart_driver_delete(UART_NUM);
  //uart_driver_install(UART_NUM, RX_BUFFER_SIZE, TX_BUFFER_SIZE, 0, NULL, 0);
  //UART_Init();
}

//_____________________________________________________________________________

void UART_Write(const uint8_t* data, uint16_t size)
{
  if (STM32_IsUSBPortOpen())
  {
    uart_write_bytes(UART_NUM, (const char*)data, size);
  }
}

//_____________________________________________________________________________

int UART_Read(uint8_t* data)
{
  int length = 0;

  ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));

  length = uart_read_bytes(UART_NUM, data, length, 0u);

  //uart_flush(UART_NUM);

  return length;
}

//_____________________________________________________________________________

int UART_ReadByte(uint8_t* data)
{
  int length = uart_read_bytes(UART_NUM, data, 1u, 500u);

  return length;
}

//_____________________________________________________________________________

bool UART_IsRxBufferEmpty(void)
{
  int16_t length = 0;

  ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));

  return (length == 0);
}

//_____________________________________________________________________________

int16_t UART_GetRxBufferDataLength(void)
{
  int16_t length = 0;

  ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));

  return (int16_t)length;
}

//_____________________________________________________________________________

void UART_WaitUntilTxFifoIsEmpty(void)
{
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 100));  // wait timeout is 100 RTOS ticks (TickType_t)
}
