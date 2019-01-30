//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: wifi.c 18076 2017-04-20 12:28:12Z v.gonet $
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

#include "wifi.h"

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
#endif /*CONFIG_SCAN_METHOD*/

#if CONFIG_WIFI_CONNECT_AP_BY_SIGNAL
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#elif CONFIG_WIFI_CONNECT_AP_BY_SECURITY
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SECURITY
#else
#define DEFAULT_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#endif /*CONFIG_SORT_METHOD*/

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
#endif /*CONFIG_FAST_SCAN_THRESHOLD*/

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "wifi";

static bool WifiIsConnected = false;

static uint32_t IpForAseba = 0u;

static EventGroupHandle_t EventGroup;

const int WIFI_CONNECTED_BIT = BIT0;

const int IPV4_GOTIP_BIT = BIT0;
const int IPV6_GOTIP_BIT = BIT1;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static esp_err_t EventHandler(void* ctx, system_event_t* event);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void WIFI_Configure(void)
{
  WIFI_InitNVS();
  WIFI_Init();
  WIFIUpdate_Init();
  WIFI_WaitForIP();
}

//_____________________________________________________________________________

void WIFI_Init(void)
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
      //.bssid_set = false
    },
  };

  // WIFI as Station Mode (connect to another wifi)
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Configure RAM as the WIFI parameters storage
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  ESP_ERROR_CHECK(esp_wifi_connect());
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

void WIFI_WaitForIP(void)
{
  uint32_t bits = (IPV4_GOTIP_BIT | IPV6_GOTIP_BIT);

  ESP_LOGI(Tag, "Waiting for AP connection...");
  xEventGroupWaitBits(EventGroup, bits, false, true, portMAX_DELAY);

  ESP_LOGI(Tag, "Connected to AP");
  WifiIsConnected = true;
}

//_____________________________________________________________________________

static esp_err_t EventHandler(void* ctx, system_event_t* event)
{
  uint16_t apCount = 0;

  switch (event->event_id)
  {
    case SYSTEM_EVENT_SCAN_DONE:
      esp_wifi_scan_get_ap_num(&apCount);

      printf("Number of access points found: %d\n", event->event_info.scan_done.number);

      if (apCount > 0u)
      {
        wifi_ap_record_t* list = (wifi_ap_record_t*)malloc(sizeof(wifi_ap_record_t) * apCount);
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, list));

        printf("======================================================================\n");
        printf("             SSID             |    RSSI    |           AUTH           \n");
        printf("======================================================================\n");

        for (uint16_t i = 0u; i < apCount; i++)
        {
          char* authmode;

          switch (list[i].authmode)
          {
            case WIFI_AUTH_OPEN:
              authmode = "WIFI_AUTH_OPEN";
              break;

            case WIFI_AUTH_WEP:
              authmode = "WIFI_AUTH_WEP";
              break;

            case WIFI_AUTH_WPA_PSK:
              authmode = "WIFI_AUTH_WPA_PSK";
              break;

            case WIFI_AUTH_WPA2_PSK:
              authmode = "WIFI_AUTH_WPA2_PSK";
              break;

            case WIFI_AUTH_WPA_WPA2_PSK:
              authmode = "WIFI_AUTH_WPA_WPA2_PSK";
              break;

            default:
              authmode = "Unknown";
              break;
          }

          printf("%26.26s    |    % 4d    |    %22.22s\n", list[i].ssid, list[i].rssi, authmode);
        }
        free(list);
        printf("\n\n");
      }
      break;

    case SYSTEM_EVENT_STA_START:
      ESP_ERROR_CHECK(esp_wifi_connect());
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_START");
      break;

    case SYSTEM_EVENT_STA_CONNECTED:
      /* enable ipv6 */
      tcpip_adapter_create_ip6_linklocal(TCPIP_ADAPTER_IF_STA);
      break;

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

      xEventGroupSetBits(EventGroup, IPV4_GOTIP_BIT);
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_GOT_IP");
      break;

    case SYSTEM_EVENT_AP_STACONNECTED:
      ESP_LOGI(Tag, "station:"MACSTR" join, AID=%d",
               MAC2STR(event->event_info.sta_connected.mac),
               event->event_info.sta_connected.aid);
      break;

    case SYSTEM_EVENT_AP_STADISCONNECTED:
      ESP_LOGI(Tag, "station:"MACSTR"leave, AID=%d",
               MAC2STR(event->event_info.sta_disconnected.mac),
               event->event_info.sta_disconnected.aid);
      break;

    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      ESP_LOGI(Tag, "SYSTEM_EVENT_AP_STADISCONNECTED: " MACSTR " rssi=%d",
               MAC2STR(event->event_info.ap_probereqrecved.mac),
			   event->event_info.ap_probereqrecved.rssi);
      break;

    case SYSTEM_EVENT_AP_STA_GOT_IP6:
      xEventGroupSetBits(EventGroup, IPV6_GOTIP_BIT);
      ESP_LOGI(Tag, "SYSTEM_EVENT_STA_GOT_IP6");

      char *ip6 = ip6addr_ntoa(&event->event_info.got_ip6.ip6_info.ip);
      ESP_LOGI(Tag, "IPv6: %s", ip6);
      break;

    default:
      break;
  }

  return ESP_OK;
}
