//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: bh1745nuc.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef BH1745NUC_H_
#define BH1745NUC_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

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
//! \return    None
extern void BH1745NUC_CheckManufacturerId(void);

extern void BH1745NUC_ReadRegisters(void);

//! \brief     Get the RGBC illuminance in [lux]
//! \pre       First initialize the color sensor
//! \param     illuminance - Illuminance RGBC in [lux]
//! \return    None
extern void BH1745NUC_GetIlluminance_lux(T_Illuminance* illuminance);

#endif // BH1745NUC_H_
