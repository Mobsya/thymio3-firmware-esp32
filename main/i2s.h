//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    i2s.h
//! \brief   This module provides the useful functions to use the I2S peripheral
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef I2S_H_
#define I2S_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

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

//! \brief     Initialize the I2S peripheral
//! \pre       None
//! \param     None
//! \return    None
extern void I2S_Init(void);

extern void I2S_EnableDAC(void);

extern void I2S_DisableDAC(void);

extern void I2S_EnableADC(void);

extern void I2S_Record(void);

extern void I2S_StartReading(void);

extern void I2S_Process(void);

extern void I2S_AcquireMicrophoneValues(void);

extern void I2S_Write(void);

extern void I2S_Read(void);

extern void I2S_ReadFromFlash(void);

#endif // I2S_H_
