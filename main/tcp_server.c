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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>
#include <sys/param.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "mdns.h"
#include <lwip/netdb.h>

#include "tcp_server.h"

#include "behavior.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PORT_NUM         CONFIG_PORT_NUM

#define RX_BUFFER_SIZE   2000

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

T_FifoBytes* TCPFifoRx = NULL;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "tcp_server";

static const char* MDNS_Tag = "mdns";

static bool SocketIsAccepted = false;

static uint8_t RxBuffer[RX_BUFFER_SIZE];  // RECV_QUEUE_SIZE

static int sock;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static const char* get_mdns_hostname(void);

static void start_zeroconf_service(uint16_t port);

static int socket_loop(int socket);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void TCPServer_Init(void)
{
  TCPFifoRx = Fifo8bits_Create(RxBuffer, RX_BUFFER_SIZE);
}

//_____________________________________________________________________________

void TCPServer_RunTask(void)
{
  char addr_str[128];
  int addr_family;
  int ip_protocol;

  while (1)
  {
#ifdef CONFIG_IPV4
    struct sockaddr_in destAddr;

    destAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    destAddr.sin_family      = AF_INET;
    destAddr.sin_port        = htons(PORT_NUM);

    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 destAddr;

    bzero(&destAddr.sin6_addr.un, sizeof(destAddr.sin6_addr.un));
    destAddr.sin6_family = AF_INET6;
    destAddr.sin6_port = htons(PORT_NUM);
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

    int err = bind(listen_sock, (struct sockaddr*)&destAddr, sizeof(destAddr));

    if (err != 0)
    {
      ESP_LOGE(Tag, "Socket unable to bind: errno %d", errno);
      break;
    }

    ESP_LOGI(Tag, "Socket bound, port %d", PORT_NUM);

    err = listen(listen_sock, 1);

    if (err != 0)
    {
      ESP_LOGE(Tag, "Error occured during listen: errno %d", errno);
      break;
    }

    start_zeroconf_service(PORT_NUM);

    ESP_LOGI(Tag, "Socket listening");

    struct sockaddr_in6 sourceAddr;  // Large enough for both IPv4 or IPv6
    uint32_t addrLen = sizeof(sourceAddr);

    while (1)
    {
      sock = accept(listen_sock, (struct sockaddr*)&sourceAddr, (socklen_t*)&addrLen);

      if (sock < 0)
      {
        ESP_LOGE(Tag, "Unable to accept connection: errno %d", errno);
        break;
      }

      // Get the sender's IP address as string
      if (sourceAddr.sin6_family == PF_INET)
      {
        inet_ntoa_r(((struct sockaddr_in*)&sourceAddr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
      }
      else if (sourceAddr.sin6_family == PF_INET6)
      {
        inet6_ntoa_r(sourceAddr.sin6_addr, addr_str, sizeof(addr_str) - 1);
      }

      SocketIsAccepted = true;
      ESP_LOGI(Tag, "Socket accepted");
      socket_loop(sock);

      if (sock != -1)
      {
        ESP_LOGE(Tag, "Shutting down socket and restarting...");
        Leds_SetDebugBrightness(0u, 0u, MAX_BRIGHTNESS);
        SocketIsAccepted = false;
        shutdown(sock, 0);
        Behavior_Enable(B_MODE);
        close(sock);
      }
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

void TCPServer_ShutDownSocket(void)
{
  ESP_LOGE(Tag, "Shutting down socket");
  SocketIsAccepted = false;
  shutdown(sock, 0);
  close(sock);
}

//_____________________________________________________________________________

void TCPServer_Send(const uint8_t* data, uint16_t size)
{
  int numberBytes = send(sock, data, size, 0);

  if ((numberBytes < 0) && SocketIsAccepted)
  {
    ESP_LOGE(Tag, "Error occured during sending: errno %d", errno);
  }
}

//_____________________________________________________________________________

static const char* get_mdns_hostname(void)
{
  static char* hostname = NULL;

  if (!hostname)
  {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    if (asprintf(&hostname, "NotAThymio3-%02X%02X%02X%02X", mac[3], mac[4], mac[5], esp_random()) == -1)
    {
      abort();
    }
  }

  return hostname;
}

//_____________________________________________________________________________

static void start_zeroconf_service(uint16_t port)
{
  ESP_ERROR_CHECK(mdns_init());
  ESP_ERROR_CHECK(mdns_hostname_set(get_mdns_hostname()));

  ESP_LOGI(MDNS_Tag, "mdns hostname set to: [%s]", get_mdns_hostname());
  mdns_instance_name_set("Not A Thymio 3");

  mdns_txt_item_t serviceTxtData[2] =
  {
    {"type", "Thymio II"},
    {"protovers", "9"}
  };

  ESP_ERROR_CHECK(mdns_service_add("Not A Thymio 3", "_aseba", "_tcp", port, serviceTxtData, 2));
}

//_____________________________________________________________________________

static int socket_loop(int socket)
{
  // Loop reading data
  uint8_t rx_buffer[RX_BUFFER_SIZE];

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
      // Fill the FIFO
      Fifo8bits_Write(TCPFifoRx, rx_buffer, len);
    }
  }

  return 0;
}
