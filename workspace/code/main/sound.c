//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sound.c
//! \brief   This module provides the useful functions to generate the sound
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sound.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

#include "driver/dac.h"

#include "sound.h"

#include "board.h"
#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SINE_TABLE_SIZE    256

//i2s number
#define I2S_NUM (0)

//i2s data bits
#define I2S_SAMPLE_BITS (16)

//I2S read buffer length
#define I2S_READ_LEN (16 * 1024)

//i2s sample rate
#define I2S_SAMPLE_RATE (16000)

//I2S data format
#define I2S_FORMAT (I2S_CHANNEL_FMT_RIGHT_LEFT)

//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))

#define ADC_UNIT_NUM      ADC_UNIT_1

#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sound";

static const T_GpioPinConfig PinConfig = {GPIO0_PIN,
                                          E_GpioMode_Output,
                                          E_GpioResistor_None,
                                          E_GpioLevel_Low,
                                          E_GpioInterrupt_Disable};

int clk_8m_div = 4;      // RTC 8M clock divider (division is by clk_8m_div+1, i.e. 0 means 8MHz frequency)
int frequency_step = 8;  // Frequency step for CW generator
int scale = 1;           // 50% of the full scale
int offset;              // leave it default / 0 = no any offset
int invert = 2; // invert MSB to get sine waveform

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sound_Init(void)
{
  // Enable tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  // Enable / connect tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
  // Invert MSB, otherwise part of waveform will have inverted
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);

  dac_output_enable(DAC_CHANNEL_1);
}

//_____________________________________________________________________________

void Sound_Task(int16_t note)
{
  Sound_SetFrequency(clk_8m_div, note);

    /* Tune parameters of channel 2 only
     * to see and compare changes against channel 1
     */
  Sound_ScaleOutput(scale);
  Sound_OffsetOutput(offset);
  Sound_InvertOutput(invert);

  float frequency = RTC_FAST_CLK_FREQ_APPROX / (1 + clk_8m_div) * (float) note / 65536;
  printf("clk_8m_div: %d, frequency step: %d, frequency: %.0f Hz\n", clk_8m_div, note, frequency);
  //printf("DAC2 scale: %d, offset %d, invert: %d\n", scale, offset, invert);
}

//_____________________________________________________________________________

void Sound_SetFrequency(int16_t clk_8m_div, int16_t frequency_step)
{
  REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequency_step, SENS_SW_FSTEP_S);
}

//_____________________________________________________________________________

void Sound_ScaleOutput(int16_t scale)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
}

//_____________________________________________________________________________

void Sound_OffsetOutput(int16_t offset)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, offset, SENS_DAC_DC1_S);
}

//_____________________________________________________________________________

void Sound_InvertOutput(int16_t invert)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, invert, SENS_DAC_INV1_S);
}
