//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    adc.c
//! \brief   This module provides the useful functions to use the internal ADC
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "driver/adc.h"

#include "adc.h"

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

static const char* Tag = "adc";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void ADC_Init(void)
{
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);  // MICROPHONE_PIN
  adc1_config_channel_atten(ADC1_CHANNEL_3, ADC_ATTEN_DB_0);   // IR_SENSE_GROUND_RIGHT_PIN
  adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_0);   // IR_SENSE_GROUND_LEFT_PIN

  ESP_LOGI(Tag, "ADC channels are initialized");
}

//_____________________________________________________________________________

void ADC_AcquireGroundIRValues(uint16_t* value)
{
  value[0] = adc1_get_raw(ADC1_CHANNEL_6);  // IR_SENSE_GROUND_LEFT_PIN
  value[1] = adc1_get_raw(ADC1_CHANNEL_3);  // IR_SENSE_GROUND_RIGHT_PIN
}

//_____________________________________________________________________________

void ADC_AcquireMicrophoneValues(uint16_t* value)
{
  value[2] = adc1_get_raw(ADC1_CHANNEL_0);  // MICROPHONE_PIN
}
