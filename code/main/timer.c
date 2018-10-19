//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    timer.c
//! \brief   This module provides the useful functions to use the timer
//!
//! \author  Vincent Gonet
//!
//! \version $Id: xxx.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stddef.h>

#include "esp_types.h"
#include "esp_log.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "timer.h"

#include "gpio.h"
#include "board.h"

#include "sensors.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------


#define TIMER_DIVIDER 16 //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define TIMER_INTERVAL0_SEC   (3.4179) // sample test interval for the first timer
#define TIMER_INTERVAL1_SEC (5.78) // sample test interval for the second timer

#define TEST_WITHOUT_RELOAD   0  // testing will be done without auto reload
#define TEST_WITH_RELOAD      1  // testing will be done with auto reload

#define TIMER_EVENT_QUEUE_SIZE 16

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct {
    int type;  // the type of timer's event
    int timer_group;
    int timer_idx;
    uint64_t timer_counter_value;
} timer_event_t;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const T_GpioPinConfig PinConfig = {IR_PULSE_FRONT_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable};

static const T_GpioPinConfig PinConfig[2] =
{
  // PinNumber           Mode               Resistor             Level            Interrupt
  {IR_PULSE_FRONT_PIN,   E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_SENSE_FRONT_1_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable}
};

//xQueueHandle timer_queue;

xSemaphoreHandle TimerSemaphore = NULL;

static const char* Tag = "timer";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void IRAM_ATTR timer_group0_isr(void *para)
{
  int timer_idx = (int) para;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  TIMERG0.hw_timer[timer_idx].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_0)
  {
	Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);
	Leds_Task();
#if 0
	ColorSensor_GetColor();
	Accelerometer_GetAcceleration();
	Compass_GetMagneticField();
	Gyroscope_GetAngularPosition();
#endif
    TIMERG0.int_clr_timers.t0 = 1;
  }
  else if ((intr_status & BIT(timer_idx)) && timer_idx == TIMER_1)
  {
	Gpio_TogglePinLevel(IR_SENSE_FRONT_1_PIN);
    Sensors_Task();
    TIMERG0.int_clr_timers.t1 = 1;
  }
  else
  {

  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG0.hw_timer[timer_idx].config.alarm_en = TIMER_ALARM_EN;
}

//_____________________________________________________________________________

void Timer_Init(int timer_idx, bool auto_reload, double timer_interval_sec)
{
  //Gpio_ConfigurePin(&PinConfig);
  for (uint16_t index = 0u; index < 2; index++)
  {
    Gpio_ConfigurePin(&PinConfig[index]);
  }

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
                     (void *) timer_idx, ESP_INTR_FLAG_IRAM, NULL);

  timer_start(TIMER_GROUP_0, timer_idx);

  ESP_LOGI(Tag, "Timer %d is initialized and started", timer_idx);
}

//_____________________________________________________________________________

void Timer_Task(void)
{
    //Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);
  //timer_event_t evt;
  //xQueueReceive(timer_queue, &evt, 1);
#if 0
  if (evt.type == TEST_WITHOUT_RELOAD)
  {
    //printf("\n    Example timer without reload\n");
  }
  else if (evt.type == TEST_WITH_RELOAD)
  {
	  Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);
      //printf("\n    Example timer with auto reload\n");
  }
  else
  {
      printf("\n    UNKNOWN EVENT TYPE\n");
  }
#endif
//#if 0
  if (TimerSemaphore != NULL)
  {
    if (xSemaphoreTake(TimerSemaphore, portMAX_DELAY) == pdTRUE)
    {
      Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);

      //TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
      //TIMERG0.wdt_feed     = 1u;
      //TIMERG0.wdt_wprotect = 0u;
    }
  }
//#endif
}
