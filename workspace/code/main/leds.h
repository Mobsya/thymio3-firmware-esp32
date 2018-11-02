//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    leds.h
//! \brief   This module provides the useful functions to use the LEDs
//!
//! \author  Vincent Gonet
//!
//! \version $Id: leds.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef LEDS_H_
#define LEDS_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MIN_BRIGHTNESS             0u
#define MAX_BRIGHTNESS            32u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details The name of the LEDs
enum
{
  // LEDs connected to U17
  E_Led_IR_Back_Left,    // U17.QA --> D25
  E_Led_IR_Back_Right,   // U17.QB --> D26
  E_Led_R_Top,           // U17.QC --> D27 and D38
  E_Led_G_Top,           // U17.QD --> D27 and D38
  E_Led_B_Top,           // U17.QE --> D27 and D38
  E_Led_Battery_0,       // U17.QF --> D28
  E_Led_Battery_1,       // U17.QG --> D29
  E_Led_Battery_2,       // U17.GH --> D30

  // LEDs connected to U13
  E_Led_R_Bottom_Left,   // U13.QA --> D2
  E_Led_G_Bottom_Left,   // U13.QB --> D2
  E_Led_B_Bottom_Left,   // U13.QC --> D2
  E_Led_R_Bottom_Right,  // U13.QD --> D6
  E_Led_G_Bottom_Right,  // U13.QE --> D6
  E_Led_B_Bottom_Right,  // U13.QF --> D6
  E_Led_SD_Card,         // U13.QG --> D35
  E_Led_White_Sensor,    // U13.QH --> D39

  // LEDs connected to U15
  E_Led_Front_IR_0,      // U15.QA -->
  E_Led_Front_IR_1,      // U15.QB -->
  E_Led_Front_IR_3,      // U15.QC -->
  E_Led_Front_IR_4,      // U15.QD -->
  E_Led_Front_IR_2A,     // U15.QE -->
  E_Led_Front_IR_2B,     // U15.QF -->
  E_Led_Ground_IR_0,     // U15.QG --> Not connected
  E_Led_Ground_IR_1,     // U15.QH --> Not connected

  // LEDs connected to U16
  E_Led_Circle_0,        // U16.QA -->
  E_Led_Circle_1,        // U16.QB -->
  E_Led_Circle_2,        // U16.QC -->
  E_Led_Circle_3,        // U16.QD -->
  E_Led_Circle_4,        // U16.QE -->
  E_Led_RC,              // U16.QF -->
  E_Led_Sound,           // U16.QG -->
  E_Led_Circle_7,        // U16.QH -->

  // LEDs connected to U14
  E_Led_Button_0,        // U14.QA -->
  E_Led_Button_2,        // U14.QB -->
  E_Led_Button_3,        // U14.QC -->
  E_Led_Button_1,        // U14.QD -->
  E_Led_Circle_5,        // U14.QE -->
  E_Led_Circle_6,        // U14.QF -->
  E_Led_Temp_Red,        // U14.QG -->
  E_Led_Temp_Blue        // U14.QH -->
};
typedef uint8_t T_Led;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the LEDs
//! \pre       None
//! \param     None
//! \return    None
extern void Leds_Init(void);

//! \brief     Run the LEDs task
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_RunTask(void);

//! \brief     Set the brightness of a single LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_SetSingleBrightness(T_Led led, uint8_t brightness);

//! \brief     Set the brightness of each circle LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_SetCircleBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t l5, uint8_t l6,
                                     uint8_t l7);

//! \brief     Set the brightness of each proximity IR LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void leds_SetProxIRBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3, uint8_t l4, uint8_t l5, uint8_t l6,
                                     uint8_t l7);

//! \brief     Set the brightness of the top RGB LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_SetTopBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the bottom left RGB LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
void Leds_SetBottomLeftBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the bottom right RGB LED
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
void Leds_SetBottomRightBrightness(uint8_t red, uint8_t green, uint8_t blue);

#endif // LEDS_H_
