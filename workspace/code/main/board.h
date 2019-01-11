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

#define MAX_NUMBER_PIN                 40u

#define VA_ENABLE_PIN                  27u
#define GPIO0_PIN                       0u

// Audio pins
#define MICROPHONE_PIN                 36u
#define SOUND_OUT_PIN                  25u

// IR ground pins
#define IR_PULSE_GROUND_LEFT_PIN       33u
#define IR_PULSE_GROUND_RIGHT_PIN      32u
#define IR_SENSE_GROUND_LEFT_PIN       34u
#define IR_SENSE_GROUND_RIGHT_PIN      39u

// IR back pins
#define IR_PULSE_BACK_PIN              26u
#define IR_SENSE_BACK_LEFT_PIN         23u
#define IR_SENSE_BACK_RIGHT_PIN        35u

// IR front pins
#define IR_PULSE_FRONT_PIN             19u
#define IR_SENSE_FRONT_1_PIN           18u
#define IR_SENSE_FRONT_2_PIN            5u
#define IR_SENSE_FRONT_3_PIN           17u
#define IR_SENSE_FRONT_4_PIN           16u
#define IR_SENSE_FRONT_5_PIN            4u

// LEDs pins
#define LED_CLK_PIN                    14u
#define LED_SDI_PIN                    13u
#define LED_CS_PIN                     15u

#define IR_RECEIVER_PIN                12u

#define ACC_INT_PIN                     2u

// I2C pins
#define SDA_PIN                        21u
#define SCL_PIN                        22u

// UART pins
#define TXD_ESP32_PIN                   1u
#define RXD_ESP32_PIN                   3u

#define IR_SENSE_PIN_SEL               ((uint64_t)1u << (uint64_t)IR_SENSE_BACK_RIGHT_PIN)

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
