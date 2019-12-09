//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    thymio-buffer.c
//! \brief   This module provides the useful functions to manage the thymio buffer
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "thymio-buffer.h"

#include "aseba_esp32.h"
#include "fifo.h"
#include "leds.h"
#include "stm32_spi.h"
#include "tcp_server.h"
#include "uart.h"
#include "wifi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

// No communication bus presents
#define MODE_DISCONNECTED 0x2
// USB is connected _AND_ running (DTE bit set)
#define MODE_USB    0x1
// RF link is present
#define MODE_WIFI   0x0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct fifo
{
  unsigned char* buffer;
  size_t size;
  size_t insert;
  size_t consume;
};

static struct
{
  struct fifo rx;
  struct fifo tx;
} AsebaFifo;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static uint8_t ConnectionMode;

static uint8_t commError;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

static inline size_t get_free(struct fifo* f)
{
  return f->size - 1 - Fifo8bits_GetNumberOfElements(f);
}

//_____________________________________________________________________________

void AsebaFifoPushToRx(unsigned char c)
{
  Fifo8bits_Write(&AsebaFifo.rx, &c, 1);
}

//_____________________________________________________________________________

int AsebaFifoRxFull(void)
{
  return !get_free(&AsebaFifo.rx);
}

//_____________________________________________________________________________

void AsebaFifoCheckConnectionMode(void)
{
  if (STM32_IsUSBPortOpen())
  {
    if (ConnectionMode != MODE_USB)
    {
      // Switch off the WIFI
      ESP_LOGI("thymio-buffer", "COUCOU Switch off WIFI");
      //WIFI_Disconnect();
      TCPServer_ShutDownSocket();
    }

    Fifo8bits_Reset(TCPFifoRx);
    ConnectionMode = MODE_USB;
  }
  else if (WIFI_IsConnected())
  {
    if (TCPServer_IsSocketAccepted())
    {
      ConnectionMode = MODE_WIFI;
    }
    else
    {
      Fifo8bits_Reset(TCPFifoRx);
    }
  }
  else
  {
    // No USB-UART, no WIFI
    Fifo8bits_Reset(TCPFifoRx);

    ConnectionMode = MODE_DISCONNECTED;
  }

#if 0
  if (usb_uart_serial_port_open())
  {
    if (connection_mode == MODE_USB)
    {
      return;  // Nothing to do ...
    }

    // Put the RF link down if it was up
    if (rf_get_status() & RF_LINK_UP)
    {
      rf_set_link(RF_DOWN);
    }

    // We are switching to usb, reset the fifo and make the switch
    Fifo_Reset(&AsebaFifo.tx);
    Fifo_Reset(&AsebaFifo.rx);
    connection_mode = MODE_USB;
    return;
  }

  // No usb, so try RF.
  if (rf_get_status() & RF_PRESENT)
  {
    if (connection_mode == MODE_WIFI)
    {
      return;  // Nothing to do
    }

    Fifo_Reset(&AsebaFifo.tx);
    Fifo_Reset(&AsebaFifo.rx);

    // We are switching *from* usb, start the RF link
    if (!(rf_get_status() & RF_LINK_UP))
    {
      rf_set_link(RF_UP);
    }

    connection_mode = MODE_WIFI;

    return;
  }

  // No RF, No usb ...
  Fifo_Reset(&AsebaFifo.tx);
  Fifo_Reset(&AsebaFifo.rx);

  connection_mode = MODE_DISCONNECTED;
#endif
}

//_____________________________________________________________________________

/* USB interrupt part */
// They can be called from the main, but with usb interrupt disabled, so it's OK
static int tx_busy;
static int debug;

unsigned char AsebaTxReady(unsigned char* data)
{
  size_t size = Fifo8bits_GetNumberOfElements(&AsebaFifo.tx);

  // Do not send anything on usb if we are not in usb mode
  if (size == 0 || ConnectionMode != MODE_USB)
  {
    tx_busy = 0;
    debug = 0;
    return 0;
  }

  if (size > ASEBA_USB_MTU)
  {
    size = ASEBA_USB_MTU;
  }

  Fifo8bits_Read(&AsebaFifo.tx, data, size);
  debug ++;
  return size;
}

//_____________________________________________________________________________

int AsebaUsbBulkRecv(unsigned char* data, unsigned char size)
{
  // Ignore all data if we are not in usb mode
  AsebaFifoCheckConnectionMode();

  if (ConnectionMode != MODE_USB)
  {
    return 0;
  }

  size_t free = get_free(&AsebaFifo.rx);

  if (size > free)
  {
    return 1;
  }

  Fifo8bits_Write(&AsebaFifo.rx, data, size);

  return 0;
}

/* RF Part */

//_____________________________________________________________________________

/* main() part */

static void uartSendUInt8(uint8_t value)
{
  UART_Write(&value, 1);
  UART_WaitUntilTxFifoIsEmpty();
  //while (e_uart1_sending());
}

//_____________________________________________________________________________

static void tcpSendUInt8(uint8_t value)
{
  TCPServer_Send(&value, 1);
}

//_____________________________________________________________________________

static void uartSendUInt16(uint16_t value)
{
//#if 0
  uint8_t data[2] =
  {
    (uint8_t)(value & 0x00FF),
    (uint8_t)((value >> 8) & 0x00FF)
  };

  UART_Write(data, 2);
//#endif

#if 0
  uint8_t val1 = (uint8_t)(value & 0x00FF);
  uint8_t val2 = (uint8_t)((value >> 8) & 0x00FF);

  UART_Write(&val1, 1);
  UART_Write(&val2, 1);
#endif

  //UART_Write((uint8_t*)&value, 2);
  UART_WaitUntilTxFifoIsEmpty();
  //while (e_uart1_sending());
}

//_____________________________________________________________________________

static void tcpSendUInt16(uint16_t value)
{
  uint8_t data[2] =
  {
    (uint8_t)(value & 0x00FF),
    (uint8_t)((value >> 8) & 0x00FF)
  };

  TCPServer_Send(data, 2);
}

//_____________________________________________________________________________

void AsebaSendBuffer(AsebaVMState* vm, const uint8_t* data, uint16_t length)
{
  //AsebaFifoCheckConnectionMode();

  if (ConnectionMode == MODE_USB)
  {
    if (length >= 2)
    {
      uartSendUInt16(length - 2u);
      uartSendUInt16(vm->nodeId);
      //uartSendUInt16(vmState.nodeId);

      for (uint16_t i = 0u; i < length; i++)
      {
        uartSendUInt8(*data++);
      }
    }
  }
  else if (ConnectionMode == MODE_WIFI)
  {
    if (length >= 2)
    {
      tcpSendUInt16(length - 2u);
      tcpSendUInt16(vm->nodeId);
      //tcpSendUInt16(vmState.nodeId);

      //ESP_LOGI("thymio-buffer", "HELLO Send");

      for (uint16_t i = 0u; i < length; i++)
      {
        tcpSendUInt8(*data++);
      }
    }
  }
  else  // MODE_DISCONNECTED
  {
    // Do nothing
  }

#if 0
  int flags;
  unsigned char mode = connection_mode;


  barrier(); // Force the compiler to capture mode


  // Here we must loop until we can send the data.
  // BUT if we are disconnected we simply drop the data.
  if (mode == MODE_DISCONNECTED)
  {
    return;
  }

  // Sanity check, should never be true
  if (length < 2)
  {
    return;
  }
  // Cannot send big user packet for Thymio.
  const uint16_t MAX_BUFF_SIZE = ((32 + 2) * 2);
  if ((data[1] < 0x80) && (length > MAX_BUFF_SIZE))
  {
    AsebaVMEmitNodeSpecificError(vm, "Argument array size is too large (>32)");
    return;
  }
  do
  {
    RAISE_IPL(flags, PRIO_COMMUNICATION);
    AsebaFifoCheckConnectionMode();
    if (mode != connection_mode)
    {
      // the connection medium changed under our feet, let's drop this packet
      // No need to reset the fifo it has already been done.
      IRQ_ENABLE(flags);
      break;
    }

    if (get_free(&AsebaFifo.tx) >= length + 4)
    {
      length -= 2;
      Fifo8bits_Write(&AsebaFifo.tx, (unsigned char*) &length, 2);
      Fifo8bits_Write(&AsebaFifo.tx, (unsigned char*) &vm->nodeId, 2);
      Fifo8bits_Write(&AsebaFifo.tx, (unsigned char*) data, length + 2);

      // Will callback AsebaUsbTxReady
      if (mode == MODE_USB)
      {
        if (!tx_busy)
        {
          tx_busy = 1;
          USBCDCKickTx();
        }
      }

      length = 0;
    }

    IRQ_ENABLE(flags);
  }
  while (length);
#endif
}

//_____________________________________________________________________________

static uint8_t uartGetUInt8(void)
{
  uint8_t c;
  UART_ReadByte(&c);
  return c;
}

//_____________________________________________________________________________

static uint16_t uartGetUInt16(void)
{
  uint16_t value = uartGetUInt8();  // Little endian

  value |= (uartGetUInt8() << 8);

  return value;
}

//_____________________________________________________________________________

uint16_t AsebaGetBuffer(AsebaVMState* vm, uint8_t* data, uint16_t maxLength, uint16_t* source)
{
  uint16_t ret = 0;
  uint16_t len = 0;

  static uint16_t counter = 0;

  AsebaFifoCheckConnectionMode();

  uint16_t used = Fifo8bits_GetNumberOfElements(TCPFifoRx);

#if 0
  if (used > 0)
  {
    ESP_LOGI("thymio-buffer", "used = %d", used);
  }
  else
  {
    counter++;

    if (counter >= 100)
    {
      counter = 0;
      ESP_LOGI("thymio-buffer", "used = %d", used);
    }
  }
#endif

  if (ConnectionMode == MODE_USB)
  {
    if (!UART_IsRxBufferEmpty())
    {
      if (UART_GetRxBufferDataLength() >= 6)
      {
        len = uartGetUInt16() + 2;

        if (len > maxLength)  // Wrong data received.
        {
          return 0;
        }

        *source = uartGetUInt16();

        for (uint16_t i = 0; i < len; i++)
        {
          *data++ = uartGetUInt8();

          if (commError)
          {
            return 0;
          }
        }

        ret = len;
      }
    }
  }
  else if (ConnectionMode == MODE_WIFI)
  {
    if (used >= 6)
    {
      //ESP_LOGI("thymio-buffer", "HELLO THYMIO");
      Fifo8bits_Peek(TCPFifoRx, (uint8_t*)&len, 2);

      if (used >= (len + 6))
      {
        Fifo8bits_Read(TCPFifoRx, (uint8_t*)&len, 2);
        Fifo8bits_Read(TCPFifoRx, (uint8_t*)source, 2);

        // msg_type is not in the len but is always present
        len += 2;

        if (len > maxLength)  // Wrong data received.
        {
          len = maxLength;
          //ESP_LOGI("thymio-buffer", "HELLO WRONG");
        }

        Fifo8bits_Read(TCPFifoRx, data, len);
        ret = len;
#if 0
        if (ret > 0)
        {
          //ESP_LOGI("thymio-buffer", "Ret = %d", ret);
        }
#endif
      }
    }
    else
    {
      //ESP_LOGI("thymio-buffer", "HELLO STRANGE");
    }
  }
  else  // MODE_CONNECTED
  {
    // Do nothing
    //ESP_LOGI("thymio-buffer", "HELLO DISCONNECTED");
  }

  return ret;

#if 0
  int flags;
  uint16_t ret = 0;
  size_t u;
  // Touching the FIFO, mask the interrupt ...
  RAISE_IPL(flags, PRIO_COMMUNICATION);

  AsebaFifoCheckConnectionMode();

  u = Fifo_GetNumberOfElements(&AsebaFifo.rx);

  /* Minium packet size == len + src + msg_type == 6 bytes */
  if (u >= 6)
  {
    int len;
    Fifo_Peek(&AsebaFifo.rx, (unsigned char*)&len, 2);

    if (u >= len + 6)
    {
      Fifo_Read(&AsebaFifo.rx, (unsigned char*)&len, 2);
      Fifo_Read(&AsebaFifo.rx, (unsigned char*)source, 2);

      // msg_type is not in the len but is always present
      len = len + 2;
      /* Yay ! We have a complete packet ! */
      if (len > maxLength)
      {
        len = maxLength;
      }

      Fifo_Read(&AsebaFifo.rx, data, len);
      ret = len;
    }
  }
  if (connection_mode == MODE_USB)
  {
    USBCDCKickRx();
  }

  IRQ_ENABLE(flags);
  return ret;
#endif
}

//_____________________________________________________________________________

void AsebaFifoInit(unsigned char* sendQueue, size_t sendQueueSize, unsigned char* recvQueue, size_t recvQueueSize)
{
  AsebaFifo.tx.buffer = sendQueue;
  AsebaFifo.tx.size = sendQueueSize;

  AsebaFifo.rx.buffer = recvQueue;
  AsebaFifo.rx.size = recvQueueSize;
}

//_____________________________________________________________________________

int AsebaFifoRecvBufferEmpty(void)
{
  // We are called with interrupt disabled ! Check if rx contain something meaningfull

  int u;

  u = Fifo8bits_GetNumberOfElements(&AsebaFifo.rx);
  if (u > 6)
  {
    int len;
    Fifo8bits_Peek(&AsebaFifo.rx, (unsigned char*)&len, 2);
    if (u >= len + 6)
    {
      return 0;
    }
  }
  return 1;
}

//_____________________________________________________________________________

int AsebaFifoTxBusy(void)
{
  return tx_busy;
}
