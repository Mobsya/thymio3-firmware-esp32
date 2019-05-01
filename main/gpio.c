//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    gpio.c
//! \brief   This module provides the useful functions to use the GPIO
//!
//! \author  Vincent Gonet
//!
//! \version $Id: gpio.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <driver/gpio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "gpio.h"
#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ESP_INTR_FLAG_DEFAULT 0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  gpio_pullup_t   up;
  gpio_pulldown_t down;
} T_PullUpDown;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "gpio";

static xQueueHandle gpio_evt_queue = NULL;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static gpio_mode_t SetMode(T_GpioMode mode);

static T_PullUpDown SetResistor(T_GpioResistor resistor);

static gpio_int_type_t SetInterrupt(T_GpioInterrupt interrupt);

static void IRAM_ATTR gpio_isr_handler(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Gpio_Init(void)
{
  //gpio_config_t io_conf;

#if 0
  //disable interrupt
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = MOTOR_PIN_SEL;
  //disable pull-down mode
  io_conf.pull_down_en = 0;
  //disable pull-up mode
  io_conf.pull_up_en = 0;
#endif

  //configure GPIO with the given settings
  //gpio_config(&io_conf);

  //interrupt of rising edge
  //io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;
  //bit mask of the button pins
  //io_conf.pin_bit_mask = BUTTON_PIN_SEL;
  //set as input mode
  //io_conf.mode = GPIO_MODE_INPUT;
  //enable pull-up mode
  //io_conf.pull_up_en = 1;

  //gpio_config(&io_conf);

  //create a queue to handle gpio event from isr
  gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

  //install gpio isr service
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
#if 0
  //hook isr handler for specific gpio pins
  gpio_isr_handler_add(BUTTON_FORWARD_PIN, gpio_isr_handler, (void*) BUTTON_FORWARD_PIN);
  gpio_isr_handler_add(BUTTON_BACKWARD_PIN, gpio_isr_handler, (void*) BUTTON_BACKWARD_PIN);
  gpio_isr_handler_add(BUTTON_LEFT_PIN, gpio_isr_handler, (void*) BUTTON_LEFT_PIN);
  gpio_isr_handler_add(BUTTON_RIGHT_PIN, gpio_isr_handler, (void*) BUTTON_RIGHT_PIN);
  gpio_isr_handler_add(BUTTON_CENTER_PIN, gpio_isr_handler, (void*) BUTTON_CENTER_PIN);

  //gpio_set_direction(M1_PWM_1A_PIN, GPIO_MODE_OUTPUT);

  //gpio_set_direction(IR_PULSE_GROUND_LEFT, GPIO_MODE_OUTPUT);
  //gpio_set_direction(IR_PULSE_GROUND_RIGHT, GPIO_MODE_OUTPUT);
#endif
}

//_____________________________________________________________________________

#if 0
void Gpio_Task(void* arg)
{
  uint32_t io_num;

  for (;;)
  {
    if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
    {
      //printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));

      if (gpio_get_level(io_num) == 1)
      {
        printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
      }
    }

    vTaskDelay(100 / portTICK_RATE_MS);
  }
}
#endif

void Gpio_Task(void)
{
  uint32_t io_num;

  if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
  {
    printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
  }
}

//_____________________________________________________________________________

void Gpio_ConfigurePin(const T_GpioPinConfig* config)
{
  T_PullUpDown pullUpDown;
  gpio_config_t pinConfig;

  if (config->pinNumber < MAX_NUMBER_PIN)
  {
    pullUpDown = SetResistor(config->resistor);

    pinConfig.intr_type    = SetInterrupt(config->interrupt);
    pinConfig.mode         = SetMode(config->mode);
    pinConfig.pin_bit_mask = (((uint64_t) 1) << config->pinNumber);
    pinConfig.pull_down_en = pullUpDown.down;
    pinConfig.pull_up_en   = pullUpDown.up;

    if (gpio_config(&pinConfig) == ESP_OK)
    {
      if (config->interrupt > E_GpioInterrupt_Disable)
      {
        //ESP_ERROR_CHECK(gpio_isr_handler_add(config->pinNumber, gpio_isr_handler, (void*) config->pinNumber));
        gpio_isr_handler_add(config->pinNumber, gpio_isr_handler, (void*) config->pinNumber);
      }

      //ESP_LOGI(Tag, "Pin %d is configured", config->pinNumber);
      ESP_ERROR_CHECK(gpio_set_level(config->pinNumber, config->level));
    }
    else
    {
      ESP_LOGE(Tag, "Pin %d is not configured", config->pinNumber);
    }
  }
  else
  {
    ESP_LOGE(Tag, "Invalid pin %d", config->pinNumber);
  }
}

//_____________________________________________________________________________

void Gpio_SetPinLevel(uint16_t pinNumber, T_GpioLevel level)
{
  ESP_ERROR_CHECK(gpio_set_level(pinNumber, level));
}

//_____________________________________________________________________________

void Gpio_TogglePinLevel(uint16_t pinNumber)
{
  static uint8_t counter = 0;

  ESP_ERROR_CHECK(gpio_set_level(pinNumber, counter % 2));
  counter++;
}

//_____________________________________________________________________________

int Gpio_GetPinLevel(uint16_t pinNumber)
{
  return gpio_get_level(pinNumber);
}

//_____________________________________________________________________________

static gpio_mode_t SetMode(T_GpioMode mode)
{
  gpio_mode_t pinMode = GPIO_MODE_INPUT;

  switch (mode)
  {
    case E_GpioMode_Input:
      pinMode = GPIO_MODE_INPUT;
      break;

    case E_GpioMode_Output:
      pinMode = GPIO_MODE_OUTPUT;
      break;

    case E_GpioMode_OutputOpenDrain:
      pinMode = GPIO_MODE_OUTPUT_OD;
      break;

    case E_GpioMode_InputOutput:
      pinMode = GPIO_MODE_INPUT_OUTPUT;
      break;

    case E_GpioMode_InputOutputOpenDrain:
      pinMode = GPIO_MODE_INPUT_OUTPUT_OD;
      break;

    default:
      ESP_LOGE(Tag, "Invalid mode %d", mode);
      break;
  }

  return pinMode;
}

//_____________________________________________________________________________

static T_PullUpDown SetResistor(T_GpioResistor resistor)
{
  T_PullUpDown pullUpDown;

  switch (resistor)
  {
    case E_GpioResistor_None:
      pullUpDown.up   = GPIO_PULLUP_DISABLE;
      pullUpDown.down = GPIO_PULLDOWN_DISABLE;
      break;

    case E_GpioResistor_PullUp:
      pullUpDown.up   = GPIO_PULLUP_ENABLE;
      pullUpDown.down = GPIO_PULLDOWN_DISABLE;
      break;

    case E_GpioResistor_PullDown:
      pullUpDown.up   = GPIO_PULLUP_DISABLE;
      pullUpDown.down = GPIO_PULLDOWN_ENABLE;
      break;

    default:
      ESP_LOGE(Tag, "Invalid resistor %d", resistor);
      break;
  }

  return pullUpDown;
}

//_____________________________________________________________________________

static gpio_int_type_t SetInterrupt(T_GpioInterrupt interrupt)
{
  gpio_int_type_t pinInterrupt = GPIO_INTR_DISABLE;

  switch (interrupt)
  {
    case E_GpioInterrupt_Disable:
      pinInterrupt = GPIO_INTR_DISABLE;
      break;

    case E_GpioInterrupt_RisingEdge:
      pinInterrupt = GPIO_INTR_POSEDGE;
      break;

    case E_GpioInterrupt_FallingEdge:
      pinInterrupt = GPIO_INTR_NEGEDGE;
      break;

    case E_GpioInterrupt_AnyEdge:
      pinInterrupt = GPIO_INTR_ANYEDGE;
      break;

    case E_GpioInterrupt_LowLevel:
      pinInterrupt = GPIO_INTR_LOW_LEVEL;
      break;

    case E_GpioInterrupt_HighLevel:
      pinInterrupt = GPIO_INTR_HIGH_LEVEL;
      break;

    default:
      ESP_LOGE(Tag, "Invalid interrupt %d", interrupt);
      break;
  }

  return pinInterrupt;
}

//_____________________________________________________________________________

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
  uint32_t gpio_num = (uint32_t) arg;
  xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}
