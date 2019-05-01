//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    cosine_generator.c
//! \brief   This module provides the useful functions to use the cosine generator
//!
//! \author  Vincent Gonet
//!
//! \version $Id: cosine_generator.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

#include "driver/dac.h"

#include "cosine_generator.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

int clk_8m_div = 7;       // RTC 8M clock divider (division is by clk_8m_div+1, i.e. 0 means 8MHz frequency)
int frequency_step = 31;  // Frequency step for CW generator
int scale = 0;            // 50% of the full scale
int offset = 0;           // leave it default / 0 = no any offset
int invert = 2;           // invert MSB to get sine waveform

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Set the frequency of the cosine signal
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
static void SetFrequency(int16_t clk_8m_div, int16_t frequency_step);

//! \brief     Set the scale of the cosine signal
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
static void ScaleOutput(int16_t scale);

//! \brief     Set the offset of the cosine signal
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
static void OffsetOutput(int16_t offset);

//! \brief     Set the inversion of the cosine signal
//! \pre       First initialize the cosine generator
//! \param     None
//! \return    None
static void InvertOutput(int16_t invert);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void CosineGenerator_Init(void)
{
  // Enable tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  // Enable / connect tone generator on GPIO25
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
  // Invert MSB, otherwise part of waveform will have inverted
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);

  // Disable / disconnect tone generator on GPIO26 to avoid disruptions on
  // IR_PULSE_BACK_PIN (GPIO26)
  CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);

  dac_output_enable(DAC_CHANNEL_1);
}

//_____________________________________________________________________________

void CosineGenerator_Enable(void)
{
  // Enable tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  dac_output_enable(DAC_CHANNEL_1);
}

//_____________________________________________________________________________

void CosineGenerator_Disable(void)
{
  // Disable tone generator
  CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  dac_output_disable(DAC_CHANNEL_1);
}

//_____________________________________________________________________________

void CosineGenerator_ConfigureSignal(int16_t noteName, int16_t noteDynamics)
{
  SetFrequency(clk_8m_div, noteName);
  ScaleOutput(noteDynamics);
  OffsetOutput(offset);
  InvertOutput(invert);
}

//_____________________________________________________________________________

static void SetFrequency(int16_t clk_8m_div, int16_t frequency_step)
{
  REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequency_step, SENS_SW_FSTEP_S);
}

//_____________________________________________________________________________

static void ScaleOutput(int16_t scale)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
}

//_____________________________________________________________________________

static void OffsetOutput(int16_t offset)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, offset, SENS_DAC_DC1_S);
}

//_____________________________________________________________________________

static void InvertOutput(int16_t invert)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, invert, SENS_DAC_INV1_S);
}
