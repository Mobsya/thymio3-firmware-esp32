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

#include "driver/timer.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "behavior.h"
#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "mode.h"
#include "power.h"
#include "prox_ir.h"
#include "sensors.h"
#include "sound.h"
#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"

// I2C modules
#include "accelerometer.h"
#include "color_sensor.h"
#include "compass.h"
#include "gyroscope.h"
#include "stm32.h"

#include "aseba_esp32.h"

#include "timer_hw.h"
#include "timer.h"

#include "board.h"  // FIXME only for debug

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TIMER_DIVIDER 16 //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define DEFAULT_WIFI_SSID           CONFIG_WIFI_SSID
#define DEFAULT_WIFI_PASSWORD       CONFIG_WIFI_PASSWORD

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//SemaphoreHandle_t Timer125usSemaphore = NULL;

static TaskHandle_t TaskToNotify = NULL;

#if 0
SemaphoreHandle_t Timer10msSemaphore = NULL;
SemaphoreHandle_t Timer200msSemaphore = NULL;
#endif

//TaskHandle_t LedsHandle = NULL;

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

static void Timer_Init(int timer_idx, bool auto_reload, double timer_interval_sec);

static void Settings_Init(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

//#if 0
void BehaviorTask(void* pvParameter)
{
  static bool first = true;

  ESP_LOGI(Tag, "Start Behavior Task");

  while (1)
  {
    if (first)
    {
	  ESP_LOGI(Tag, "First run Behavior Task");
	  first = false;
	}

    Behavior_Run();
	Behavior_SetIRSensorsLeds();
	//ProxIR_ReadPulseDuration();
    vTaskDelay(80 / portTICK_PERIOD_MS);
  }
}
//#endif

//_____________________________________________________________________________

void SensorTask(void* pvParameter)
{
  uint32_t result = 0;
  static bool first = true;

  ESP_LOGI(Tag, "Start Sensor Task");

  // Attempt to create a semaphore
  //Timer125usSemaphore = xSemaphoreCreateBinary();
  TaskToNotify = xTaskGetCurrentTaskHandle();

  timer_start(TIMER_GROUP_0, 0);

  Leds_Init();

  while (1)
  {
#if 0
	if (Timer125usSemaphore != NULL)
	{
	  if (xSemaphoreTake(Timer125usSemaphore, 0) == pdTRUE) //portMAX_DELAY );
      {
	    if (first)
	    {
	      ESP_LOGI(Tag, "First run Sensor Task");
	      first = false;
	    }

	    Leds_RunTask();
	    Sensors_RunTask();

	    //Behavior_SetIRSensorsLeds();
	    //ESP_LOGI(Tag, "Run Leds Task");
      }
	  else
	  {
	    // There was insufficient FreeRTOS heap available for the semaphore to
	    // be created successfully
	  }
	}
#endif

    result = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (result == 1)
    {
	  Leds_RunTask();
	  Sensors_RunTask();
    }

    //taskYIELD();

    //FeedWatchdog();
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

  int16_t vbat[2] = {0, 0};
  int16_t vind[2] = {0, 0};
  int16_t current[2] = {0, 0};
  int16_t pwm[2] = {0, 0};
  int16_t button_raw[5] = {0, 0, 0, 0, 0};

  while (1)
  {
    STM32_ReadStatus();
	STM32_GetMotorCurrent(current);
	STM32_GetBatteryVoltage(vbat);
    STM32_GetInducedVoltage(vind);
    STM32_GetPwmDutyCycle(pwm);
	STM32_ReadButtonStatus();
	STM32_GetButtonRawData(button_raw);

    ColorSensor_GetColor();
#if 0
    Accelerometer_GetTapSource();
    Accelerometer_GetAcceleration();
    Compass_GetMagneticField();
    Gyroscope_GetAngularPosition();
#endif

    //STM32_CheckId();
    //STM32_GetButtonRawData(button_raw);

    //STM32_UpdateLeftMotorTarget(target);
    //STM32_UpdateRightMotorTarget(target);

    //STM32_GetLeftMotorTarget(target_read);
    //STM32_GetRightMotorTarget(target_read);
#if 0
    if (STM32_IsReadyToSwitchOff())
    {
      //Power_SwitchOff();
      Leds_SetSingleBrightness(E_Led_Battery_1, 32);
      STM32_AllowToSwitchOff();
    }
#endif

    Behavior_Run();
	Behavior_SetIRSensorsLeds();
	//ProxIR_ReadPulseDuration();

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void AsebaTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Aseba Task");

  UART_Init();
  AsebaESP32_Init();

  while (1)
  {
    AsebaESP32_Run();

    vTaskDelay(3 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________
#if 0
void SoundTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Sound Task");

  Sound_Init();

  while (1)
  {
    //Sound_Task();

    FeedWatchdog();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}
#endif
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
  I2C_Init();

  //Leds_Init();
  Sensors_Init();
  Mode_Init();

  ColorSensor_Init();
#if 0
  Accelerometer_Init();
  Compass_Init();
  Gyroscope_Init();
#endif

//#if 0
  //WIFI_InitNVS();
  //WIFI_Init();
  WIFIUpdate_InitNVS();
  WIFIUpdate_Init();
  //WifiUpdate_Connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

  //while (!WIFIUpdate_IsConnected())
  {}
//#endif

  Timer_Init(0, 1, 0.000125);  // TODO Move to Sensors_Init();



#if 0
  xTaskCreatePinnedToCore(
    BehaviorTask, // Function to implement the task
    "behavior",   // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    2,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
#endif
//#if 0
  xTaskCreatePinnedToCore(
    CommTask,    // Function to implement the task
    "sensors",   // Name of the task
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
#if 0
  xTaskCreatePinnedToCore(
    SoundTask,    // Function to implement the task
    "sound",      // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    5,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
#endif

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

void IRAM_ATTR timer_group0_isr(void* para)
{
  static BaseType_t higherPriorityTaskWoken = pdFALSE;

  int timer_idx = (int) para;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  TIMERG0.hw_timer[timer_idx].update = 1;

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0)
  {
    TIMERG0.int_clr_timers.t0 = 1;

    vTaskNotifyGiveFromISR(TaskToNotify, &higherPriorityTaskWoken);

    if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
  else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1)
  {
    TIMERG0.int_clr_timers.t1 = 1;

    //Sensors_RunTask();

#if 0
    xSemaphoreGiveFromISR(Timer125usSemaphore, &higherPriorityTaskWoken);
//#if 0
    if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
//#endif
#endif
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________

static void Timer_Init(int timer_idx, bool auto_reload, double timer_interval_sec)
{
  // Select and initialize basic parameters of the timer
  timer_config_t config;

  config.divider     = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en  = TIMER_PAUSE;
  config.alarm_en    = TIMER_ALARM_EN;
  config.intr_type   = TIMER_INTR_LEVEL;
  config.auto_reload = auto_reload;

  timer_init(TIMER_GROUP_0, timer_idx, &config);

  // Timer's counter will initially start from value below
  // Also, if auto_reload is set, this value will be automatically reload on alarm
  timer_set_counter_value(TIMER_GROUP_0, timer_idx, 0x00000000ULL);

  // Configure the alarm value and the interrupt on alarm
  timer_set_alarm_value(TIMER_GROUP_0, timer_idx, timer_interval_sec * TIMER_SCALE);
  timer_enable_intr(TIMER_GROUP_0, timer_idx);
  timer_isr_register(TIMER_GROUP_0, timer_idx, timer_group0_isr,
                     (void*) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

  //timer_start(TIMER_GROUP_0, timer_idx);

  ESP_LOGI(Tag, "Timer %d is initialized", timer_idx);
}

//_____________________________________________________________________________

static void Settings_Init()
{
  Settings.LeftMotor  = 256;
  Settings.RightMotor = 256;

  OldSettings.LeftMotor  = 256;
  OldSettings.RightMotor = 256;
}

