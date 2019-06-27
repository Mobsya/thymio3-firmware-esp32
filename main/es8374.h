//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    es8374.h
//! \brief   This module provides the useful functions to use the audio codec ES8374
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef BH1745NUC_H_
#define BH1745NUC_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "audio_hal.h"

#include "error.h"

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
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the external audio codec
//! \pre       None
//! \param     None
//! \return    None
extern esp_err_t ES8374_Init(audio_hal_codec_config_t *cfg);

//! \brief     De-initialize the external audio codec
//! \pre       None
//! \param     None
//! \return    None
extern esp_err_t ES8374_Deinit(void);

//! \brief     Configure the I2S protocole
//! \pre       First initialize the codec
//! \param     None
//! \return    None
extern esp_err_t ES8374_ConfigureI2S(audio_hal_codec_mode_t mode, audio_hal_codec_i2s_iface_t *iface);

//min volume = 0; max volume = 96
extern esp_err_t ES8374_SetVoiceVolume(int volume);

extern esp_err_t ES8374_GetVoiceVolume(int *volume);

extern esp_err_t ES8374_ControlState(audio_hal_codec_mode_t mode, audio_hal_ctrl_t ctrl_state);

#endif // BH1745NUC_H_
