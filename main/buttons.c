//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    buttons.c
//! \brief   This module provides the useful functions to use the capacitive buttons
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
//#include "soc/sens_periph.h"

#include "esp_log.h"

#include "buttons.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define BUTTONS_NUM            6u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "buttons";

static touch_pad_t Buttons_Table[BUTTONS_NUM];

static bool s_pad_activated[BUTTONS_NUM];
static uint32_t s_pad_init_val[BUTTONS_NUM];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunButtonsTask(void* arg);

static void tp_example_touch_pad_init();

static void tp_example_rtc_intr(void * arg);

static void tp_example_set_thresholds(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Buttons_Init(void)
{
  // Initialize touch pad peripheral, it will start a timer to run a filter
  touch_pad_init();

  // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);

  // Set reference voltage for charging/discharging
  // For most usage scenarios, we recommend using the following combination:
  // the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
  touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);

  // Init touch pad IO
  tp_example_touch_pad_init();

  // Initialize and start a software filter to detect slight change of capacitance.
  touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);

  // Set thresh hold
  tp_example_set_thresholds();

  // Register touch interrupt ISR
  touch_pad_isr_register(tp_example_rtc_intr, NULL);

  ESP_LOGI(Tag, "Buttons are initialized");
}

//_____________________________________________________________________________

void Buttons_Start(void)
{
  xTaskCreatePinnedToCore(
    RunButtonsTask,  // Function to implement the task
    "buttons",       // Name of the task
    2048,         // Stack size in words
    NULL,         // Task input parameter
    3,            // Priority of the task
    NULL,         // Task handle
    0);           // Core where the task should run
}

//_____________________________________________________________________________

static void RunButtonsTask(void* arg)
{
  static int show_message;
  int change_mode = 0;
  int filter_mode = 0;

  while (1)
  {
    if (filter_mode == 0)
    {
      //interrupt mode, enable touch interrupt
      touch_pad_intr_enable();

      for (uint8_t index = 0u; index < BUTTONS_NUM; index++)
      {
        if (s_pad_activated[index] == true)
        {
          ESP_LOGI(Tag, "T%d activated!", Buttons_Table[index]);

          // Wait a while for the pad being released
          vTaskDelay(200 / portTICK_PERIOD_MS);

          // Clear information on pad activation
          s_pad_activated[index] = false;

          // Reset the counter triggering a message
          // that application is running
          show_message = 1;
        }
      }

#if 0
      for (int i = 0; i < TOUCH_PAD_MAX; i++)
      {
        if (s_pad_activated[i] == true)
        {
          ESP_LOGI(Tag, "T%d activated!", i);

          // Wait a while for the pad being released
          vTaskDelay(200 / portTICK_PERIOD_MS);

          // Clear information on pad activation
          s_pad_activated[i] = false;

          // Reset the counter triggering a message
          // that application is running
          show_message = 1;
        }
      }
#endif
    }
#if 0
    else
    {
      //filter mode, disable touch interrupt
      touch_pad_intr_disable();
      touch_pad_clear_status();

      for (uint8_t index = 0u; index < BUTTONS_NUM; index++)
      {
        uint16_t value = 0;

        touch_pad_read_filtered(Buttons_Table[index], &value);

        if (value < s_pad_init_val[index] * TOUCH_THRESH_PERCENT / 100)
        {
          ESP_LOGI(Tag, "T%d activated!", Buttons_Table[index]);
          ESP_LOGI(Tag, "value: %d; init val: %d", value, s_pad_init_val[index]);

          vTaskDelay(200 / portTICK_PERIOD_MS);

          // Reset the counter to stop changing mode.
          change_mode = 1;
          show_message = 1;
        }
      }
#endif
#if 0
      for (int i = 0; i < TOUCH_PAD_MAX; i++)
      {
        uint16_t value = 0;

        touch_pad_read_filtered(i, &value);

        if (value < s_pad_init_val[i] * TOUCH_THRESH_PERCENT / 100)
        {
          ESP_LOGI(Tag, "T%d activated!", i);
          ESP_LOGI(Tag, "value: %d; init val: %d", value, s_pad_init_val[i]);

          vTaskDelay(200 / portTICK_PERIOD_MS);

          // Reset the counter to stop changing mode.
          change_mode = 1;
          show_message = 1;
        }
      }
    }
#endif
//#if 0
    vTaskDelay(10 / portTICK_PERIOD_MS);

    // If no pad is touched, every couple of seconds, show a message
    // that application is running
    if (show_message++ % 500 == 0)
    {
      ESP_LOGI(Tag, "Waiting for any pad being touched...");
    }
#if 0
    // Change mode if no pad is touched for a long time.
    // We can compare the two different mode.
    if (change_mode++ % 2000 == 0)
    {
      filter_mode = !filter_mode;
      ESP_LOGW(Tag, "Change mode...%s", filter_mode == 0? "interrupt mode": "filter mode");
    }
#endif
  }
}

//_____________________________________________________________________________

static void tp_example_touch_pad_init()
{
  Buttons_Table[0] = TOUCH_PAD_NUM0;
  Buttons_Table[1] = TOUCH_PAD_NUM2;
  Buttons_Table[2] = TOUCH_PAD_NUM5;
  Buttons_Table[3] = TOUCH_PAD_NUM7;
  Buttons_Table[4] = TOUCH_PAD_NUM8;
  Buttons_Table[5] = TOUCH_PAD_NUM9;

  for (uint8_t index = 0u; index < BUTTONS_NUM; index++)
  {
    // Initialize RTC IO and mode for touch pad
	touch_pad_config(Buttons_Table[index], TOUCH_THRESH_NO_USE);
  }

#if 0
  for (int i = 0; i < TOUCH_PAD_MAX;i++)
  {
    //init RTC IO and mode for touch pad.
    touch_pad_config(i, TOUCH_THRESH_NO_USE);
  }
#endif
}

//_____________________________________________________________________________

static void tp_example_rtc_intr(void * arg)
{
  uint32_t pad_intr = touch_pad_get_status();

  //clear interrupt
  touch_pad_clear_status();

  for (uint8_t index = 0u; index < BUTTONS_NUM; index++)
  {
	if ((pad_intr >> Buttons_Table[index]) & 0x01)
	{
	  s_pad_activated[index] = true;
	}
  }

#if 0
  for (int i = 0; i < TOUCH_PAD_MAX; i++)
  {
    if ((pad_intr >> i) & 0x01)
    {
      s_pad_activated[i] = true;
    }
  }
#endif
}

//_____________________________________________________________________________

static void tp_example_set_thresholds(void)
{
  uint16_t touch_value;

  for (uint8_t index = 0u; index < BUTTONS_NUM; index++)
  {
    //read filtered value
    touch_pad_read_filtered(Buttons_Table[index], &touch_value);
	s_pad_init_val[index] = touch_value;
	ESP_LOGI(Tag, "test init: touch pad [%d] val is %d", index, touch_value);
	//set interrupt threshold.
	ESP_ERROR_CHECK(touch_pad_set_thresh(Buttons_Table[index], touch_value * 2 / 3));
  }

#if 0
  for (int i = 0; i<TOUCH_PAD_MAX; i++)
  {
    //read filtered value
    touch_pad_read_filtered(i, &touch_value);
    s_pad_init_val[i] = touch_value;
    ESP_LOGI(Tag, "test init: touch pad [%d] val is %d", i, touch_value);
    //set interrupt threshold.
    ESP_ERROR_CHECK(touch_pad_set_thresh(i, touch_value * 2 / 3));
  }
#endif
}

