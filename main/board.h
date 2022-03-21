//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    board.h
//! \brief   This module configures the board
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef BOARD_H_
#define BOARD_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MAX_NUMBER_PIN               40u  //!< Maximum number of pins

//*****************************************************************************
// Pins mapping
//*****************************************************************************

// Power and boot pins
#define GPIO0_PIN                     0u  //!< GPIO0 pin is GPIO0

// LEDs pins (HSPI)
#define LED_CLK_PIN                  14u  //!< LED_CLK pin is GPIO14
#define LED_SDI_PIN                  13u  //!< LED_SDI pin is GPIO13
#define LED_CS_PIN                   15u  //!< LED_CS pin is GPIO15

// Inter-Processors SPI (VSPI)
#define SPI_CLK_PIN                  18u  //!< SPI_CLK pin is GPIO18
#define SPI_MISO_PIN                 19u  //!< SPI_MISO pin is GPIO19
#define SPI_MOSI_PIN                 23u  //!< SPI_MOSI pin is GPIO23
#define SPI_CS_PIN                    5u  //!< SPI_CS pin is GPIO5

// Capacitive buttons pins
#define BUTTON_BACKWARD_PIN           2u  //!< BUTTON_BACKWARD pin is GPIO2 (Channel T2)
#define BUTTON_LEFT_PIN              12u  //!< BUTTON_LEFT pin is GPIO12 (Channel T5)
#define BUTTON_CENTER_PIN            27u  //!< BUTTON_CENTER pin is GPIO27 (Channel T7)
#define BUTTON_FORWARD_PIN           32u  //!< BUTTON_FORWARD pin is GPIO32 (Channel T9)
#define BUTTON_RIGHT_PIN             33u  //!< BUTTON_RIGHT pin is GPIO33 (Channel T8)

// Mechanical button pins
#define BUTTON_SIDE_PIN              34u  //!< BUTTON_SIDE pin is GPIO34

// Sensors interrupt pins
#define ACC_INT1_PIN                 36u  //!< ACC_INT pin is GPIO36
#define ACC_INT2_PIN                 39u  //!< ACC_INT pin is GPIO39

// I2C pins
#define SDA_PIN                      21u  //!< SDA pin is GPIO21
#define SCL_PIN                      22u  //!< SCL pin is GPIO22

// I2S pins
#define I2S_MCLK_PIN                 17u  //!< I2S_MCLK pin is GPIO17
#define I2S_SCLK_PIN                 16u  //!< I2S_SCLK pin is GPIO16
#define I2S_LCLK_PIN                  4u  //!< I2S_LCLK pin is GPIO04
#define I2S_DSIN_PIN                 26u  //!< I2S_DSIN pin is GPIO26
#define I2S_DOUT_PIN                 25u  //!< I2S_DOUT pin is GPIO25

// UART pins
#define TXD_ESP32_PIN                 1u  //!< TXD_ESP32 pin is GPIO1
#define RXD_ESP32_PIN                 3u  //!< RXD_ESP32 pin is GPIO3

// IR receiver pins
#define IR_RECEIVER_PIN              35u  //!< IR_RECEIVER pin is GPIO35

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern xSemaphoreHandle I2CMutex;

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

#endif // BOARD_H_
