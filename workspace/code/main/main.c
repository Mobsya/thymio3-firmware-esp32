//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    main.c
//! \brief   This module provides the useful functions to use the xxx
//!
//! \author  Vincent Gonet
//!
//! \version $Id: main.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "aseba_esp32.h"
#include "behavior.h"
#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "mode.h"
#include "mp3.h"
#include "power.h"
#include "prox_ir.h"
#include "sensors.h"
#include "sound.h"
#include "timer.h"
#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"

// I2C modules
#include "accelerometer.h"
#include "color_sensor.h"
#include "gyroscope.h"
#include "stm32.h"

#include "board.h"  // FIXME only for debug

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DEFAULT_WIFI_SSID           CONFIG_WIFI_SSID
#define DEFAULT_WIFI_PASSWORD       CONFIG_WIFI_PASSWORD

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

static T_Settings Settings;
static T_Settings OldSettings;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void FeedWatchdog(void);

static void Settings_Init(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void BehaviorTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Behavior Task");

  while (1)
  {
    Behavior_Run();

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void SoundTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Sound Task");

  //Sound_Init();
  //MP3_Init();
//#if 0
  //while (1)
  {
    Sound_Task();

    FeedWatchdog();
    //vTaskDelay(50 / portTICK_PERIOD_MS);
  }
//#endif
}

//_____________________________________________________________________________

void SensorTask(void* pvParameter)
{
  uint32_t result = 0;

  ESP_LOGI(Tag, "Start Sensor Task");

  //Leds_Init();

  // Attempt to create a notification
  TaskToNotify = xTaskGetCurrentTaskHandle();

  Timer_Start(0, 0);
  Timer_Start(0, 1);

  while (1)
  {
    result = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (result == 1)
    {
	  Leds_RunTask();
	  Sensors_RunTask();
    }

    //taskYIELD();

    FeedWatchdog();
  }
}

//_____________________________________________________________________________

void CommTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Comm Task");

#if 0
  ColorSensor_Init();
  Accelerometer_Init();
  Compass_Init();
  Gyroscope_Init();
#endif

  int16_t vind[2] = {0, 0};
  int16_t current[2] = {0, 0};
  int16_t pwm[2] = {0, 0};
  int16_t button_raw[5] = {0, 0, 0, 0, 0};

  while (1)
  {
    STM32_ReadStatus();
	STM32_GetMotorCurrent(current);
	STM32_ReadBatteryVoltage();
    STM32_GetInducedVoltage(vind);
    STM32_GetPwmDutyCycle(pwm);
	STM32_ReadButtonStatus();
	STM32_GetButtonRawData(button_raw);

    ColorSensor_GetColor();
    Accelerometer_GetTapSource();
    Accelerometer_GetAcceleration();
    Gyroscope_GetAngularPosition();

#if 0
    if (Power_IsSwitchOffEnabled())
    {
      ESP_LOGI(Tag, "COUCOU");
      //Power_SwitchOff();
      Leds_SetTopBrightness(32, 0, 0);
    }
    else if (STM32_IsReadyToSwitchOff())
    {
      Power_EnableSwitchOff();
      Leds_SetSingleBrightness(E_Led_Battery_1, 32);
      STM32_AllowToSwitchOff();
      //Power_SwitchOff();
    }
#endif

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void AsebaTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Aseba Task");

  //UART_Init();
  AsebaESP32_Init();

  while (1)
  {
    AsebaESP32_Run();

    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________
#if 0
void WifiTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Wifi Task");
  //NVS_Init();
  //WifiUpdate_Init();
  //WifiUpdate_Connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

  WIFI_InitNVS();
  WIFI_Init();
//#if 0
  while (1)
  {
    //vTaskDelete(NULL);
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
//#endif
}
#endif
//_____________________________________________________________________________

int app_main(void)
{
  static int appCore1 = 0;
  static int appCore2 = 1;

#if 0
  WIFI_InitNVS();
  WIFI_Init();
  //WifiUpdate_Init();

  while (!WIFI_IsConnected())
  {}
#endif

  Settings_Init();

  Gpio_Init();
  Power_Init();

  UART_Init();
  I2C_Init();
  //I2S_Init();

  //Leds_Init();
  Sensors_Init();
  Mode_Init();
  Mode_InitVM();

  ColorSensor_Init();
  Accelerometer_Init();
  Gyroscope_Init();

#if 0
  //WIFI_InitNVS();
  //WIFI_Init();
  WIFIUpdate_InitNVS();
  WIFIUpdate_Init();
  //WifiUpdate_Connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

  //while (!WIFIUpdate_IsConnected())
  {}
#endif

  //MP3_Init();

  Timer_Init(0, 0, 1, 0.000125);  // Timer used to run the SensorTask
  Timer_Init(0, 1, 1, 1);         // Timer used to handle the IR_SENSE_BACK_RIGHT_PIN

//#if 0  // MP3 debug

//#if 0
  xTaskCreatePinnedToCore(
    BehaviorTask, // Function to implement the task
    "behavior",   // Name of the task
    4096,         // Stack size in words
    NULL,         // Task input parameter
    2,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
//#endif
#if 0
  xTaskCreatePinnedToCore(
    SoundTask,    // Function to implement the task
    "sound",      // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    3,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
#endif
//#if 0
  xTaskCreatePinnedToCore(
    CommTask,    // Function to implement the task
    "comm",   // Name of the task
    2048,        // Stack size in words
    NULL,        // Task input parameter
    3,           // Priority of the task
    NULL,        // Task handle
    appCore1);   // Core where the task should run
//#endif
#if 0
  xTaskCreatePinnedToCore(
    WifiTask,     // Function to implement the task
    "wifi",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    3,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
#endif
//#if 0
  xTaskCreatePinnedToCore(
    AsebaTask,   // Function to implement the task
    "aseba",     // Name of the task
    2048,        // Stack size in words
    NULL,        // Task input parameter
    4,           // Priority of the task
    NULL,        // Task handle
    appCore1);   // Core where the task should run
//#endif
//#if 0
  xTaskCreatePinnedToCore(
    SensorTask,  // Function to implement the task
    "sensor",    // Name of the task
    2048,        // Stack size in words
    NULL,        // Task input parameter
    9,           // Priority of the task
    NULL,        // Task handle
    appCore1);   // Core where the task should run
//#endif


//#endif  // MP3 debug

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

  behavior_stop(B_ALL);

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
#if 0
  static unsigned int old_timer[2];
  int i;

  for (i = 0; i < 2; i++)
  {
    if (vmVariables.timers[i] != old_timer[i])
    {
      old_timer[i] = vmVariables.timers[i];
      timer[i] = 0;
    }
  }

  pid_motor_set_target((int*) vmVariables.target);
#endif

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

static void FeedWatchdog(void)
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed     = 1u;
  TIMERG0.wdt_wprotect = 0u;
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

void AsebaVMResetCB(AsebaVMState *vm)
{
	Leds_SetSingleBrightness(E_Led_Battery_1, 32);
	Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
	Leds_SetBodyBrightness(0u, 0u, 0u);
#if 0 // FIXME
	leds_set(LED_SOUND,0);
	leds_set(LED_RC,0);
#endif
	Behavior_Start(B_LEDS_ACC);
#if 0 // FIXME
	behavior_start(B_LEDS_NTC);
	behavior_start(B_LEDS_MIC);
#endif
	Behavior_Start(B_LEDS_PROX);
#if 0 // FIXME
	behavior_start(B_SOUND_BUTTON);
	behavior_start(B_LEDS_MIC);
	behavior_start(B_LEDS_RC5);
	prox_disable_network();
	events_flags[0] = 0;
	events_flags[1] = 0;
	memset(vm->variables, 0, vm->variablesSize*sizeof(int16_t));
	vmVariables.id = vmState.nodeId;
	vmVariables.productid = PRODUCT_ID;
	vmVariables.fwversion[0] = FW_VERSION;
	vmVariables.fwversion[1] = FW_VARIANT;
	vmVariables.sd_present = !sd_user_open("_TESTSD");
	sd_user_open(NULL);
#endif
}
