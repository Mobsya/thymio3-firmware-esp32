//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
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
//! \version $Id: board.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef BOARD_H_
#define BOARD_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MAX_NUMBER_PIN                 40u  //!< Maximum number of pins

#define VA_ENABLE_PIN                  27u  //!< VA_ENABLE pin is GPIO27
#define GPIO0_PIN                       0u  //!< GPIO0 pin is GPIO0

// Audio pins
#define MICROPHONE_PIN                 36u  //!< MICROPHONE pin is GPIO36
#define SOUND_OUT_PIN                  25u  //!< SOUND_OUT pin is GPIO25

// IR ground pins
#define IR_PULSE_GROUND_LEFT_PIN       33u  //!< IR_PULSE_GROUND_LEFT pin is GPIO33
#define IR_PULSE_GROUND_RIGHT_PIN      32u  //!< IR_PULSE_GROUND_RIGHT pin is GPIO32
#define IR_SENSE_GROUND_LEFT_PIN       34u  //!< IR_SENSE_GROUND_LEFT pin is GPIO34
#define IR_SENSE_GROUND_RIGHT_PIN      39u  //!< IR_SENSE_GROUND_RIGHT pin is GPIO39

// IR back pins
#define IR_PULSE_BACK_PIN              26u  //!< IR_PULSE_BACK pin is GPIO26
#define IR_SENSE_BACK_LEFT_PIN         23u  //!< IR_SENSE_BACK_LEFT pin is GPIO23
#define IR_SENSE_BACK_RIGHT_PIN        35u  //!< IR_SENSE_BACK_RIGHT pin is GPIO35

// IR front pins
#define IR_PULSE_FRONT_PIN             19u  //!< IR_PULSE_FRONT pin is GPIO19
#define IR_SENSE_FRONT_1_PIN           18u  //!< IR_SENSE_FRONT_1 pin is GPIO18
#define IR_SENSE_FRONT_2_PIN            5u  //!< IR_SENSE_FRONT_2 pin is GPIO5
#define IR_SENSE_FRONT_3_PIN           17u  //!< IR_SENSE_FRONT_3 pin is GPIO17
#define IR_SENSE_FRONT_4_PIN           16u  //!< IR_SENSE_FRONT_4 pin is GPIO16
#define IR_SENSE_FRONT_5_PIN            4u  //!< IR_SENSE_FRONT_5 pin is GPIO4

// LEDs pins
#define LED_CLK_PIN                    14u  //!< LED_CLK pin is GPIO14
#define LED_SDI_PIN                    13u  //!< LED_SDI pin is GPIO13
#define LED_CS_PIN                     15u  //!< LED_CS pin is GPIO15

#define IR_RECEIVER_PIN                12u  //!< IR_RECEIVER pin is GPIO12

#define ACC_INT_PIN                     2u  //!< ACC_INT pin is GPIO2

// I2C pins
#define SDA_PIN                        21u  //!< SDA pin is GPIO21
#define SCL_PIN                        22u  //!< SCL pin is GPIO22

// UART pins
#define TXD_ESP32_PIN                   1u  //!< TXD_ESP32 pin is GPIO1
#define RXD_ESP32_PIN                   3u  //!< RXD_ESP32 pin is GPIO3

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

#endif // BOARD_H_
