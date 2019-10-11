//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    main.c
//! \brief   This module provides the useful functions to run the application
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "aseba.h"
#include "aseba_esp32.h"
//#include "audio.h"
#include "behavior.h"
#include "buttons.h"
#include "codec.h"
#include "comm.h"
#include "cosine_generator.h"
#include "fifo.h"
#include "file_system.h"
#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "mode.h"
#include "mp3.h"
#include "power.h"
#include "rc5.h"
#include "sensors.h"
#include "sound.h"
//#include "sound_data.h"
#include "test.h"
#include "timer_sw.h"
#include "tracking.h"
//#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"

#include "i2s.h"
#include "stm32_i2c.h"

#include "tcp_server.h"
#include "uart.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "main";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

int app_main(void)
{
//*****************************************************************************
// Initialization
//*****************************************************************************

  esp_log_level_set("*", ESP_LOG_INFO);

  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "** Initializations **");
  ESP_LOGI(Tag, "*********************");

//  Settings_Init();
#if 0
  FileSystem_Init();
  FileSystem_CreateSettingsFile();
  //FileSystem_ReadFile();
  FileSystem_ReadSettingsFile();
  FileSystem_WriteSettingsFile();
  FileSystem_ReadSettingsFile();
  FileSystem_UpdateSettings(29, 56);
  FileSystem_WriteSettingsFile();
  FileSystem_ReadSettingsFile();
#endif

  TimerSw_Init();

  Gpio_Init();

  Aseba_Init();

  Leds_Init();

  //Buttons_Init();

  //I2S_Init();
  //Sound_Init();
  //Sound_Process();
  //I2S_Process();
  //I2S_Record();
  //Sound_Replay();

  RC5_Init();

  I2C_Init();
  Sensors_Init();
  Comm_Init();

  Fifo8bits_Init();
  //Fifo16bits_Init();
  //FifoFloat_Init();

  //Test_Run();

  Behavior_Init();

  //Behavior_Enable(B_ALWAYS | B_LEDS_PROX);  // FIXME Replace by Mode_Init()

  Mode_Init();
  Mode_InitVM();

  WIFI_Init();
  //AsebaESP32_Init();

  while (!WIFI_IsConnected())
  {}

  esp_log_level_set("*", ESP_LOG_ERROR);

  Codec_SetVolume(100);
  //Codec_PlayMP3FromFileSystem(0);
  //Codec_PlayMP3(2);
  //Codec_RecordWAV(0);
  //Codec_PlayWAV(2);

  //Codec_SetVolume(80);

//*****************************************************************************
// Start the tasks
//*****************************************************************************

  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "******* Tasks *******");
  ESP_LOGI(Tag, "*********************");

  ESP_LOGI(Tag, "OTA");

  //Create semaphores to synchronize
  //sync_spin_task = xSemaphoreCreateCounting(1, 0);
  //sync_stats_task = xSemaphoreCreateBinary();

  //Tracking_Start();

  WIFI_Start();
  AsebaESP32_Start();

  //Test_StartDebugging();

  Behavior_Start();

  RC5_Start();

  Sensors_Start();
  Comm_Start();

  Leds_Start();

  //while (!TCPServer_IsTestFlagOk())
  //{}

  //ESP_LOGI(Tag, "All tasks are started");
  //UART_Flush();

  return 0;
}
