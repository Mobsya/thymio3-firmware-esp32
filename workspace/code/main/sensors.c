//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sensors.c
//! \brief   This module provides the useful functions to use the no-I2C sensors
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sensors.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "sensors.h"

#include "adc.h"
#include "ground_ir.h"
#include "leds.h"
#include "prox_ir.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PERIOD_100_ms   799u  // 125us * 800 = 100ms

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sensors";

static int16_t PeriodAccumulator = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sensors_Init(void)
{
  ADC_Init();
  //Leds_Init();

  GroundIR_Init();
  ProxIR_Init();
  TimerHw_Init();

  ESP_LOGI(Tag, "Sensors are initialized");
}

//_____________________________________________________________________________

void Sensors_RunTask(void)
{
  static uint16_t tick = 0u;

  int16_t sensors[4];
//#if 0
  //if ((tick == 7) || (tick == 50) || (tick == 53) || (tick == 56))
  if ((tick == 50) || (tick == 53) || (tick == 56))
  {
    ADC_AcquireValues(sensors);
  }
//#endif
  //Leds_RunTask();

  //PeriodAccumulator += ProxIR_EmitPulses(tick, sensors[3]);
  //PeriodAccumulator = 0;

  if (PeriodAccumulator > 100)
  {
    PeriodAccumulator = 100;
  }

  if (PeriodAccumulator < -100)
  {
    PeriodAccumulator = -100;
  }

  // The ground IR sensors need a period of 100ms (frequency = 10Hz),
  // The ground IR sensors trigger at time = 50, need 6 cycles
  GroundIR_EmitPulses(tick, sensors[1], sensors[2]);
  //GroundIR_EmitPulses(0, 0, tick);

  if (PeriodAccumulator < 0)
  {
    if (tick == PERIOD_100_ms)
    {
      PeriodAccumulator++;
      tick++;
    }
    else if (tick > PERIOD_100_ms)
    {
      tick = 0u;
    }
    else
    {
      tick++;
    }
  }
  else if (PeriodAccumulator > 0)
  {
    if (tick == (PERIOD_100_ms - 1u))
    {
      PeriodAccumulator--;
      tick = 0u;
    }
    else if (tick++ >= PERIOD_100_ms)
    {
      // We have to re-check as per_acc might be set when time == PERIOD_100MS
      tick = 0u;
    }
    else
    {
      // Do nothing
    }
  }
  else
  {
    if (tick++ >= PERIOD_100_ms)
    {
      tick = 0u;
    }
  }
}
