//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    imu_common.h
//! \brief   This module provides the useful functions to use the gyroscope
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef IMU_COMMON_H_
#define IMU_COMMON_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#include "gpio.h"
#include "pins_def.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------
typedef struct
{
  int16_t X;
  int16_t Y;
  int16_t Z;
} T_Axis;  //!< Axis

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------
static const T_GpioPinConfig PinConfig[2] =
{
  {ACC_INT1_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_RisingEdge},
  {ACC_INT2_PIN, E_GpioMode_Input, E_GpioResistor_None, E_GpioLevel_Low, E_GpioInterrupt_RisingEdge}
};

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

#endif // IMU_COMMON_H_
