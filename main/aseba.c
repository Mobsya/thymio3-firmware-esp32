//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    aseba.c
//! \brief   This module provides the useful functions to use Aseba Studio
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "aseba.h"

#include "behavior.h"
#include "comm.h"
#include "leds.h"
#include "sensors.h"
#include "timer_sw.h"

#include "aseba_esp32.h"
#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//#define FW_VERSION 13

/* Firmware variant. Each variant of the firmware has it own number */

/* Variant list:
0: Standard one
1: Development one

*/
//#define FW_VARIANT 1

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

xSemaphoreHandle I2CMutex;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "aseba";

static T_TimerSw* AsebaTimer0 = NULL;  //!< Used to schedule the Aseba timer 0
static T_TimerSw* AsebaTimer1 = NULL;  //!< Used to schedule the Aseba timer 0

static int16_t TimerDuration[2] = {0, 0};
static int16_t OldTimerDuration[2] = {0, 0};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void UpdateTimers(void);

static void UpdateLedsCircle(void);

static void UpdateLedsMatrixFront(void);

static void UpdateLedsMatrixBack(void);

static void UpdateLedFrontLeft(void);

static void UpdateLedFrontRight(void);

static void UpdateLedBackLeft(void);

static void UpdateLedBackRight(void);

static void UpdateLedColorSensor(void);

static void Callback_AsebaTimer0(void* arg);

static void Callback_AsebaTimer1(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Aseba_Init(void)
{
  AsebaTimer0 = TimerSw_Create(0, Callback_AsebaTimer0);
  AsebaTimer1 = TimerSw_Create(0, Callback_AsebaTimer1);

  ESP_LOGI(Tag, "Aseba is initialized");
}

//_____________________________________________________________________________

void Aseba_UpdateGroundIRLedsBrightness(uint16_t l0, uint16_t l1)
{
  //GroundIRLed[0] = l0;
  //GroundIRLed[1] = l1;
}

//_____________________________________________________________________________

void update_aseba_variables_write(void)
{
  UpdateTimers();
  UpdateLedsCircle();
  UpdateLedsMatrixFront();
  UpdateLedsMatrixBack();
  UpdateLedFrontLeft();
  UpdateLedFrontRight();
  UpdateLedBackLeft();
  UpdateLedBackRight();
  UpdateLedColorSensor();
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
  //WIFIUpdate_GetIPAddress();
}

//_____________________________________________________________________________

void AsebaVMResetCB(AsebaVMState* vm)
{
  Leds_SetBodyBrightness(0u, 0u, 0u);
  Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetMatrixFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetMatrixBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  Leds_SetButtonsBrightness(0u, 0u, 0u, 0u);

  //Leds_SetSingleBrightness(E_Led_Battery_1, MAX_BRIGHTNESS);

#if 0 // FIXME
  leds_set(LED_SOUND, 0);
  leds_set(LED_RC, 0);
#endif
  Behavior_Enable(B_LEDS_ACC);
  Behavior_Enable(B_LEDS_MATRIX);
  Behavior_Enable(B_LED_MIC);
  Behavior_Enable(B_LEDS_PROX);
  Behavior_Enable(B_SOUND_BUTTON);
  Behavior_Enable(B_LED_RC5);
#if 0 // FIXME
  Behavior_Enable(B_LED_MIC);
  Behavior_Enable(B_LED_RC5);
  prox_disable_network();
  events_flags[0] = 0;
  events_flags[1] = 0;
  memset(vm->variables, 0, vm->variablesSize * sizeof(int16_t));
  vmVariables.id = vmState.nodeId;
  vmVariables.productid = PRODUCT_ID;
  vmVariables.fwversion[0] = FW_VERSION;
  vmVariables.fwversion[1] = FW_VARIANT;
#endif

//#if 0
  //events_flags[0] = 0;
  //events_flags[1] = 0;
  memset(vm->variables, 0, vm->variablesSize * sizeof(int16_t));
  vmVariables.id = vmState.nodeId;
  vmVariables.productid = PRODUCT_ID;
  //vmVariables.fwversion[0] = FW_VERSION;
  //vmVariables.fwversion[1] = FW_VARIANT;
//#endif
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

static void UpdateLedsCircle(void)
{
  // brightness[0] is assigned to the LED D27
  // brightness[1] is assigned to the LED D30
  // brightness[2] is assigned to the LED D33
  // brightness[3] is assigned to the LED D36
  // brightness[4] is assigned to the LED D39
  // brightness[5] is assigned to the LED D42
  // brightness[6] is assigned to the LED D45
  // brightness[7] is assigned to the LED D48
  static int16_t brightness[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t index = 0u; index < 8u; index++)
  {
    if (brightness[index] != vmVariables.leds_circle[index])
    {
      brightness[index] = vmVariables.leds_circle[index];

      Behavior_Disable(B_LEDS_CIRCLE);
      Leds_SetSingleBrightness((E_Led_Circle_N + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedsMatrixFront(void)
{
  // brightness[0] is assigned to the LED D28
  // brightness[1] is assigned to the LED D31
  // brightness[2] is assigned to the LED D34
  // brightness[3] is assigned to the LED D37
  // brightness[4] is assigned to the LED D40
  // brightness[5] is assigned to the LED D43
  // brightness[6] is assigned to the LED D46
  // brightness[7] is assigned to the LED D49
  static int16_t brightness[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t index = 0u; index < 8u; index++)
  {
    if (brightness[index] != vmVariables.leds_matrix_front[index])
    {
      brightness[index] = vmVariables.leds_matrix_front[index];

      Behavior_Disable(B_LEDS_MATRIX);
      Leds_SetSingleBrightness((E_Led_Matrix_Front_0 + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedsMatrixBack(void)
{
  // brightness[0] is assigned to the LED D29
  // brightness[1] is assigned to the LED D32
  // brightness[2] is assigned to the LED D35
  // brightness[3] is assigned to the LED D38
  // brightness[4] is assigned to the LED D41
  // brightness[5] is assigned to the LED D44
  // brightness[6] is assigned to the LED D47
  // brightness[7] is assigned to the LED D50
  static int16_t brightness[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (uint8_t index = 0u; index < 8u; index++)
  {
    if (brightness[index] != vmVariables.leds_matrix_back[index])
    {
      brightness[index] = vmVariables.leds_matrix_back[index];

      Behavior_Disable(B_LEDS_MATRIX);
      Leds_SetSingleBrightness((E_Led_Matrix_Back_0 + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedFrontLeft(void)
{
  // brightness[0] is assigned to the LED D19 (red)
  // brightness[1] is assigned to the LED D19 (green)
  // brightness[2] is assigned to the LED D19 (blue)
  static int16_t brightness[3] = {0, 0, 0};

  for (uint8_t index = 0u; index < 3u; index++)
  {
    if (brightness[index] != vmVariables.led_front_left[index])
    {
      brightness[index] = vmVariables.led_front_left[index];

      Behavior_Disable(B_LEDS_RGB);
      Leds_SetSingleBrightness((E_Led_R_Front_Left + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedFrontRight(void)
{
  // brightness[0] is assigned to the LED D26 (red)
  // brightness[1] is assigned to the LED D26 (green)
  // brightness[2] is assigned to the LED D26 (blue)
  static int16_t brightness[3] = {0, 0, 0};

  for (uint8_t index = 0u; index < 3u; index++)
  {
    if (brightness[index] != vmVariables.led_front_right[index])
    {
      brightness[index] = vmVariables.led_front_right[index];

      Behavior_Disable(B_LEDS_RGB);
      Leds_SetSingleBrightness((E_Led_R_Front_Right + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedBackLeft(void)
{
  // brightness[0] is assigned to the LED D52 (red)
  // brightness[1] is assigned to the LED D52 (green)
  // brightness[2] is assigned to the LED D52 (blue)
  static int16_t brightness[3] = {0, 0, 0};

  for (uint8_t index = 0u; index < 3u; index++)
  {
    if (brightness[index] != vmVariables.led_back_left[index])
    {
      brightness[index] = vmVariables.led_back_left[index];

      Behavior_Disable(B_LEDS_RGB);
      Leds_SetSingleBrightness((E_Led_R_Back_Left + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedBackRight(void)
{
  // brightness[0] is assigned to the LED D51 (red)
  // brightness[1] is assigned to the LED D51 (green)
  // brightness[2] is assigned to the LED D51 (blue)
  static int16_t brightness[3] = {0, 0, 0};

  for (uint8_t index = 0u; index < 3u; index++)
  {
    if (brightness[index] != vmVariables.led_back_right[index])
    {
      brightness[index] = vmVariables.led_back_right[index];

      Behavior_Disable(B_LEDS_RGB);
      Leds_SetSingleBrightness((E_Led_R_Back_Right + index), brightness[index]);
    }
  }
}

//_____________________________________________________________________________

static void UpdateLedColorSensor(void)
{
  // brightness[0] is assigned to the LED D25 (red)
  // brightness[1] is assigned to the LED D25 (green)
  // brightness[2] is assigned to the LED D25 (blue)
  static int16_t brightness[3] = {0, 0, 0};

  for (uint8_t index = 0u; index < 3u; index++)
  {
    if (brightness[index] != vmVariables.led_color_sensor[index])
    {
      brightness[index] = vmVariables.led_color_sensor[index];

      Behavior_Disable(B_LEDS_RGB);
      Leds_SetSingleBrightness((E_Led_R_Color_Sensor + index), brightness[index]);
    }
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
