//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    tcp_server.c
//! \brief   This module provides the useful functions to use the TCP server
//!
//! \author  Vincent Gonet
//!
//! \version $Id: tcp_server.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "tcp_server.h"

#include "wifi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PORT    CONFIG_PORT

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "tcp_server";

static bool SocketIsAccepted = false;

char rx_buffer[512];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TCPServer_Init(void)
{
  WIFI_InitNVS();
  WIFI_Init();
  WIFI_WaitForIP();
}

//_____________________________________________________________________________

void TCPServer_RunTask(void)
{
  //char rx_buffer[512];
  char addr_str[128];
  int addr_family;
  int ip_protocol;

  while (1)
  {
#ifdef CONFIG_IPV4
    struct sockaddr_in destAddr;

    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family = AF_INET;
    destAddr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 destAddr;

    bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(destAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

    if (listen_sock < 0)
    {
      ESP_LOGE(Tag, "Unable to create socket: errno %d", errno);
      break;
    }

    ESP_LOGI(Tag, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&destAddr, sizeof(destAddr));

    if (err != 0)
    {
      ESP_LOGE(Tag, "Socket unable to bind: errno %d", errno);
      break;
    }

    ESP_LOGI(Tag, "Socket binded");

    err = listen(listen_sock, 1);

    if (err != 0)
    {
      ESP_LOGE(Tag, "Error occured during listen: errno %d", errno);
      break;
    }

    ESP_LOGI(Tag, "Socket listening");

    struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6

    uint addrLen = sizeof(sourceAddr);
    int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);

    if (sock < 0)
    {
      ESP_LOGE(Tag, "Unable to accept connection: errno %d", errno);
      break;
    }

    SocketIsAccepted = true;
    ESP_LOGI(Tag, "Socket accepted");

    // Loop reading data
    while (1)
    {
      int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

      // Error occured during receiving
      if (len < 0)
      {
        ESP_LOGE(Tag, "recv failed: errno %d", errno);
        break;
      }
      else if (len == 0)  // Connection closed
      {
        ESP_LOGI(Tag, "Connection closed");
        break;
      }
      else  // Data received
      {
        // Get the sender's ip address as string
        if (sourceAddr.sin6_family == PF_INET)
        {
          inet_ntoa_r(((struct sockaddr_in *)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        }
        else if (sourceAddr.sin6_family == PF_INET6)
        {
          inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }

        rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
//#if 0  // FIXME
        ESP_LOGI(Tag, "Received %d bytes from %s:", len, addr_str);
        ESP_LOGI(Tag, "%s", rx_buffer);

        //int err = send(sock, rx_buffer, len, 0);  // FIXME Don't send data to Aseba Studio

        if (err < 0)
        {
          ESP_LOGE(Tag, "Error occured during sending: errno %d", errno);
          break;
        }
//#endif
      }
    }

    if (sock != -1)
    {
      ESP_LOGE(Tag, "Shutting down socket and restarting...");
      shutdown(sock, 0);
      close(sock);
    }
  }
  vTaskDelete(NULL);
}

//_____________________________________________________________________________

bool TCPServer_IsSocketAccepted(void)
{
  return SocketIsAccepted;
}

//_____________________________________________________________________________

void TCPServer_Send(const uint8_t* data, uint16_t length)
{
  int addr_family = AF_INET;
  int ip_protocol = IPPROTO_IP;

  int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);

  struct sockaddr_in6 sourceAddr; // Large enough for both IPv4 or IPv6

  uint addrLen = sizeof(sourceAddr);
  int sock = accept(listen_sock, (struct sockaddr *)&sourceAddr, &addrLen);

  ESP_LOGI(Tag, "%s", rx_buffer);
  int err = send(sock, data, length, 0);

  if (err < 0)
  {
    ESP_LOGE(Tag, "Error occured during sending: errno %d", errno);
  }
}

//_____________________________________________________________________________

uint8_t* TCPServer_GetRxBuffer(void)
{
  //ESP_LOGI(Tag, "%s", rx_buffer);
  return (uint8_t*)rx_buffer;
}
