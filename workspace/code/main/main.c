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
#include "power.h"
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

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

SemaphoreHandle_t Timer125usSemaphore = NULL;
SemaphoreHandle_t Timer10msSemaphore = NULL;
SemaphoreHandle_t Timer200msSemaphore = NULL;

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void FeedWatchdog(void);

static void Timer_Init(int timer_idx, bool auto_reload, double timer_interval_sec);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void SensorTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Leds Task");

  Sensors_Init();

  Leds_Init();

  //leds_SetProxIRBrightness(32, 32, 32, 32, 32, 32, 32, 32);
  //Leds_SetTopBrightness(2, 0, 0);
  //Leds_SetSingleBrightness(E_Led_Battery_1, 2);
  //Leds_SetSingleBrightness(E_Led_Circle_1, 2);
  //Leds_SetCircleBrightness(1, 1, 1, 1, 1, 1, 1, 1);
  //Leds_SetBottomLeftBrightness(0, 32, 0);
  //Leds_SetBottomRightBrightness(0, 32, 5);
  //Power_EnableVA();

  // Attempt to create a semaphore
  Timer125usSemaphore = xSemaphoreCreateBinary();

  timer_start(TIMER_GROUP_0, 1);

  while (1)
  {
    if (Timer125usSemaphore == NULL)
    {
      // There was insufficient FreeRTOS heap available for the semaphore to
      // be created successfully
    }
    else
    {
      /* The semaphore can now be used. Its handle is stored in the
      xSemahore variable. Calling xSemaphoreTake() on the semaphore here
      will fail until the semaphore has first been given. */
      if (xSemaphoreTake(Timer125usSemaphore, 0) == pdTRUE) //portMAX_DELAY );
      {
        Sensors_Task();
        Leds_RunTask();

        xSemaphoreGive(Timer10msSemaphore);
      }
    }

    FeedWatchdog();
  }
  //vTaskDelete(NULL);
  //vTaskDelay(100 / portTICK_PERIOD_MS);
}

//_____________________________________________________________________________

void CommTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Sensors Task");

  Timer200msSemaphore = xSemaphoreCreateBinary();

  //Sound_Init();

  //I2C_Init();

  //STM32_CheckId();
  //ColorSensor_Init();
//#if 0
  //BH1745NUC_Init();
  ColorSensor_Init();
  Accelerometer_Init();
  Compass_Init();
  Gyroscope_Init();
//#endif

  int16_t voltage[2] = {0, 0};
  int16_t current[2] = {0, 0};

  //int16_t frequency = 0;

  while (1)
  {
    if (Timer200msSemaphore == NULL)
    {
      /* There was insufficient FreeRTOS heap available for the semaphore to
      be created successfully. */
    }
    else
    {
      if (xSemaphoreTake(Timer200msSemaphore, 0) == pdTRUE) //portMAX_DELAY );
      {
        //ESP_LOGI(Tag, "Run Sensor Task");
        //frequency++;
        //Sound_Task(frequency);
#if 0
        ColorSensor_GetColor();
        Accelerometer_GetAcceleration();
        Compass_GetMagneticField();
        Gyroscope_GetAngularPosition();
#endif
//#if 0
        //STM32_CheckId();
    	//ColorSensor_GetColor();
        STM32_GetMotorCurrent(current);
        STM32_GetBatteryVoltage(voltage);
        //STM32_UpdateLeftMotorTarget(Target);
        //STM32_UpdateLeftMotorTarget(Target);

        Accelerometer_GetAcceleration();
        //STM32_GetMotorCurrent(current);
        ColorSensor_GetColor();
        //STM32_GetBatteryVoltage(voltage);
        //STM32_UpdateLeftMotorTarget(Target);
        Compass_GetMagneticField();
        //STM32_GetMotorCurrent(current);
        Gyroscope_GetAngularPosition();
        //STM32_GetBatteryVoltage(voltage);
        //ColorSensor_GetColor();

        //BH1745NUC_CheckManufacturerId();

//#endif
      }
    }

    //FeedWatchdog();
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void AsebaTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Aseba Task");

  Timer10msSemaphore = xSemaphoreCreateBinary();

  //uint8_t data[6] = "hello\n";
  //int len = 0;

  UART_Init();
  AsebaESP32_Init();

#if 0
  while (1)
  {
    //len = UART_Read(data);
    //UART_Write(data, len);
    AsebaESP32_Run();
    //UART_Task();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
#endif

//#if 0
  while (1)
  {
    if (Timer10msSemaphore == NULL)
    {
      /* There was insufficient FreeRTOS heap available for the semaphore to
      be created successfully. */
    }
    else
    {
      /* The semaphore can now be used. Its handle is stored in the
      xSemahore variable.  Calling xSemaphoreTake() on the semaphore here
      will fail until the semaphore has first been given. */
      if (xSemaphoreTake(Timer10msSemaphore, 0) == pdTRUE) //portMAX_DELAY );
      {
        //ESP_LOGI(Tag, "Run Aseba Task");
        //len = UART_Read(data);
        //UART_Write(data, len);
        AsebaESP32_Run();
        Behavior_SetIRSensorsLeds();
        //UART_Task();

        xSemaphoreGive(Timer200msSemaphore);
      }
    }

    //FeedWatchdog();

    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
//#endif

#if 0
  while (1)
  {
    if (Timer10msSemaphore == NULL)
    {
      /* There was insufficient FreeRTOS heap available for the semaphore to
      be created successfully. */
    }
    else
    {
      /* The semaphore can now be used. Its handle is stored in the
      xSemahore variable.  Calling xSemaphoreTake() on the semaphore here
      will fail until the semaphore has first been given. */
      if (xSemaphoreTake(Timer10msSemaphore, 0) == pdTRUE) //portMAX_DELAY );
      {
        ESP_LOGI(Tag, "Run Aseba Task");
        //len = UART_Read(data);
        //UART_Write(data, len);
        //AsebaESP32_Run();
        //UART_Task();

        xSemaphoreGive(Timer200msSemaphore);
      }
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
#endif
}

//_____________________________________________________________________________

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

//_____________________________________________________________________________

void WifiTask(void* pvParameter)
{
  //NVS_Init();
  //WifiUpdate_Init();
  //WifiUpdate_Connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

  WIFI_InitNVS();
  WIFI_Init();
#if 0
  while (1)
  {
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
#endif
}

//_____________________________________________________________________________

int app_main(void)
{
  static int appCore1 = 0;
  static int appCore2 = 1;

  //WIFI_InitNVS();
  //WIFI_Init();

  //while (!WIFI_IsConnected())
  {}

//#if 0
  Gpio_Init();
  Power_Init();
  I2C_Init();

  //Timer125usSemaphore = xSemaphoreCreateBinary();

  Timer_Init(1, 1, 0.000125);
  //timer_start(TIMER_GROUP_0, 1);
  //Timer_Init(0, 1, 0.2);

//#endif

  //WIFI_InitNVS();
  //WIFI_Init();

//#if 0
  xTaskCreatePinnedToCore(
    CommTask,  // Function to implement the task
    "sensors",    // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    6,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
//#endif
//#if 0
  xTaskCreatePinnedToCore(
    AsebaTask,    // Function to implement the task
    "aseba",      // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    6,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
//#endif
//#if 0
  xTaskCreatePinnedToCore(
    SensorTask,     // Function to implement the task
    "leds",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    5,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
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
#if 0
  xTaskCreatePinnedToCore(
    WifiTask,     // Function to implement the task
    "wifi",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    4,            // Priority of the task
    NULL,         // Task handle
    appCore2);    // Core where the task should run
#endif

  //Timer_Init(1, 1, 0.000125);
  //Timer_Init(0, 1, 0.2);

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
//#if 0
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
//#endif
}

//_____________________________________________________________________________

void update_aseba_variables_read(void)
{
  // TODO: REMOVE ME (move to behavior ? /!\ behavior == IPL 1 !! race wrt aseba !)
#if 0
  usb_uart_tick();

  motor_get_vind((int*) vmVariables.uind);
#endif
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
  static BaseType_t higherPriorityTaskWoken = false;

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
  }
  else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1)
  {
    TIMERG0.int_clr_timers.t1 = 1;

    xSemaphoreGiveFromISR(Timer125usSemaphore, &higherPriorityTaskWoken);

    if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
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

  config.divider = TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.intr_type = TIMER_INTR_LEVEL;
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

  ESP_LOGI(Tag, "Timer %d is initialized and started", timer_idx);
}
