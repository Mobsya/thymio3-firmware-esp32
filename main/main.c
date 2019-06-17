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

#include "adc.h"
#include "aseba_esp32.h"
//#include "audio.h"
#include "behavior.h"
#include "comm.h"
#include "cosine_generator.h"
#include "fifo.h"
#include "file_system.h"
#include "gpio.h"
#include "ground_ir.h"
#include "leds.h"
#include "mode.h"
#include "mp3.h"
#include "power.h"
#include "prox_ir.h"
#include "sound.h"
//#include "sound_data.h"
#include "stm32.h"
#include "test.h"
#include "timer_sw.h"
//#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"

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

static int16_t Target[2] = {0, 0};
static int16_t OldTarget[2] = {0, 0};

static int16_t TimerDuration[2] = {0, 0};
static int16_t OldTimerDuration[2] = {0, 0};

static T_Settings Settings;
static T_Settings OldSettings;

static T_TimerSw* AsebaTimer0 = NULL;  //!< Used to schedule the Aseba timer 0
static T_TimerSw* AsebaTimer1 = NULL;  //!< Used to schedule the Aseba timer 0

//T_Wav Wav;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void Settings_Init(void);

static void UpdateMotorTargets(void);

static void UpdateTimers(void);

static void UpdateSettings(void);

static void Callback_AsebaTimer0(void* arg);

static void Callback_AsebaTimer1(void* arg);

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

  Settings_Init();
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
  Power_Init();

  ADC_Init();

  Leds_Init();

  GroundIR_Init();
  ProxIR_Init();

  Sound_Init();
  //Sound_Process();
  //I2S_Process();

  //I2S_Record();
  //Sound_Replay();

  Comm_Init();

  Fifo8bits_Init();
  Fifo16bits_Init();
  FifoFloat_Init();

  //Test_Run();

  Behavior_Init();

  Mode_Init();
  Mode_InitVM();

  AsebaTimer0 = TimerSw_Create(0, Callback_AsebaTimer0);
  AsebaTimer1 = TimerSw_Create(0, Callback_AsebaTimer1);

  WIFI_Init();

  while (!WIFI_IsConnected())
  {}

//*****************************************************************************
// Start the tasks
//*****************************************************************************

  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "******* Tasks *******");
  ESP_LOGI(Tag, "*********************");

  //Test_StartDebugging();
  //I2S_StartReading();

  //MP3_StartPlayer(0);
  WIFI_Start();
  AsebaESP32_Start();

  //Sound_StartProcessing();

  Behavior_Start();
  Comm_Start();
  GroundIR_Start();
  ProxIR_Start();
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

void update_aseba_variables_write(void)
{
  if (Comm_IsBusAvailable())
  {
    UpdateMotorTargets();

    UpdateTimers();

    UpdateSettings();
  }
}

//_____________________________________________________________________________

void update_aseba_variables_read(void)
{
  // TODO: REMOVE ME (move to behavior ? /!\ behavior == IPL 1 !! race wrt aseba !)
#if 0
  usb_uart_tick();

  motor_get_vind((int*) vmVariables.uind);
#endif
  //WIFI_GetIPAddress();
  WIFIUpdate_GetIPAddress();
}

//_____________________________________________________________________________

void switch_off(void)
{
  //STM32_UpdateLeftMotorTarget(0);
  //STM32_UpdateRightMotorTarget(0);
}

//_____________________________________________________________________________

static void Settings_Init()
{
  Settings.LeftMotor  = 256;
  Settings.RightMotor = 256;

  OldSettings.LeftMotor  = 0;
  OldSettings.RightMotor = 0;
}

//_____________________________________________________________________________

static void UpdateMotorTargets(void)
{
  Target[0] = vmVariables.target[0];
  Target[1] = vmVariables.target[1];

  if (Target[0] != OldTarget[0])
  {
    STM32_UpdateLeftMotorTarget(Target);
    OldTarget[0] = Target[0];
  }

  if (Target[1] != OldTarget[1])
  {
    STM32_UpdateRightMotorTarget(Target);
    OldTarget[1] = Target[1];
  }
}

//_____________________________________________________________________________

static void UpdateTimers(void)
{
  TimerDuration[0] = vmVariables.timers[0];
  TimerDuration[1] = vmVariables.timers[1];

  if (TimerDuration[0] != OldTimerDuration[0])
  {
    TimerSw_StopTimer(AsebaTimer0);

    OldTimerDuration[0] = TimerDuration[0];

    if (TimerDuration[0] > 0)
    {
      TimerSw_StartTimerPeriodically(AsebaTimer0, TimerDuration[0] * 1000);
    }
  }

  if (TimerDuration[1] != OldTimerDuration[1])
  {
    TimerSw_StopTimer(AsebaTimer1);

    OldTimerDuration[1] = TimerDuration[1];

    if (TimerDuration[1] > 0)
    {
      TimerSw_StartTimerPeriodically(AsebaTimer1, TimerDuration[1] * 1000);
    }
  }
}

//_____________________________________________________________________________

static void UpdateSettings(void)
{
  Settings.LeftMotor  = vmVariables.settings[0];
  Settings.RightMotor = vmVariables.settings[1];

  if ((Settings.LeftMotor != OldSettings.LeftMotor) ||
      (Settings.RightMotor != OldSettings.RightMotor))
  {
    STM32_UpdateSettings(Settings);
    OldSettings.LeftMotor  = Settings.LeftMotor;
    OldSettings.RightMotor = Settings.RightMotor;
  }
}

//_____________________________________________________________________________

static void Callback_AsebaTimer0(void* arg)
{
  SET_EVENT(EVENT_TIMER0);
}

//_____________________________________________________________________________

static void Callback_AsebaTimer1(void* arg)
{
  SET_EVENT(EVENT_TIMER1);
}

//_____________________________________________________________________________

void AsebaVMResetCB(AsebaVMState* vm)
{
  //Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetBodyBrightness(0u, 0u, 0u);
#if 0 // FIXME
  leds_set(LED_SOUND, 0);
  leds_set(LED_RC, 0);
#endif
  Behavior_Enable(B_LEDS_ACC);
#if 0 // FIXME
  Behavior_Enable(B_LEDS_TEMPERATURE);
  Behavior_Enable(B_LEDS_MIC);
#endif
  Behavior_Enable(B_LEDS_PROX);
  Behavior_Enable(B_SOUND_BUTTON);
#if 0 // FIXME
  Behavior_Enable(B_LEDS_MIC);
  Behavior_Enable(B_LEDS_RC5);
  prox_disable_network();
  events_flags[0] = 0;
  events_flags[1] = 0;
  memset(vm->variables, 0, vm->variablesSize * sizeof(int16_t));
  vmVariables.id = vmState.nodeId;
  vmVariables.productid = PRODUCT_ID;
  vmVariables.fwversion[0] = FW_VERSION;
  vmVariables.fwversion[1] = FW_VARIANT;
#endif
}
