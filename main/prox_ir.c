//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: prox_ir.c 18076 2017-04-20 12:28:12Z v.gonet $
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

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PROX_IR_PIN_NUM          3u

#define CALIB_HYSTERESIS         20

#define DEFAULT_CALIB        0x7FFF

#define CAP0_INT_EN         BIT(27)  // Capture 0 interrupt bit
#define CAP1_INT_EN         BIT(28)  // Capture 1 interrupt bit
#define CAP2_INT_EN         BIT(29)  // Capture 2 interrupt bit

#define CAP_SIG_NUM              3u  // Three capture signals

#define MAX_PULSE_DURATION    4600u

#define PULSE_DURATION_FACTOR    4u  // Factor to adapt the pulse duration

#define TX_PULSE_DURATION_us    60u

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

static const T_GpioPinConfig PinConfig[PROX_IR_PIN_NUM] =
{
  // PinNumber              Mode               Resistor             Level            Interrupt
  {IR_PULSE_BACK_PIN,       E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_PULSE_FRONT_PIN,      E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_SENSE_BACK_RIGHT_PIN, E_GpioMode_Input,  E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_AnyEdge}
};
// Other pins are configured by the input capture module

static uint8_t ProxCalibMaxCounter[PROX_IR_SENSORS_NUM];
static uint16_t LoopbackDelay[PROX_IR_SENSORS_NUM];

static T_NetworkStatus NetworkStatus = E_NetworkStatus_Disabled;

static uint16_t last_tx;

static uint8_t edge[PROX_IR_SENSORS_NUM];

static bool FirstTxPulsesAreInProgress = false;
static bool SecondTxPulsesAreInProgress = false;

static volatile uint32_t PulseCounter[7]    = {0u, 0u, 0u, 0u, 0u, 0u, 0u};
static uint32_t OldPulseCounter[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};

static volatile uint32_t RisingEdgeTime[7]  = {0u, 0u, 0u, 0u, 0u, 0u, 0u};
static volatile uint32_t FallingEdgeTime[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};

static volatile uint32_t RisingEdgeCounter[7]  = {0u, 0u, 0u, 0u, 0u, 0u, 0u};  // For debug only
static volatile uint32_t FallingEdgeCounter[7] = {0u, 0u, 0u, 0u, 0u, 0u, 0u};  // For debug only

static uint32_t PulseDuration[7];

static T_TimerHw* TxPulsesDurationTimer = NULL;  //!< Timer used to determine the duration of the TX pulses
static T_TimerHw* TxPulsesGapTimer = NULL;       //!< Timer used to determine the gap between two TX pulses

static uint8_t SlotNumber = 0xFFu;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void ConfigureInputCapture(void);

static void ReadUnit0PulseDuration(void);

static void EnableInputCapture(void);

static void DisableInputCapture(void);

static void ReadUnit1PulseDuration(void);

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor);

static int16_t Calibrate(int16_t value, T_Sensor sensor);

static void ResetRx(void);

static void ir_tx(int value);

static mcpwm_dev_t* MCPWM[2] = {&MCPWM0, &MCPWM1};

static void Callback_TimerTxPulsesDuration(void* arg);
static void Callback_TimerTxPulsesGap(void* arg);

static void IRAM_ATTR ISR_InputCaptureUnit0(void* arg);
static void IRAM_ATTR ISR_InputCaptureUnit1(void* arg);

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
    Gpio_ConfigurePin(&PinConfig[index]);
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

  TxPulsesDurationTimer = TimerHw_Create(TX_PULSE_DURATION_us, Callback_TimerTxPulsesDuration);
  TxPulsesGapTimer      = TimerHw_Create(200, Callback_TimerTxPulsesGap);

  ConfigureInputCapture();

  //EnableInputCapture();

  ESP_LOGI(Tag, "Proximity IR sensors are initialized");
}

//_____________________________________________________________________________

void ProxIR_Run(uint16_t tick)
{
  switch (tick)
  {
    case 4:
      EnableInputCapture();
      break;

    case 5:
      // Transmitting the first pulse
      TimerHw_StartTimerOnce(TxPulsesDurationTimer, TX_PULSE_DURATION_us);

      Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_High);
      Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);

      FirstTxPulsesAreInProgress = true;
      break;

    case 11:
      ReadUnit0PulseDuration();
      break;

    case 12:
      ReadUnit1PulseDuration();

      //DisableInputCapture();
      break;

    default:
      // Do nothing
      break;
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
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, IR_SENSE_FRONT_1_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_1, IR_SENSE_FRONT_2_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_2, IR_SENSE_FRONT_3_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_0, IR_SENSE_FRONT_4_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_1, IR_SENSE_FRONT_5_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_2, IR_SENSE_BACK_LEFT_PIN);
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

static void DisableInputCapture(void)
{
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1);
  mcpwm_capture_disable(MCPWM_UNIT_0, MCPWM_SELECT_CAP2);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP0);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP1);
  mcpwm_capture_disable(MCPWM_UNIT_1, MCPWM_SELECT_CAP2);
}

//_____________________________________________________________________________

static void ReadUnit0PulseDuration(void)
{
  for (uint8_t index = 0u; index <= 2u; index++)
  {
    if (OldPulseCounter[index] != PulseCounter[index])
    {
      //ESP_LOGI(Tag, "index = %d, rise = %d, fall = %d, pulse = %d", index, RisingEdgeCounter[index], FallingEdgeCounter[index], PulseCounter[index]);

      PulseDuration[index] = ((FallingEdgeTime[index] - RisingEdgeTime[index]) / PULSE_DURATION_FACTOR);

      if (PulseDuration[index] < MAX_PULSE_DURATION)
      {
        vmVariables.prox[index] = PulseDuration[index];
      }
      else
      {
        //ESP_LOGE(Tag, "index = %d, pulse = %d", index, PulseDuration[index]);
        vmVariables.prox[index] = 0;
      }

      OldPulseCounter[index] = PulseCounter[index];
    }
    else if (RisingEdgeCounter[index] != PulseCounter[index])
    {
      RisingEdgeCounter[index] = PulseCounter[index];
    }
    else if (FallingEdgeCounter[index] != PulseCounter[index])
    {
      FallingEdgeCounter[index] = PulseCounter[index];
    }
    else  // No obstacle detected (or unfortunately no pulse detected)
    {
      vmVariables.prox[index] = 0;

#if 0
      if (index == 2u)
      {
        ESP_LOGE(Tag, "index = %d, rise = %d, fall = %d, pulse = %d", index, RisingEdgeCounter[index],
                 FallingEdgeCounter[index], PulseCounter[index]);
      }
#endif
    }
  }
}

//_____________________________________________________________________________

static void ReadUnit1PulseDuration(void)
{
  for (uint8_t index = 3u; index <= 5u; index++)
  {
    if (OldPulseCounter[index] != PulseCounter[index])
    {
      // FIXME ESP_LOGI(Tag, "index = %d, rise = %d, fall = %d, pulse = %d", index, RisingEdgeCounter[index], FallingEdgeCounter[index], PulseCounter[index]);

      PulseDuration[index] = ((FallingEdgeTime[index] - RisingEdgeTime[index]) / PULSE_DURATION_FACTOR);

      if (PulseDuration[index] < MAX_PULSE_DURATION)
      {
        vmVariables.prox[index] = PulseDuration[index];
      }
      else
      {
        // FIXME ESP_LOGE(Tag, "index = %d, pulse = %d", index, PulseDuration[index]);
        vmVariables.prox[index] = 0;
      }

      OldPulseCounter[index] = PulseCounter[index];
    }
    else if (RisingEdgeCounter[index] != PulseCounter[index])
    {
      RisingEdgeCounter[index] = PulseCounter[index];
    }
    else if (FallingEdgeCounter[index] != PulseCounter[index])
    {
      FallingEdgeCounter[index] = PulseCounter[index];
    }
    else
    {
      vmVariables.prox[index] = 0;
      // FIXME ESP_LOGE(Tag, "index = %d, rise = %d, fall = %d, pulse = %d", index, RisingEdgeCounter[index], FallingEdgeCounter[index], PulseCounter[index]);
    }
  }

  PulseCounter[6] = Gpio_GetPulseCounter();

  if (OldPulseCounter[6] != PulseCounter[6])
  {
    PulseDuration[6] = (Gpio_GetFallingEdgeTime() - Gpio_GetRisingEdgeTime()) * 4u;
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

  CLEAR_EVENT(EVENT_DATA);
  vmVariables.rx_data = 0;
  vmVariables.ir_tx_data = 0;
}

//_____________________________________________________________________________

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor)
{
  int16_t value = raw;
  int16_t calibration = 0;

  if (settings.prox_min[sensor] > 0)
  {
    // On the fly re-calibration
    calibration = Calibrate(value, sensor);
  }
  else
  {
    // Calibration disabled if settings are negative
    calibration = value;
  }

  return calibration;
}

//_____________________________________________________________________________

static int16_t Calibrate(int16_t value, T_Sensor sensor)
{
  int16_t ret;

  if ((value - CALIB_HYSTERESIS) < settings.prox_min[sensor])
  {
    if (++ProxCalibMaxCounter[sensor] > 3)
    {
      if (value < settings.prox_min[sensor])
      {
        settings.prox_min[sensor] = value;
        set_save_settings();
      }
      else
      {
        ProxCalibMaxCounter[sensor] = 0;
      }
    }
  }
  else
  {
    ProxCalibMaxCounter[sensor] = 0;
  }

  if (settings.prox_min[sensor] == DEFAULT_CALIB)
  {
    ret = value;
  }
  else
  {
    ret = value - (3 * ((unsigned int) settings.prox_min[sensor])) / 4 + 800;
  }

  if (ret < 0)
  {
    ret = 0;
  }

  return ret;
}

//_____________________________________________________________________________

static void ResetRx(void)
{
  for (uint8_t sensor = 0u; sensor < PROX_IR_SENSORS_NUM; sensor++)
  {
#if 0
    while (ic_bufne(sensor))
    {
      ic_buf(sensor);
    }
#endif
    edge[sensor] = 0;
  }
}

//_____________________________________________________________________________
#if 0
static int16_t ir_prox_rx_oa(void)
{
  int16_t temp[2];
  int16_t ret = 0;

  for (int16_t sensor = 0; sensor < SENSORS_NUM; sensor++)
  {
    vmVariables.prox[sensor] = 0;
    LoopbackDelay[sensor] = 0;

    if (ic_bufne(sensor))
    {
      temp[0] = ic_buf(sensor);

      if (ic_bufne(sensor))
      {
        temp[1] = ic_buf(sensor);
        LoopbackDelay[sensor] = temp[0];

        // Validity check
        if (temp[0] < 2000)
        {
          if (temp[0] > 100)
          {
            vmVariables.prox[sensor] = PerformCalibration((temp[1] - temp[0]), sensor);
          }
          else if (!sensors_prox_drift())
          {
            ret = ((temp[0] & 0x3) - 2) * 5; // Should use random here ....
          }
        }
        else if (!sensors_prox_drift())
        {
          ret = ((temp[0] & 0x3) - 1) * 5; // Should use random here ....
        }
      }
    }

    // Reset the calibration counter if the sensor did not see something:
    // => perform_calib was not triggered, thus, ProxCalibMaxCounter was not updated
    // So, do it here.
    if (!vmVariables.prox[sensor])
    {
      ProxCalibMaxCounter[sensor] = 0;
    }
  }

  return ret;
}
#endif
//_____________________________________________________________________________

static void ir_tx(int value)
{

}

//_____________________________________________________________________________

static void Callback_TimerTxPulsesDuration(void* arg)
{
  if (FirstTxPulsesAreInProgress)
  {
    Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_Low);
    Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);

    // Start the timer to determine the gap with the 2nd pulse
    TimerHw_StartTimerOnce(TxPulsesGapTimer, 200);

    FirstTxPulsesAreInProgress = false;
  }
  else if (SecondTxPulsesAreInProgress)
  {
    Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_Low);
	Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);

	SecondTxPulsesAreInProgress = false;
  }
  else
  {
    // Do nothing
  }
}

//_____________________________________________________________________________
#if 0
static void Callback_TimerBackPulses(void* arg)
{
  if (BackPulseIsInProgress)
  {
    Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);
    BackPulseIsInProgress = false;
  }
}
#endif

//_____________________________________________________________________________

static void Callback_TimerTxPulsesGap(void* arg)
{
  // Transmitting the 2nd pulse
  TimerHw_StartTimerOnce(TxPulsesDurationTimer, TX_PULSE_DURATION_us);

  Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_High);
  Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);

  SecondTxPulsesAreInProgress = true;
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_InputCaptureUnit0(void* arg)
{
  volatile uint32_t mcpwm_intr_status;

  static uint32_t tmp2nd[CAP_SIG_NUM] = {0, 0, 0};
  static uint32_t tmp[CAP_SIG_NUM] = {0, 0, 0};
  static bool risingEdgeFront1Done = false;
  static bool risingEdgeFront2Done = false;
  static bool risingEdgeFront3Done = false;

  static uint8_t tmpSlot = 0;

  mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val;  // Read interrupt status

  if (mcpwm_intr_status & CAP0_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_0, MCPWM_SELECT_CAP0) == 1)  // Rising edge
    {
      tmp[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);

      RisingEdgeCounter[0]++;
      risingEdgeFront1Done = true;

      // Rising edge received during the same slot
      if (SlotNumber == tmpSlot)
      {
    	tmp2nd[0] = tmp[0];
      }
    }
    else  // Falling edge
    {
      FallingEdgeCounter[0]++;

      if (risingEdgeFront1Done)
      {
        risingEdgeFront1Done = false;
        RisingEdgeTime[0] = tmp[0];
        tmpSlot = SlotNumber;
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
      risingEdgeFront2Done = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[1]++;

      if (risingEdgeFront2Done)
      {
        risingEdgeFront2Done = false;
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
      risingEdgeFront3Done = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[2]++;

      if (risingEdgeFront3Done)
      {
        risingEdgeFront3Done = false;
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
  uint32_t mcpwm_intr_status;

  static uint32_t tmp[CAP_SIG_NUM] = {0, 0, 0};
  static bool risingEdgeFront4Done = false;
  static bool risingEdgeFront5Done = false;
  static bool risingEdgeFront6Done = false;

  mcpwm_intr_status = MCPWM[MCPWM_UNIT_1]->int_st.val;  // Read interrupt status

  if (mcpwm_intr_status & CAP0_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if (mcpwm_capture_signal_get_edge(MCPWM_UNIT_1, MCPWM_SELECT_CAP0) == 1)  // Rising edge
    {
      tmp[0] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP0);

      RisingEdgeCounter[3]++;
      risingEdgeFront4Done = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[3]++;

      if (risingEdgeFront4Done)
      {
        risingEdgeFront4Done = false;
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
      risingEdgeFront5Done = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[4]++;

      if (risingEdgeFront5Done)
      {
        risingEdgeFront5Done = false;
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
      risingEdgeFront6Done = true;
    }
    else  // Falling edge
    {
      FallingEdgeCounter[5]++;

      if (risingEdgeFront6Done)
      {
        risingEdgeFront6Done = false;
        RisingEdgeTime[5] = tmp[2];
        FallingEdgeTime[5] = mcpwm_capture_signal_get_value(MCPWM_UNIT_1, MCPWM_SELECT_CAP2);
        PulseCounter[5]++;
      }
    }
  }

  MCPWM[MCPWM_UNIT_1]->int_clr.val = mcpwm_intr_status;
}
