//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    bh1745nuc.h
//! \brief   This module provides the useful functions to use the color sensor BH1745NUC
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

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  uint16_t Red;
  uint16_t Green;
  uint16_t Blue;
  uint16_t Clear;
} T_Illuminance;  //!< RGBC illuminance

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the color sensor
//! \pre       None
//! \param     None
//! \return    None
extern void BH1745NUC_Init(void);

//! \brief     Check the manufacturer ID
//! \pre       First initialize the color sensor
//! \param     None
//! \return    E_Error_None if no error, otherwise E_Error_Color_InvalidID
extern T_Error BH1745NUC_CheckManufacturerId(void);

//! \brief     Read the registers
//! \pre       First initialize the color sensor
//! \param     None
//! \return    None
extern void BH1745NUC_ReadRegisters(void);

//! \brief     Read the RGBC illuminance
//! \pre       First initialize the color sensor
//! \param     illuminance - Illuminance RGBC in [lux]
//! \return    None
//! \image     html C:\Users\Vincent\Thymio3\ESP32\documentation\images\bh1745nuc\ReadIlluminance.svg
extern void BH1745NUC_ReadIlluminance(T_Illuminance* illuminance);

#endif // BH1745NUC_H_
