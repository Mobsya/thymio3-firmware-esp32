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

#define PROX_IR_PIN_NUM      2u  // FIXME

#define SENSORS_NUM          7u

#define CALIB_HYSTERESIS     20

#define DEFAULT_CALIB    0x7FFF

#define CAP0_INT_EN     BIT(27)  // Capture 0 interrupt bit
#define CAP1_INT_EN     BIT(28)  // Capture 1 interrupt bit
#define CAP2_INT_EN     BIT(29)  // Capture 2 interrupt bit

#define CAP_SIG_NUM          3u  // Three capture signals

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

typedef struct
{
  uint32_t RisingEdge;
  uint32_t FallingEdge;
  mcpwm_capture_signal_t sel_cap_signal;
} Capture;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "prox_ir";

static const T_GpioPinConfig PinConfig[PROX_IR_PIN_NUM] =
{
  // PinNumber                Mode               Resistor             Level            Interrupt
  {IR_PULSE_BACK_PIN,  E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable},
  {IR_PULSE_FRONT_PIN, E_GpioMode_Output, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_Disable}
};

static uint8_t ProxCalibMaxCounter[SENSORS_NUM];
static uint16_t LoopbackDelay[SENSORS_NUM];

static T_NetworkStatus NetworkStatus = E_NetworkStatus_Disabled;

static uint16_t last_tx;

static uint8_t edge[SENSORS_NUM];

static bool FrontPulseIsInProgress = false;
static bool BackPulseIsInProgress = false;

xQueueHandle cap_queue;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static int16_t PerformCalibration(int16_t raw, T_Sensor sensor);

static int16_t Calibrate(int16_t value, T_Sensor sensor);

static void ResetRx(void);

static void ir_tx(int value);

static mcpwm_dev_t* MCPWM[2] = {&MCPWM0, &MCPWM1};

static void IRAM_ATTR isr_handler();

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
  for (uint8_t sensor = 0; sensor < SENSORS_NUM; sensor++)
  {
    if (settings.prox_min[sensor] == 0)
    {
      settings.prox_min[sensor] = DEFAULT_CALIB;
    }
  }

  cap_queue = xQueueCreate(1, sizeof(Capture));

  // TODO configuration of Capture, Timers, ...
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_0, IR_SENSE_FRONT_1_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_1, IR_SENSE_FRONT_2_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM_CAP_2, IR_SENSE_FRONT_3_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_0, IR_SENSE_FRONT_4_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_1, IR_SENSE_FRONT_5_PIN);
  mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_2, IR_SENSE_BACK_LEFT_PIN);

  // IR_SENSE_BACK_RIGHT_PIN is handled by the internal ADC

  gpio_pulldown_en(IR_SENSE_FRONT_1_PIN);    // Enable pull down on CAP0 signal
  gpio_pulldown_en(IR_SENSE_FRONT_2_PIN);    // Enable pull down on CAP1 signal
  gpio_pulldown_en(IR_SENSE_FRONT_3_PIN);    // Enable pull down on CAP2 signal
  gpio_pulldown_en(IR_SENSE_FRONT_4_PIN);    // Enable pull down on CAP0 signal
  gpio_pulldown_en(IR_SENSE_FRONT_5_PIN);    // Enable pull down on CAP1 signal
  gpio_pulldown_en(IR_SENSE_BACK_LEFT_PIN);  // Enable pull down on CAP2 signal

//#if 0  // TODO Uncomment when the MCPWM library is up-to-date
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_0, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP0, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP1, MCPWM_BOTH_EDGE, 0);
  mcpwm_capture_enable(MCPWM_UNIT_1, MCPWM_SELECT_CAP2, MCPWM_BOTH_EDGE, 0);

  // Enable interrupt on CAP0, CAP1 and CAP2 signal,
  // so each this a rising or falling edge occurs interrupt is triggered
  MCPWM[MCPWM_UNIT_0]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN | CAP2_INT_EN;
//  MCPWM[MCPWM_UNIT_1]->int_ena.val = CAP0_INT_EN | CAP1_INT_EN | CAP2_INT_EN;    // Enable interrupt on  CAP0, CAP1 and CAP2 signal
  mcpwm_isr_register(MCPWM_UNIT_0, isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL); // Set ISR Handler
//  mcpwm_isr_register(MCPWM_UNIT_1, isr_handler, NULL, ESP_INTR_FLAG_IRAM, NULL); // Set ISR Handler
//#endif

  ESP_LOGI(Tag, "Proximity IR sensors are initialized");
}

//_____________________________________________________________________________

int16_t ProxIR_EmitPulses(uint16_t tick)
{
  switch (tick)
  {
    case 5:
      TimerHw_StartFrontTimer60us();

      Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_High);
      FrontPulseIsInProgress = true;
      break;

    case 6:
      TimerHw_StartBackTimer60us();

      Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);
      BackPulseIsInProgress = true;
      break;

    case 11:
      //Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);
      break;

    default:
      // Do nothing
      break;
  }

  return 0;
}

#if 0
int16_t ProxIR_Run(uint16_t tick)
{
  int16_t ret = 0;

  switch (tick)
  {
    case 0:
      ResetRx();

      if (NetworkStatus == E_NetworkStatus_WillBeEnabled)
      {
        NetworkStatus = E_NetworkStatus_Enabled;
        ir_tx(vmVariables.ir_tx_data);
      }
      else
      {
        ir_tx(-1);
      }
      break;

    case 5: // First pulse should be emitted by now. (max pulse: 8000, emition time: 960, back is delayed by 960: 9920 => 5.
//      ret = ir_prox_rx_oa();
      break;

    default:
      if ((NetworkStatus == E_NetworkStatus_Enabled) && last_tx)
      {
        if (last_tx == 6)
        {
//          ret = ir_prox_check_2nd_pulse(); // If we just sent the 2nd pulse, check if there is contention.
          ResetRx();
          last_tx++;
        }
        else if (last_tx > 6)
        {
          // We sent the two pulses, now start to listen
//          ret = ir_prox_rx(tick);
        }
        else
        {
          // Wait the local echo
          last_tx++;
        }
      }
      break;
  }

  if (NetworkStatus != E_NetworkStatus_Enabled)
  {
    ret = 0;
  }

  return ret;
}
#endif

//_____________________________________________________________________________

void ProxIR_ReadPulseDuration(void)
{
  static uint8_t index = 0;

  uint32_t* current_cap_value = (uint32_t*)malloc(sizeof(CAP_SIG_NUM));
  uint32_t* previous_cap_value = (uint32_t*)malloc(sizeof(CAP_SIG_NUM));
  uint32_t duration = 0;

  uint32_t pulse[2];

  Capture evt;

  xQueueReceive(cap_queue, &evt, portMAX_DELAY);

  //printf("R %d\n", evt.RisingEdge);
  //printf("F %d\n", evt.FallingEdge);

  //pulse[0] = (evt.FallingEdge - evt.RisingEdge);
  //pulse[0] = ((evt.FallingEdge - evt.RisingEdge) * (1000000 / rtc_clk_apb_freq_get()));
  //pulse[0] = rtc_clk_apb_freq_get();

  //pulse[0] = ((evt.FallingEdge - evt.RisingEdge) / 10000) * (10000000000 / rtc_clk_apb_freq_get());
  //pulse[0] = ((evt.FallingEdge - evt.RisingEdge) / 10) (100000000 / rtc_clk_apb_freq_get());
  pulse[0] = ((evt.FallingEdge - evt.RisingEdge) / 1000) * (1000000000 / rtc_clk_apb_freq_get());
  //pulse[0] = ((evt.RisingEdge - evt.FallingEdge) / rtc_clk_apb_freq_get());

  printf("%d\n", pulse[0]);




  //current_cap_value[0] = evt.capture_signal - previous_cap_value[0];
  //previous_cap_value[0] = evt.capture_signal;
  //current_cap_value[0] = (current_cap_value[0] / 10000) * (10000000000 / rtc_clk_apb_freq_get());
  //printf("CAP0 : previous %d us\n", previous_cap_value[0]);
  //printf("CAP0 : %d us\n", current_cap_value[0]);
  //printf("%d\n", current_cap_value[0]);
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
  for (uint8_t sensor = 0u; sensor < SENSORS_NUM; sensor++)
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

void TimerHw_CallbackFront60us(void* arg)
{
  if (FrontPulseIsInProgress)
  {
    Gpio_SetPinLevel(IR_PULSE_FRONT_PIN, E_GpioLevel_Low);
    FrontPulseIsInProgress = false;

    //TimerHw_StartTimer60us();

    //Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_High);
    //BackPulseIsInProgress = true;
  }
}

//_____________________________________________________________________________

void TimerHw_CallbackBack60us(void* arg)
{
  if (BackPulseIsInProgress)
  {
    Gpio_SetPinLevel(IR_PULSE_BACK_PIN, E_GpioLevel_Low);
    BackPulseIsInProgress = false;
  }
}

//_____________________________________________________________________________

static void IRAM_ATTR isr_handler()
{
  uint32_t mcpwm_intr_status;
  Capture evt;
  static uint8_t index;
  static uint32_t tmp = 0;

  mcpwm_intr_status = MCPWM[MCPWM_UNIT_0]->int_st.val;  // Read interrupt status

  if (mcpwm_intr_status & CAP0_INT_EN)  // Check for interrupt on rising edge or falling edge on CAP0 signal
  {
    if ((index % 2) == 0)  // Rising edge
    {
      tmp = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);  // Get capture signal counter value
    }
    else  // Falling edge
    {
      evt.RisingEdge = tmp;
      evt.FallingEdge = mcpwm_capture_signal_get_value(MCPWM_UNIT_0, MCPWM_SELECT_CAP0);  // Get capture signal counter value
      evt.sel_cap_signal = MCPWM_SELECT_CAP0;
      xQueueSendFromISR(cap_queue, &evt, NULL);
    }

    index++;
  }

  MCPWM[MCPWM_UNIT_0]->int_clr.val = mcpwm_intr_status;
}
