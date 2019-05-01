//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    wifi_update.c
//! \brief   This module provides the useful functions to use the WIFI for the update
//!
//! \author  Vincent Gonet
//!
//! \version $Id: wifi_update.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#include "nvs.h"
#include "nvs_flash.h"

#include "wifi_update.h"
#include "ota_update.h"

#include "aseba_esp32.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define FIRMWARE_UPGRADE_URL    CONFIG_FIRMWARE_UPGRADE_URL

const int CONNECTED_BIT = BIT0;

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

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "wifi_update";

// FreeRTOS event group to signal when we are connected
static EventGroupHandle_t EventGroup;

static uint32_t IpForAseba = 0u;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static esp_err_t HttpEventHandler(esp_http_client_event_t* event);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------
#if 0
void WIFIUpdate_InitNVS(void)
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

void WIFIUpdate_Init(void)
{
#if 0
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

  //ESP_LOGI(Tag, "WIFI configuration SSID %s...", wifi_config.sta.ssid);

  xTaskCreate(&NetworkTask, "NetworkTask", 32768, NULL, 5, NULL);
#endif
  OtaUpdate_Init();

  ESP_LOGI(Tag, "WIFI Update is initialized");
}

//_____________________________________________________________________________

void WifiUpdate_Connect(const char* ssid, const char* password)
{
  ESP_LOGI(Tag, "networkConnect %s", ssid);

  wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&config));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));

  wifi_config_t wifiConfig;
  strncpy((char*)wifiConfig.sta.ssid, ssid, sizeof(wifiConfig.sta.ssid) / sizeof(char));
  strncpy((char*)wifiConfig.sta.password, password, sizeof(wifiConfig.sta.password) / sizeof(char));
  wifiConfig.sta.bssid_set = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());
}

//_____________________________________________________________________________

void WifiUpdate_Disconnect(void)
{
  ESP_ERROR_CHECK(esp_wifi_stop());
}

//_____________________________________________________________________________

bool WIFIUpdate_IsConnected(void)
{
  return xEventGroupGetBits(EventGroup) & CONNECTED_BIT;
}
#endif
//_____________________________________________________________________________

void WIFIUpdate_GetIPAddress(void)
{
  vmVariables.ip[0] = (IpForAseba & 0x000000FF);
  vmVariables.ip[1] = ((IpForAseba & 0x0000FF00) >> 8);
  vmVariables.ip[2] = ((IpForAseba & 0x00FF0000) >> 16);
  vmVariables.ip[3] = ((IpForAseba & 0xFF000000) >> 24);
}

//_____________________________________________________________________________

static esp_err_t HttpEventHandler(esp_http_client_event_t* event)
{
  switch (event->event_id)
  {
    case HTTP_EVENT_ERROR:
      ESP_LOGD(Tag, "HTTP_EVENT_ERROR");
      break;

    case HTTP_EVENT_ON_CONNECTED:
      ESP_LOGD(Tag, "HTTP_EVENT_ON_CONNECTED");
      break;

    case HTTP_EVENT_HEADER_SENT:
      ESP_LOGD(Tag, "HTTP_EVENT_HEADER_SENT");
      break;

    case HTTP_EVENT_ON_HEADER:
      ESP_LOGD(Tag, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", event->header_key, event->header_value);
      break;

    case HTTP_EVENT_ON_DATA:
      ESP_LOGD(Tag, "HTTP_EVENT_ON_DATA, len=%d", event->data_len);
      break;

    case HTTP_EVENT_ON_FINISH:
      ESP_LOGD(Tag, "HTTP_EVENT_ON_FINISH");
      break;

    case HTTP_EVENT_DISCONNECTED:
      ESP_LOGD(Tag, "HTTP_EVENT_DISCONNECTED");
      break;

    default:
      break;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

void WIFIUpdate_RunTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start WIFI Update Task");

#if 0
  // Wait for the callback to set the CONNECTED_BIT in the event group
  xEventGroupWaitBits(EventGroup, CONNECTED_BIT,
                      false, true, portMAX_DELAY);
#endif

  ESP_LOGI(Tag, "Connected to WiFi network! Attempting to connect to server...");

  esp_http_client_config_t config =
  {
    .url = FIRMWARE_UPGRADE_URL,
    .cert_pem = (char*)server_cert_pem_start,
    .event_handler = HttpEventHandler,
  };

  esp_err_t ret = esp_https_ota(&config);

  if (ret == ESP_OK)
  {
    esp_restart();
  }
  else
  {
    ESP_LOGE(Tag, "Firmware upgrade failed");
  }

  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

