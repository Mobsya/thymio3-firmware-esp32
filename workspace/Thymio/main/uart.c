//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: uart.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "driver/uart.h"
#include <esp_log.h>

#include "uart.h"

#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define UART_NUM          UART_NUM_0

#define UART_BAUDRATE        115200u

#define RX_BUFFER_SIZE         1024u
#define TX_BUFFER_SIZE         1024u

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

  uart_param_config(UART_NUM, &config);
  uart_set_pin(UART_NUM, TXD_ESP32_PIN, RXD_ESP32_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  uart_driver_install(UART_NUM, RX_BUFFER_SIZE * 2, 0, 0, NULL, 0);
  //uart_driver_install(UART_NUM, RX_BUFFER_SIZE * 2, TX_BUFFER_SIZE * 2, 20, &UartQueue, 0);

  ESP_LOGI(Tag, "UART is initialized");
}

//_____________________________________________________________________________
#if 0
void UART_Task(void)
{
  uart_event_t event;
  uint8_t* data = (uint8_t*)malloc(RX_BUFFER_SIZE);
  int len = 0;

  while (1)
  {
    // Waiting for UART event
	if (xQueueReceive(UartQueue, (void*)&event, (portTickType)portMAX_DELAY))
	{
      bzero(data, RX_BUFFER_SIZE);

      switch(event.type)
      {
        // Event of UART receving data
        case UART_DATA:
          len = UART_Read(data);
          UART_Write(data, len);
          break;

        // Event of HW FIFO overflow detected
        case UART_FIFO_OVF:
          uart_flush_input(UART_NUM);
          xQueueReset(UartQueue);
          break;

        // Event of UART ring buffer full
        case UART_BUFFER_FULL:
          uart_flush_input(UART_NUM);
          xQueueReset(UartQueue);
          break;

        // Event of UART RX break detected
        case UART_BREAK:
          ESP_LOGE(Tag, "uart rx break");
          break;

        // Event of UART parity check error
        case UART_PARITY_ERR:
          ESP_LOGI(Tag, "uart parity error");
          break;

        // Event of UART frame error
        case UART_FRAME_ERR:
          ESP_LOGI(Tag, "uart frame error");
          break;

        // Event of UART pattern detected
        case UART_PATTERN_DET:
          break;

        default:
          break;
      }
	}

	vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}
#endif
//_____________________________________________________________________________

void UART_Write(const uint8_t* data, uint16_t size)
{
  uart_write_bytes(UART_NUM, (const char*)data, size);
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
  int length = uart_read_bytes(UART_NUM, data, 1u, 0u);

  return length;
}

//_____________________________________________________________________________

bool UART_IsReceptionBufferEmpty(void)
{
  int length = 0;

  ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM, (size_t*)&length));

  return (length == 0);
}

//_____________________________________________________________________________

void UART_WaitUntilTxFifoIsEmpty(void)
{
  ESP_ERROR_CHECK(uart_wait_tx_done(UART_NUM, 100));  // wait timeout is 100 RTOS ticks (TickType_t)
}
