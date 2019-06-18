//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    prox_ir.c
//! \brief   This module provides the useful functions to use the proximity IR sensors
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/mcpwm.h"
#include "soc/rtc.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

#include "prox_ir.h"

#include "aseba_esp32.h"
#include "board.h"
#include "gpio.h"
#include "timer_hw.h"
#include "timer_sw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PROX_IR_PIN_NUM                   3u

#define CALIB_HYSTERESIS                  20

#define DEFAULT_CALIB                 0x7FFF

#define CAP0_INT_EN                  BIT(27)  // Capture 0 interrupt bit
#define CAP1_INT_EN                  BIT(28)  // Capture 1 interrupt bit
#define CAP2_INT_EN                  BIT(29)  // Capture 2 interrupt bit

#define CAP_SIG_NUM                       3u  // Three capture signals

#define MAX_PULSE_DURATION             4600u

#define PULSE_DURATION_FACTOR             4u  // Factor to adapt the pulse duration

// Duration of the pulse = 60 [us] -> TIMER_SCALE * 60 [us] = 300 (timer_group)
#define TX_PULSE_DURATION               300u

#define READ_RX_PULSE_us                750u
// Duration of the pulse = 750 [us] -> TIMER_SCALE * 750 [us] = 3750 (timer_group)
#define RX_PULSE_READ_DURATION         3750u

//#define READ_UNIT1_INTERVAL_us          875u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_Sensor_Right,
  E_Sensor_Left
} T_Sensor;

typedef enum
{
  E_NetworkStatus_Disabled,
  E_NetworkStatus_WillBeEnabled,
  E_NetworkStatus_Enabled
} T_NetworkStatus;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "prox_ir";

#if 0
static const T_GpioPinConfig PinConfig[PROX_IR_PIN_NUM] =
{
  // PinNumber              Mode               Resistor             Level            Interrupt
  {IR_PULSE_BACK_PIN,       E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_PULSE_FRONT_PIN,      E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_SENSE_BACK_RIGHT_PIN, E_GpioMode_Input,  E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_AnyEdge}
};
// Other pins are configured by the input capture module
#endif

static T_NetworkStatus NetworkStatus = E_NetworkStatus_Disabled;

static bool FirstTxPulsesAreInProgress = false;
static bool SecondTxPulsesAreInProgress = false;

static volatile uint32_t PulseCounter[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};
static uint32_t OldPulseCounter[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};

static volatile uint32_t RisingEdgeTime[7]  = {0u, 0u, 0u, 0u, 0u, 0u, 0u};
static volatile uint32_t FallingEdgeTime[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};

static volatile uint32_t RisingEdgeCounter[7]  = {0u, 0u, 0u, 0u, 0u, 0u, 0u};  // For debug only
static volatile uint32_t FallingEdgeCounter[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};  // For debug only

static uint32_t PulseDuration[7];

//static T_TimerSw* TxPulsesDurationTimer = NULL;  //!< Timer used to determine the duration of the TX pulses
static T_TimerSw* ReadRxPulseTimer = NULL;
//static T_TimerSw* ReadUnit1Timer        = NULL;
//static T_TimerSw* TxPulsesGapTimer = NULL;       //!< Timer used to determine the gap between two TX pulses

static T_TimerSw* BackRightPulseTimer = NULL;

static uint8_t SlotNumber = 0xFFu;

TaskHandle_t TxProxIRTask;
TaskHandle_t RxProxIRTask;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunTxProxIRTask(void* arg);

static void RunRxProxIRTask(void* arg);

static void ConfigureInputCapture(void);

static void CalculateRxPulseDuration(void);

static void EnableInputCapture(void);

//static void DisableInputCapture(void);

static mcpwm_dev_t* MCPWM[2] = {&MCPWM0, &MCPWM1};

static void Callback_TimerReadRxPulse(void* arg);
//static void Callback_TimerTxPulsesGap(void* arg);

static void IRAM_ATTR ISR_InputCaptureUnit0(void* arg);
static void IRAM_ATTR ISR_InputCaptureUnit1(void* arg);

static void IRAM_ATTR ISR_EndOfTxPulse(void* para);

static void IRAM_ATTR ISR_ReadRxPulse(void* para);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void ProxIR_Init(void)
{
  for (uint16_t index = 0u; index < PROX_IR_PIN_NUM; index++)
  {
    //Gpio_ConfigurePin(&PinConfig[index]);
  }

  // brand-new robots will have a settings to 0
  // Thus put the maximum minimum value in order to start the calibration
  // from a valid point.
  for (uint8_t sensor = 0; sensor < PROX_IR_SENSORS_NUM; sensor++)
  {
    if (settings.prox_min[sensor] == 0)
    {
      settings.prox_min[sensor] = DEFAULT_CALIB;
    }
  }

  ConfigureInputCapture();

  // Timer used to generate the TX pulse
  TimerHw_Init(1, 0, true, TX_PULSE_DURATION, ISR_EndOfTxPulse);

  // When this timer expires, the duration of the RX pulse is calculated
  //ReadRxPulseTimer = TimerSw_Create(READ_RX_PULSE_us, Callback_TimerReadRxPulse);
  TimerHw_Init(1, 1, true, RX_PULSE_READ_DURATION, ISR_ReadRxPulse);

  //TxPulsesGapTimer      = TimerSw_Create(200, Callback_TimerTxPulsesGap);

  ESP_LOGI(Tag, "Proximity IR sensors are initialized");
}

//_____________________________________________________________________________

void ProxIR_Start(void)
{
  xTaskCreatePinnedToCore(
    RunTxProxIRTask,  // Function to implement the task
    "tx",             // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    5,                // Priority of the task
    &TxProxIRTask,    // Task handle
    0);               // Core where the task should run

  xTaskCreatePinnedToCore(
    RunRxProxIRTask,  // Function to implement the task
    "rx",             // Name of the task
    4096,             // Stack size in words
    NULL,             // Task input parameter
    5,                // Priority of the task
    &RxProxIRTask,    // Task handle
    0);               // Core where the task should run
}

//_____________________________________________________________________________

static void RunTxProxIRTask(void* arg)
{
  ESP_LOGI(Tag, "Start TX Prox IR Task");

  while (1)
  {
    //Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_High);
    //Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);

    TimerHw_Start(1, 0);  // Timer used to generate the TX pulse
    TimerHw_Start(1, 1);  // When this timer expires, the duration of the RX pulse is calculated

    FirstTxPulsesAreInProgress = true;

    // When this timer expires, the duration of the RX pulse is calculated
    //TimerSw_StartTimerOnce(ReadRxPulseTimer, READ_RX_PULSE_us);

    //ESP_LOGE(Tag, "old = %d, new = %d", OldPulseCounter[0], PulseCounter[0]);

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

static void RunRxProxIRTask(void* arg)
{
  ESP_LOGI(Tag, "Start RX Prox IR Task");

  while (1)
  {
    if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) != 0u)
    {
      CalculateRxPulseDuration();
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }
  }
}

//_____________________________________________________________________________

static void SetSlotNumber(uint16_t tick)
{
  SlotNumber = tick / 80;
}

//_____________________________________________________________________________

static void ConfigureInputCapture(void)
{
  //mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, IR_SENSE_FRONT_1_PIN);
  //mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_1, IR_SENSE_FRONT_2_PIN);
  //mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_2, IR_SENSE_FRONT_3_PIN);
  //mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_0, IR_SENSE_FRONT_4_PIN);
  //mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_1, IR_SENSE_FRONT_5_PIN);
  //mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_2, IR_SENSE_BACK_LEFT_PIN);
  // IR_SENSE_BACK_RIGHT_PIN is handled by the GPIO ISR

#if 0
  gpio_pulldown_en(IR_SENSE_FRONT_1_PIN);    // Enable pull down on CAP0 signal
  gpio_pulldown_en(IR_SENSE_FRONT_2_PIN);    // Enable pull down on CAP1 signal
  gpio_pulldown_en(IR_SENSE_FRONT_3_PIN);    // Enable pull down on CAP2 signal
  gpio_pulldown_en(IR_SENSE_FRONT_4_PIN);    // Enable pull down on CAP0 signal
  gpio_pulldown_en(IR_SENSE_FRONT_5_PIN);    // Enable pull down on CAP1 signal
  gpio_pulldown_en(IR_SENSE_BACK_LEFT_PIN);  // Enable pull down on CAP2 signal
#endif

#if 0  // TODO Uncomment when the MCPWM library is up-to-date
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);
#endif

  // Enable interrupt on CAP0, CAP1 and CAP2 signal,
  // so each this a rising or falling edge occurs interrupt is triggered
  MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN | CAP2_INT_EN;
  MCPWM[MCPWM_UNIT_1]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN | CAP2_INT_EN;
  mcpwm_isr_register(MCPWM_UNIT_0, ISR_InputCaptureUnit0, NULL, ESP_INTR_FLAG_IRAM, NULL);  // Set ISR Handler
  mcpwm_isr_register(MCPWM_UNIT_1, ISR_InputCaptureUnit1, NULL, ESP_INTR_FLAG_IRAM, NULL);  // Set ISR Handler

  EnableInputCapture();
}

//_____________________________________________________________________________

static void EnableInputCapture(void)
{
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);
}

//_____________________________________________________________________________
#if 0
static void DisableInputCapture(void)
{
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1);
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP2);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP0);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP1);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP2);
}
#endif
//_____________________________________________________________________________

static void CalculateRxPulseDuration(void)
{
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

  // Calculate the RX pulse duration of the 5 front sensors and the back left sensor
  for (uint8_t index = 0u; index <= 5u; index++)
  {
    if (OldPulseCounter[index] != PulseCounter[index])
    {
      //ESP_LOGI(Tag, "index = %d, rise = %d, fall = %d, pulse = %d", index, RisingEdgeCounter[index], FallingEdgeCounter[index], PulseCounter[index]);

      PulseDuration[index] = ((FallingEdgeTime[index] - RisingEdgeTime[index]) / PULSE_DURATION_FACTOR);

      //if (index == 5)
      {
    	//ESP_LOGE(Tag, "COUCOU");
      }

      if (PulseDuration[index] < MAX_PULSE_DURATION)
      {
        vmVariables.prox[index] = PulseDuration[index];
      }
      else
      {
        //ESP_LOGE(Tag, "index = %d, pulse = %d", index, PulseDuration[index]);
        //if (index == 0)
        {
          //ESP_LOGE(Tag, "Pulse duration too high, %d", PulseDuration[0]);
        }
        vmVariables.prox[index] = 0;
      }

      OldPulseCounter[index] = PulseCounter[index];
    }
    else if (RisingEdgeCounter[index] != PulseCounter[index])
    {
      //if (index == 0)
      {
        //ESP_LOGE(Tag, "A, %d ,%d", RisingEdgeCounter[0], PulseCounter[0]);
      }

      portENTER_CRITICAL(&mux);
      RisingEdgeCounter[index] = PulseCounter[index];
      //if (index == 0)
      {
        //ESP_LOGE(Tag, "A1, %d ,%d", RisingEdgeCounter[0], PulseCounter[0]);
      }
      portEXIT_CRITICAL(&mux);
    }
    else if (FallingEdgeCounter[index] != PulseCounter[index])
    {
      //if (index == 0)
      {
    	//ESP_LOGE(Tag, "B, %d ,%d", FallingEdgeCounter[0], PulseCounter[0]);
      }

      portENTER_CRITICAL(&mux);
      FallingEdgeCounter[index] = PulseCounter[index];
      portEXIT_CRITICAL(&mux);
    }
    else  // No obstacle detected (or unfortunately no pulse detected)
    {
      //if (index == 4)
      {
        //ESP_LOGE(Tag, "C");
      }
      vmVariables.prox[index] = 0;
    }
  }

  // Calculate the RX pulse duration of the back right sensor
  PulseCounter[6] = Gpio_GetPulseCounter();

  if (OldPulseCounter[6] != PulseCounter[6])
  {
    PulseDuration[6] = (Gpio_GetFallingEdgeTime() - Gpio_GetRisingEdgeTime()) * 20u;
    vmVariables.prox[6] = PulseDuration[6];

    OldPulseCounter[6] = PulseCounter[6];
  }
  else
  {
    vmVariables.prox[6] = 0;
  }
}

//_____________________________________________________________________________

void ProxIR_EnableNetwork(void)
{
  if (NetworkStatus == E_NetworkStatus_Disabled)
  {
    NetworkStatus = E_NetworkStatus_WillBeEnabled;  // will be fully enabled by the interrupt
  }
}

//_____________________________________________________________________________

void ProxIR_DisableNetwork(void)
{
  NetworkStatus = E_NetworkStatus_Disabled;

  //CLEAR_EVENT(EVENT_DATA);
  vmVariables.rx_data = 0;
  vmVariables.ir_tx_data = 0;
}

//_____________________________________________________________________________

static void Callback_TimerReadRxPulse(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  vTaskNotifyGiveFromISR(RxProxIRTask, &higherPriorityTaskWoken);

  portYIELD_FROM_ISR();
}

//_____________________________________________________________________________

#if 0
static void Callback_TimerTxPulsesGap(void* arg)
{
  // Transmitting the 2nd pulse
  TimerSw_StartTimerOnce(TxPulsesDurationTimer, TX_PULSE_DURATION_us);

  Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_High);
  Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);

  SecondTxPulsesAreInProgress = true;
}
#endif
//_____________________________________________________________________________

static void IRAM_ATTR ISR_InputCaptureUnit0(void* arg)
{
  volatile uint32_t mcpwm_intr_status;

  static uint32_t tmp[CAP_SIG_NUM] = {0, 0, 0};
  static bool risingEdgeDone[CAP_SIG_NUM] = {false, false, false};

  mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val;  // Read interrupt status

  if (mcpwm_intr_status & CAP0_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_0, MCPWM_SELECT_CAP0) == 1)  // Rising edge
    {
      tmp[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);

      RisingEdgeCounter[0]++;
      risingEdgeDone[0] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[0]++;

      if (risingEdgeDone[0])
      {
        risingEdgeDone[0] = false;
        RisingEdgeTime[0] = tmp[0];
        FallingEdgeTime[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
        PulseCounter[0]++;
      }
    }
  }

  if (mcpwm_intr_status & CAP1_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_0, MCPWM_SELECT_CAP1) == 1)  // Rising edge
    {
      tmp[1] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP1);

      RisingEdgeCounter[1]++;
      risingEdgeDone[1] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[1]++;

      if (risingEdgeDone[1])
      {
        risingEdgeDone[1] = false;
        RisingEdgeTime[1] = tmp[1];
        FallingEdgeTime[1] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP1);
        PulseCounter[1]++;
      }
    }
  }

  if (mcpwm_intr_status & CAP2_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_0, MCPWM_SELECT_CAP2) == 1)  // Rising edge
    {
      tmp[2] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP2);

      RisingEdgeCounter[2]++;
      risingEdgeDone[2] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[2]++;

      if (risingEdgeDone[2])
      {
        risingEdgeDone[2] = false;
        RisingEdgeTime[2] = tmp[2];
        FallingEdgeTime[2] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP2);
        PulseCounter[2]++;
      }
    }
  }

  MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_InputCaptureUnit1(void* arg)
{
  volatile uint32_t mcpwm_intr_status;

  static uint32_t tmp[CAP_SIG_NUM] = {0, 0, 0};
  static bool risingEdgeDone[CAP_SIG_NUM] = {false, false, false};

  mcpwm_intr_status = MCPWM[MCPWM_UNIT_1]->int_st.val;  // Read interrupt status

  if (mcpwm_intr_status & CAP0_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_1, MCPWM_SELECT_CAP0) == 1)  // Rising edge
    {
      tmp[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP0);

      RisingEdgeCounter[3]++;
      risingEdgeDone[0] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[3]++;

      if (risingEdgeDone[0])
      {
        risingEdgeDone[0] = false;
        RisingEdgeTime[3] = tmp[0];
        FallingEdgeTime[3] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP0);
        PulseCounter[3]++;
      }
    }
  }

  if (mcpwm_intr_status & CAP1_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_1, MCPWM_SELECT_CAP1) == 1)  // Rising edge
    {
      tmp[1] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP1);

      RisingEdgeCounter[4]++;
      risingEdgeDone[1] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[4]++;

      if (risingEdgeDone[1])
      {
        risingEdgeDone[1] = false;
        RisingEdgeTime[4] = tmp[1];
        FallingEdgeTime[4] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP1);
        PulseCounter[4]++;
      }
    }
  }

  if (mcpwm_intr_status & CAP2_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_1, MCPWM_SELECT_CAP2) == 1)  // Rising edge
    {
      tmp[2] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP2);

      RisingEdgeCounter[5]++;
      risingEdgeDone[2] = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[5]++;

      if (risingEdgeDone[2])
      {
        risingEdgeDone[2] = false;
        RisingEdgeTime[5] = tmp[2];
        FallingEdgeTime[5] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP2);
        PulseCounter[5]++;
      }
    }
  }

  MCPWM[MCPWM_UNIT_1]->int_clr.val = mcpwm_intr_status;
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_EndOfTxPulse(void* para)
{
  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[0].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if (intr_status & BIT(0))
  {
    //Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_Low);
    //Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);

    TIMERG1.int_clr_timers.t0 = 1;
    timer_pause(1, 0);
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_ReadRxPulse(void* para)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;

  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[1].update = 1;

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[1].config.alarm_en = TIMER_ALARM_EN;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if (intr_status & BIT(1))
  {
    vTaskNotifyGiveFromISR(RxProxIRTask, &higherPriorityTaskWoken);

    TIMERG1.int_clr_timers.t1 = 1;
    timer_pause(1, 1);

    if (higherPriorityTaskWoken != pdFALSE)
    {
      portYIELD_FROM_ISR();
    }
  }
}
