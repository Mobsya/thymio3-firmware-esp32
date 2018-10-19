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

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "sdkconfig.h"

#include "esp_log.h"

#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "power.h"
#include "sound.h"
#include "uart.h"
#include "wifi_update.h"

// I2C modules
#include "accelerometer.h"
#include "color_sensor.h"
#include "compass.h"
#include "gyroscope.h"
#include "stm32.h"

#include "aseba_esp32.h"

#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

// Set the SSID and Password via "make menuconfig"
#define DEFAULT_WIFI_SSID           "mobsya"
#define DEFAULT_WIFI_PASSWORD       "Thymio2LeRobot"

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void FeedWatchdog(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void LedsTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Leds Task");

  Leds_Init();

  //leds_SetProxIRBrightness(32, 32, 32, 32, 32, 32, 32, 32);
  //Leds_SetTopBrightness(2, 0, 0);
  //Leds_SetSingleBrightness(E_Led_Battery_1, 2);
  //Leds_SetSingleBrightness(E_Led_Circle_1, 2);
  //Leds_SetCircleBrightness(1, 1, 1, 1, 1, 1, 1, 1);
  //Leds_SetBottomLeftBrightness(0, 32, 0);
  //Leds_SetBottomRightBrightness(0, 32, 5);
  //Power_EnableVA();

  while (1)
  {
    Leds_Task();

    FeedWatchdog();
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void SensorsTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Sensors Task");

  //I2C_Init();

  //STM32_CheckId();

  //BH1745NUC_Init();
  ColorSensor_Init();
  Accelerometer_Init();
  Compass_Init();
  Gyroscope_Init();

  //LSM303C_InitAccelerometer();
  //LSM303C_InitMagnetometer();

  //LSM6DS3US_InitAccelerometer();
  //LSM6DS3US_InitGyroscope();

  //int16_t voltage[2] = {0,0};

  while (1)
  {
    //STM32_CheckId();
    //STM32_UpdateMotorLeftTarget(Target);
    //STM32_GetBatteryVoltage(voltage);

    //BH1745NUC_CheckManufacturerId();
    //(void)BH1745NUC_GetIlluminance_lux();
    ColorSensor_GetColor();
    Accelerometer_GetAcceleration();
    Compass_GetMagneticField();
    Gyroscope_GetAngularPosition();

    //LSM303C_InitMagnetometer();
    //LSM303C_CheckAccManufacturerId();
    //LSM303C_CheckMagManufacturerId();
    //LSM303C_GetAcceleration();
    //LSM303C_GetMagneticField();

    //LSM6DS3US_CheckManufacturerId();
    //LSM6DS3US_GetAcceleration();
    //LSM6DS3US_GetAngularPosition();

    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void AsebaTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Aseba Task");

  //uint8_t data[6] = "hello\n";
  //int len = 0;

  UART_Init();
  AsebaESP32_Init();

  while (1)
  {
    //len = UART_Read(data);
    //UART_Write(data, len);
    AsebaESP32_Run();
    //UART_Task();
    //vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void SoundTask(void* pvParameter)
{
  ESP_LOGI(Tag, "Start Sound Task");

  Sound_Init();

  while (1)
  {
    Sound_Task();

    FeedWatchdog();
    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void WifiTask(void* pvParameter)
{
  NVS_Init();
  WifiUpdate_Init();
  WifiUpdate_Connect(DEFAULT_WIFI_SSID, DEFAULT_WIFI_PASSWORD);

  while (1)
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

int app_main(void)
{
  static int appCore1 = 0;
  static int appCore2 = 1;

  Gpio_Init();
  Power_Init();
  I2C_Init();

  xTaskCreatePinnedToCore(
    LedsTask,     // Function to implement the task
    "leds",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    1,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run

  xTaskCreatePinnedToCore(
    SensorsTask,  // Function to implement the task
    "sensors",    // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    5,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run

  xTaskCreatePinnedToCore(
    AsebaTask,    // Function to implement the task
    "aseba",      // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    5,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run
#if 0
  xTaskCreatePinnedToCore(
    SoundTask,    // Function to implement the task
    "sound",      // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    5,            // Priority of the task
    NULL,         // Task handle
    appCore1);    // Core where the task should run

  xTaskCreatePinnedToCore(
    WifiTask,     // Function to implement the task
    "wifi",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    4,            // Priority of the task
    NULL,         // Task handle
    appCore2);    // Core where the task should run
#endif
  return 0;
}

//_____________________________________________________________________________

AsebaNativeFunctionDescription AsebaNativeDescription_poweroff = {
    "_poweroff",
    "Poweroff",
    {
        {0,0}
    }
};

//_____________________________________________________________________________

void power_off(AsebaVMState *vm) {
        unsigned int flags;

    // Protect against two racing poweroff:
    //  One from the softirq (button)
    //  One from the VM
#if 0
    RAISE_IPL(flags,1);

    behavior_stop(B_ALL);

    play_sound_block(SOUND_POWEROFF);

    // Shutdown all peripherals ...
    switch_off();

    // Switch off USB
    // If we are connected to a PC, disconnect.
    // If we are NOT connected to a PC but 5V is present
    // ( == charger ) we need to keep the transciever on
    if(usb_uart_configured())
        USBDeviceDetach();

    // In any case, disable the usb interrupt. It's safer
    _USB1IE = 0;


    CHARGE_ENABLE_DIR = 1;

    analog_enter_poweroff_mode();
#endif
}

//_____________________________________________________________________________

void update_aseba_variables_write(void) {
#if 0
    static unsigned int old_timer[2];
    int i;

    for(i = 0; i < 2; i++) {
        if(vmVariables.timers[i] != old_timer[i]) {
            old_timer[i] = vmVariables.timers[i];
            timer[i] = 0;
        }
    }

    pid_motor_set_target((int *) vmVariables.target);
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

void update_aseba_variables_read(void) {
    // TODO: REMOVE ME (move to behavior ? /!\ behavior == IPL 1 !! race wrt aseba !)
#if 0
    usb_uart_tick();

    motor_get_vind((int *) vmVariables.uind);
#endif
}

//_____________________________________________________________________________

static void FeedWatchdog(void)
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed     = 1u;
  TIMERG0.wdt_wprotect = 0u;
}
