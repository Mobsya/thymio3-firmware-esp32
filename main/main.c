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
#include "ir_receiver.h"
#include "leds.h"
#include "mode.h"
#include "mp3.h"
#include "power.h"
#include "sensors.h"
#include "sound.h"
//#include "sound_data.h"
#include "test.h"
#include "timer_sw.h"
//#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"

#include "i2s.h"
#include "stm32_i2c.h"

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

//  Buttons_Init();

  //I2S_Init();
//  Sound_Init();
  //Sound_Process();
  //I2S_Process();

  //I2S_Record();
  //Sound_Replay();

  //IRReceiver_Init();

  I2C_Init();
  Sensors_Init();
  Comm_Init();

  Fifo8bits_Init();
  Fifo16bits_Init();
  FifoFloat_Init();

  //Test_Run();

  Behavior_Init();

  Behavior_Enable(B_ALWAYS);  // FIXME Replace by Mode_Init()

//  Mode_Init();
//  Mode_InitVM();

//  AsebaTimer0 = TimerSw_Create(0, Callback_AsebaTimer0);
//  AsebaTimer1 = TimerSw_Create(0, Callback_AsebaTimer1);

  WIFI_Init();

  while (!WIFI_IsConnected())
  {}


//*****************************************************************************
// Start the tasks
//*****************************************************************************

  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "******* Tasks *******");
  ESP_LOGI(Tag, "*********************");

  ESP_LOGI(Tag, "OTA");

  //Test_StartDebugging();
  //I2S_StartReading();

  //Codec_StartMP3Player(0);
  WIFI_Start();
//  AsebaESP32_Start();

  //Sound_StartProcessing();

//  Behavior_Start();

//  IRReceiver_Start();

  //Sensors_Start();
  Comm_Start();

  //Buttons_Start();
  Leds_Start();
  //Sound_StartAcquisition();

  return 0;
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_poweroff =
{
  "_poweroff",
  "Poweroff",
  {
    {0, 0}
  }
};

//_____________________________________________________________________________

void power_off(AsebaVMState* vm)
{
  unsigned int flags;

  switch_off();

  // Protect against two racing poweroff:
  //  One from the softirq (button)
  //  One from the VM
#if 0
  RAISE_IPL(flags, 1);

  Behavior_Disable(B_ALL);

  play_sound_block(SOUND_POWEROFF);

  // Shutdown all peripherals ...
  switch_off();

  // Switch off USB
  // If we are connected to a PC, disconnect.
  // If we are NOT connected to a PC but 5V is present
  // ( == charger ) we need to keep the transciever on
  if (usb_uart_configured())
  {
    USBDeviceDetach();
  }

  // In any case, disable the usb interrupt. It's safer
  _USB1IE = 0;


  CHARGE_ENABLE_DIR = 1;

  analog_enter_poweroff_mode();
#endif
}

//_____________________________________________________________________________

void switch_off(void)
{
  //STM32_UpdateLeftMotorTarget(0);
  //STM32_UpdateRightMotorTarget(0);
}
