//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    wifi.c
//! \brief   This module provides the useful functions to use the WIFI
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "lwip/sockets.h"

#include "wifi.h"

#include "ota_update.h"
#include "tcp_server.h"
#include "wifi_update.h"

#include "aseba_esp32.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

// Set the SSID and Password via "make menuconfig"
#define DEFAULT_WIFI_SSID           CONFIG_WIFI_SSID
#define DEFAULT_WIFI_PASSWORD       CONFIG_WIFI_PASSWORD

#if CONFIG_WIFI_ALL_CHANNEL_SCAN
#define DEFAULT_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#elif CONFIG_WIFI_FAST_SCAN
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#else
#define DEFAULT_SCAN_METHOD WIFI_FAST_SCAN
#endif  // CONFIG_SCAN_METHOD

#if CONFIG_WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_WIFI_CONNECT_AP_BY_SECURITY
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#else
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#endif  // CONFIG_SORT_METHOD

#if CONFIG_FAST_SCAN_THRESHOLD
#define DEFAULT_RSSI CONFIG_FAST_SCAN_MINIMUM_SIGNAL
#if CONFIG_EXAMPLE_OPEN
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#elif CONFIG_EXAMPLE_WEP
#define DEFAULT_AUTHMODE WIFI_AUTH_WEP
#elif CONFIG_EXAMPLE_WPA
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA_PSK
#elif CONFIG_EXAMPLE_WPA2
#define DEFAULT_AUTHMODE WIFI_AUTH_WPA2_PSK
#else
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif
#else
#define DEFAULT_RSSI -127
#define DEFAULT_AUTHMODE WIFI_AUTH_OPEN
#endif  // CONFIG_FAST_SCAN_THRESHOLD

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Update_Status_ReadComplete = 0,
  E_Update_Status_ReadTimeout,
  E_Update_Status_Disconnected,
  E_Update_Status_Error
} T_Update_Status;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "wifi";

// Indicates that we should trigger a re-boot after sending the response.
static int RebootAfterReply;

static bool WifiIsConnected = false;

static uint32_t IpForAseba = 0u;

static EventGroupHandle_t EventGroup;

#define NORM_C(c) (((c) >= 32 && (c) < 127) ? (c) : '.')

const int WIFI_CONNECTED_BIT = BIT0;
const int WIFI_UPDATE_CONNECTED_BIT = BIT0;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunWifiTask(void* arg);

static void RunUpdateTask(void* arg);

static esp_err_t EventHandler(void* ctx, system_event_t* event);

static int NetworkReceive(int s, char* buf, int maxLen, int* actualLen);

static void ProcessMessage(const char* message, int messageLen, char* responseBuf, int responseBufLen);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void WIFI_Configure(void)
{
  EventGroup = xEventGroupCreate();

  // Initialize the TCP Stack
  tcpip_adapter_init();

  // Initialize the system event handler
  ESP_ERROR_CHECK(esp_event_loop_init(EventHandler, NULL));

  // Configure the WIFI
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  wifi_config_t wifi_config =
  {
    .sta =
    {
      .ssid = CONFIG_WIFI_SSID,
      .password = CONFIG_WIFI_PASSWORD,
      //.scan_method = DEFAULT_SCAN_METHOD,
      //.sort_method = DEFAULT_SORT_METHOD,
      //.threshold.rssi = DEFAULT_RSSI,
      //.threshold.authmode = DEFAULT_AUTHMODE,
      .bssid_set = false
    },
  };

  // WIFI as Station Mode (connect to another wifi)
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Configure RAM as the WIFI parameters storage
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  //ESP_ERROR_CHECK(esp_wifi_connect());

  ESP_LOGI(Tag, "WIFI is configured");
}

//_____________________________________________________________________________

void WIFI_Init(void)
{
  TCPServer_Init();

//#if 0  // FIXME
  WIFI_InitNVS();
  WIFI_Configure();
  //WIFIUpdate_Init();
  //WIFI_WaitForIP();
//#endif

  //xTaskCreate(&RunUpdateTask, "NetworkTask", 32768, NULL, 5, NULL);

  OtaUpdate_Init();

  ESP_LOGI(Tag, "WIFI is initialized");
}

//_____________________________________________________________________________

void WIFI_InitNVS(void)
{
  esp_err_t error = nvs_flash_init();

  if ((error == ESP_ERR_NVS_NO_FREE_PAGES) || (error == ESP_ERR_NVS_NEW_VERSION_FOUND))
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    error = nvs_flash_init();
  }

  ESP_ERROR_CHECK(error);
}

//_____________________________________________________________________________

void WIFI_Start(void)
{
//#if 0
  xTaskCreatePinnedToCore(
    RunWifiTask,  // Function to implement the task
    "wifi",       // Name of the task
    4096,         // Stack size in words
    NULL,         // Task input parameter
    1,            // Priority of the task
    NULL,         // Task handle
    1);           // Core where the task should run
//#endif

//#if 0
  xTaskCreatePinnedToCore(
    RunUpdateTask,  // Function to implement the task
    "update",       // Name of the task
    32768,          // Stack size in words
    NULL,           // Task input parameter
    1,              // Priority of the task
    NULL,           // Task handle
    1);             // Core where the task should run
//#endif
}

//_____________________________________________________________________________

void WIFI_Connect(void)
{
  ESP_ERROR_CHECK(esp_wifi_connect());
}

//_____________________________________________________________________________

void WIFI_Disconnect(void)
{
  ESP_ERROR_CHECK(esp_wifi_disconnect());
}

//_____________________________________________________________________________

bool WIFI_IsConnected(void)
{
  return WifiIsConnected;
}

//_____________________________________________________________________________

void WIFI_GetIPAddress(void)
{
  vmVariables.ip[0] = (IpForAseba & 0x000000FF);
  vmVariables.ip[1] = ((IpForAseba & 0x0000FF00) >> 8);
  vmVariables.ip[2] = ((IpForAseba & 0x00FF0000) >> 16);
  vmVariables.ip[3] = ((IpForAseba & 0xFF000000) >> 24);
}

//_____________________________________________________________________________
#if 0
void WIFI_WaitForIP(void)
{
  uint32_t bits = (IPV4_GOTIP_BIT | IPV6_GOTIP_BIT);

  ESP_LOGI(Tag, "Waiting for AP connection...");
  xEventGroupWaitBits(EventGroup, bits, false, true, portMAX_DELAY);

  ESP_LOGI(Tag, "Connected to AP");
  WifiIsConnected = true;
}
#endif
//_____________________________________________________________________________

static void RunWifiTask(void* arg)
{
  ESP_LOGI(Tag, "Start Wifi Task");

  TCPServer_RunTask();
}

//_____________________________________________________________________________

static void RunUpdateTask(void* arg)
{
  const int maxRequestLen = 10000;
  const int maxResponseLen = 1000;
  const int tcpPort = 80;

  ESP_LOGI(Tag, "NetworkTask");

  while (1)
  {
    // Barrier for the connection (we need to be connected to an AP).
    xEventGroupWaitBits(EventGroup, WIFI_UPDATE_CONNECTED_BIT, false, true, portMAX_DELAY);
    ESP_LOGI(Tag, "NetworkTask: connected to access point");

    // Create TCP socket.
    int s = socket(AF_INET, SOCK_STREAM, 0);

    if (s < 0)
    {
      ESP_LOGE(Tag, "NetworkTask: failed to create socket: %d (%s)", errno, strerror(errno));
      vTaskDelay(1000 / portTICK_RATE_MS);
      continue;
    }

    // Bind socket to port.
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(struct sockaddr_in));
    serverAddr.sin_len = sizeof(struct sockaddr_in);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(tcpPort);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int b = bind(s, (struct sockaddr*)&serverAddr, sizeof(struct sockaddr_in));

    if (b < 0)
    {
      ESP_LOGE(Tag, "NetworkTask: failed to bind socket %d: %d (%s)", s, errno, strerror(errno));
      vTaskDelay(1000 / portTICK_RATE_MS);
      continue;
    }

    // Listen to incoming connections.
    ESP_LOGD(Tag, "NetworkTask: 'listen' on socket %d", s);
    listen(s, 1);  // backlog max. 1 connection

    while (1)
    {
      // Accept the connection on a separate socket.
      ESP_LOGD(Tag, "--------------------");
      ESP_LOGD(Tag, "NetworkTask: 'accept' on socket %d", s);
      struct sockaddr_in clientAddr;
      socklen_t clen = sizeof(clientAddr);
      int s2 = accept(s, (struct sockaddr*)&clientAddr, &clen);

      if (s2 < 0)
      {
        ESP_LOGE(Tag, "NetworkTask: 'accept' failed: %d (%s)", errno, strerror(errno));
        vTaskDelay(1000 / portTICK_RATE_MS);
        break;
      }

      // Would normally fork here.
      // For the moment, we support only a single open connection at any time.
      do
      {
        //ESP_LOGD(Tag, "NetworkTask: waiting for data on socket %d...", s2);

        // Allocate and clear memory for the request data.
        char* requestBuf = malloc(maxRequestLen * sizeof(char));

        if (!requestBuf)
        {
          ESP_LOGE(Tag, "NetworkTask: malloc for requestBuf failed: %d (%s)", errno, strerror(errno));
          break;
        }

        bzero(requestBuf, maxRequestLen);

        // Read the request and store it in the allocated buffer.
        int totalRequestLen = 0;
        T_Update_Status result = NetworkReceive(s2, requestBuf, maxRequestLen, &totalRequestLen);

        if (result != E_Update_Status_ReadComplete)
        {
          ESP_LOGI(Tag, "nothing more to, closing socket %d", s2);
          free(requestBuf);
          close(s2);
          break;
        }

        // Read completed successfully.
        // Process the request and create the response.

        ESP_LOGI(Tag, "NetworkTask: received %d bytes: %02x %02x %02x %02x ... | %c%c%c%c...",
                 totalRequestLen,
                 requestBuf[0], requestBuf[1], requestBuf[2], requestBuf[3],
                 NORM_C(requestBuf[0]), NORM_C(requestBuf[1]), NORM_C(requestBuf[2]), NORM_C(requestBuf[3]));


        char* responseBuf = malloc(maxResponseLen * sizeof(char));
        ProcessMessage(requestBuf, totalRequestLen, responseBuf, maxResponseLen);

        free(requestBuf);

        // Send the response back to the client.
        int totalLen = strlen(responseBuf);
        int nofWritten = 0;
        ESP_LOGD(Tag, "networkTask: write %d bytes to socket %d: %02x %02x %02x %02x ... | %c%c%c%c...", totalLen, s2,
                 responseBuf[0], responseBuf[1], responseBuf[2], responseBuf[3],
                 NORM_C(responseBuf[0]), NORM_C(responseBuf[1]), NORM_C(responseBuf[2]), NORM_C(responseBuf[3]));

        do
        {
          int n = write(s2, &responseBuf[nofWritten], totalLen - nofWritten);
          int e = errno;
          //ESP_LOGD(TAG, "networkTask: write: socket %d, n = %d, errno = %d", s2, n, e);

          if (n > 0)
          {
            nofWritten += n;

            // More to write?
            if (totalLen - nofWritten == 0)
            {
              break;
            }
          }
          else if (n == 0)
          {
            // Disconnected?
            break;
          }
          else
          {
            // n < 0
            if (e == EAGAIN)
            {
              //ESP_LOGD(TAG, "networkTask: write: EAGAIN");
              continue;
            }

            ESP_LOGE(Tag, "NetworkTask: write failed: %d (%s)", errno, strerror(errno));
            break;
          }
        }
        while (1);

        free(responseBuf);

        if (RebootAfterReply)
        {
          ESP_LOGI(Tag, "NetworkTask: Reboot in 2 seconds...");
          vTaskDelay(2000 / portTICK_RATE_MS);
          esp_restart();
        }
      }
      while (1);
    }

    // Should never arrive here
    close(s);
    vTaskDelay(2000 / portTICK_RATE_MS);
  }
}

//_____________________________________________________________________________

static esp_err_t EventHandler(void* ctx, system_event_t* event)
{
  uint16_t apCount = 0;

  switch (event->event_id)
  {
    case SYSTEM_EVENT_STA_START:
      ESP_ERROR_CHECK(esp_wifi_connect());
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_START");
      break;

    //case SYSTEM_EVENT_STA_CONNECTED:
    /* enable ipv6 */
    //tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
    //break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_ERROR_CHECK(esp_wifi_connect());
      xEventGroupClearBits(EventGroup, IPV4_GOTIP_BIT);
      xEventGroupClearBits(EventGroup, IPV6_GOTIP_BIT);
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_DISCONNECTED");
      break;

    case SYSTEM_EVENT_STA_GOT_IP:
      //const char* ip = ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip);
      //IpForAseba = ipaddr_addr(ip);

      //ESP_LOGI(Tag, "got ip:%s", ip);
      //ESP_LOGI(Tag, "%d", ip_uint);
      //ESP_LOGI(Tag, "got ip:%s", ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));

      WifiIsConnected = true;

      xEventGroupSetBits(EventGroup, IPV4_GOTIP_BIT);
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_GOT_IP");
      break;

    //case SYSTEM_EVENT_AP_STACONNECTED:
    //ESP_LOGI(Tag, "station:"MACSTR" join, AID=%d",
    //         MAC2STR(event->event_info.sta_connected.mac),
    //         event->event_info.sta_connected.aid);
    //break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(Tag, "station:"MACSTR"leave, AID=%d",
               MAC2STR(event->event_info.sta_disconnected.mac),
               event->event_info.sta_disconnected.aid);
      break;

    //case SYSTEM_EVENT_AP_PROBEREQRECVED:
    //ESP_LOGI(Tag, "SYSTEM_EVENT_AP_STADISCONNECTED: " MACSTR " rssi=%d",
    //         MAC2STR(event->event_info.ap_probereqrecved.mac),
    //         event->event_info.ap_probereqrecved.rssi);
    //break;

    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      xEventGroupSetBits(EventGroup, IPV6_GOTIP_BIT);
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_GOT_IP6");

      char* ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
      ESP_LOGI(Tag, "IPv6: %s", ip6);
      break;

    default:
      break;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static int NetworkReceive(int s, char* buf, int maxLen, int* actualLen)
{
  bool readAgain = false;
  int totalLen = 0;

  //ESP_LOGI(Tag, "NetworkReceive: start maxlen = %d", maxLen);

  for (int timeoutCtr = 0; timeoutCtr < 3000; timeoutCtr++)
  {
    readAgain = false;

    do
    {
      buf[totalLen] = 0x00;
      int n = recv(s, &buf[totalLen], maxLen - totalLen, MSG_DONTWAIT);
      int e = errno;

      // Error?
      if (n > 0)
      {
        // Message complete?
        totalLen += n;

        if (totalLen > 0)
        {
          // We currently support two record types:
          // Records that start with !xxxx where x is a hexadecimal length indicator
          // Records that start with something else and are terminated with a newline
          int recordLen = 0;
          int recordWithLengthIndicator = (1 == sscanf(buf, "!%04x", &recordLen));
          ESP_LOGD(Tag, "NetworkReceive: recordWithLengthIndicator = %d, expected length = %d, current length = %d",
                   recordWithLengthIndicator, recordLen, totalLen);

          if ((recordWithLengthIndicator && totalLen == recordLen) ||
              (!recordWithLengthIndicator && buf[totalLen - 1] == '\n'))
          {
            ESP_LOGI(Tag, "NetworkReceive: received %d byte packet on socket %d", totalLen, s);
            *actualLen = totalLen;
            return E_Update_Status_ReadComplete;
          }
        }

        // Not yet complete. Read again immediately.
        readAgain = true;
      }
      else if (n < 0 && e == EAGAIN)
      {
        // No data available right now.
        // Wait for a short moment before trying again.
        readAgain = false;
      }
      else
      {
        // Error (n=0, n<0).
        ESP_LOGE(Tag, "recv n = %d, errno = %d (%s)", n, e, strerror(e));
        return E_Update_Status_Error;
      }
    }
    while (readAgain);

    // n == 0, wait a bit
    //ESP_LOGI(TAG, "NetworkReceive: wait for more data");
    vTaskDelay(10 / portTICK_RATE_MS);
  }

  return E_Update_Status_ReadTimeout;
}

//_____________________________________________________________________________

static void ProcessMessage(const char* message, int messageLen, char* responseBuf, int responseBufLen)
{
  // Response to send back to the TCP client.
  char response[256];
  sprintf(response, "OK\r\n");

  if (message[0] == '!')
  {
    T_Ota_Status result = E_Ota_Status_Ok;

    if (message[1] == '[')
    {
      ESP_LOGI(Tag, "ProcessMessage: OTA start");
      result = OtaUpdate_Start();
    }
    else if (message[1] == ']')
    {
      ESP_LOGI(Tag, "ProcessMessage: OTA end");
      result = OtaUpdate_Finish();
    }
    else if (message[1] == '*')
    {
      ESP_LOGI(Tag, "ProcessMessage: Reboot");
      RebootAfterReply = 1;
    }
    else
    {
      result = OtaUpdate_WriteHexData(&message[5], messageLen - 5);
    }

    if (result != E_Ota_Status_Ok)
    {
      ESP_LOGE(Tag, "ProcessMessage: OTA_ERROR %d", result);
      sprintf(response, "OTA_ERROR %d\r\n", result);
    }
  }
  else if (message[0] == '?')
  {
    OtaUpdate_DumpInformation();
  }

  strncpy(responseBuf, response, responseBufLen);
}
