//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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
//! \license This project is released under the GNU Lesser General Public License
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

#define MAX_BRIGHTNESS            16u  //!< Max brightness applied on the LEDs (duty cycle = 100%)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//  -----       -----       -----       -----       -----       -----
// | U30 | --> | U29 | --> | U28 | --> | U25 | --> | U27 | --> | U26 |
//  -----       -----       -----       -----       -----       -----

//! \details The name of the LEDs
enum
{
  // LEDs connected to U26
  E_Led_Button_Forward,  // U26.QA --> D21
  E_Led_Button_Right,    // U26.QB --> D22
  E_Led_Button_Backward, // U26.QC --> D23
  E_Led_Button_Left,     // U26.QD --> D24
  E_Led_R_Color_Sensor,  // U26.QE --> D25 Red
  E_Led_G_Color_Sensor,  // U26.QF --> D25 Green
  E_Led_B_Color_Sensor,  // U26.QG --> D25 Blue
  E_Led_White_Sensor,    // U26.GH --> D20

  // LEDs connected to U27
  E_Led_Circle_N,        // U27.QA --> D27
  E_Led_Circle_NE,       // U27.QB --> D30
  E_Led_Circle_E,        // U27.QC --> D33
  E_Led_Circle_SE,       // U27.QD --> D36
  E_Led_Circle_S,        // U27.QE --> D39
  E_Led_Circle_SW,       // U27.QF --> D42
  E_Led_Circle_W,        // U27.QG --> D45
  E_Led_Circle_NW,       // U27.QH --> D48

  // LEDs connected to U25
  E_Led_R_Front_Left,    // U25.QA --> D19 Red
  E_Led_G_Front_Left,    // U25.QB --> D19 Green
  E_Led_B_Front_Left,    // U25.QC --> D19 Blue
  E_Led_R_Front_Right,   // U25.QD --> D26 Red
  E_Led_G_Front_Right,   // U25.QE --> D26 Green
  E_Led_B_Front_Right,   // U25.QF --> D26 Blue
  E_Led_RC5,             // U25.QG --> D18
  E_Led_R_Debug,         // U25.QH --> D53 Red

  // LEDs connected to U28
  E_Led_Lego_Front_0,    // U28.QA --> D28
  E_Led_Lego_Front_1,    // U28.QB --> D31
  E_Led_Lego_Front_2,    // U28.QC --> D34
  E_Led_Lego_Front_3,    // U28.QD --> D37
  E_Led_Lego_Front_4,    // U28.QE --> D40
  E_Led_Lego_Front_5,    // U28.QF --> D43
  E_Led_Lego_Front_6,    // U28.QG --> D46
  E_Led_Lego_Front_7,    // U28.QH --> D49

  // LEDs connected to U29
  E_Led_Lego_Back_0,     // U29.QA --> D29
  E_Led_Lego_Back_1,     // U29.QB --> D32
  E_Led_Lego_Back_2,     // U29.QC --> D35
  E_Led_Lego_Back_3,     // U29.QD --> D38
  E_Led_Lego_Back_4,     // U29.QE --> D41
  E_Led_Lego_Back_5,     // U29.QF --> D44
  E_Led_Lego_Back_6,     // U29.QG --> D47
  E_Led_Lego_Back_7,     // U29.QH --> D50

  // LEDs connected to U30 (bank 0)
  E_Led_R_Back_Right,    // U30.QA --> D51 Red
  E_Led_G_Back_Right,    // U30.QB --> D51 Green
  E_Led_B_Back_Right,    // U30.QC --> D51 Blue
  E_Led_R_Back_Left,     // U30.QD --> D52 Red
  E_Led_G_Back_Left,     // U30.QE --> D52 Green
  E_Led_B_Back_Left,     // U30.QF --> D52 Blue
  E_Led_G_Debug,         // U30.QG --> D53 Green
  E_Led_B_Debug          // U30.QH --> D53 Blue
  // D53 Red is driven directly by GPIO17
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

//! \brief     Start the LEDs task
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_Start(void);

//! \brief     Stop the LEDs task
//! \pre       First initialize the LEDs
//! \param     None
//! \return    None
extern void Leds_Stop(void);

//! \brief     Set the brightness of a single LED
//! \pre       First initialize the LEDs
//! \param     led - LED to handle
//! \param     brightness - Brightness
//! \return    None
extern void Leds_SetSingleBrightness(T_Led led, uint8_t brightness);

//! \brief     Set the brightness of each circle LED
//! \pre       First initialize the LEDs
//! \param     l0 to l7 - Brightness of the LEDs associated with the circle
//! \return    None
extern void Leds_SetCircleBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
                                     uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7);

//! \brief     Set the brightness of each buttons LED
//! \pre       First initialize the LEDs
//! \param     forward - Brightness of the LED associated with the forward button
//! \param     right - Brightness of the LED associated with the right button
//! \param     backward - Brightness of the LED associated with the backward button
//! \param     left - Brightness of the LED associated with the left button
//! \return    None
extern void Leds_SetButtonsBrightness(uint8_t forward, uint8_t right, uint8_t backward, uint8_t left);

//! \brief     Set the brightness of each front Lego LED
//! \pre       First initialize the LEDs
//! \param     l0 to l7 - LEDs associated with the front Lego
//! \return    None
extern void Leds_SetLegoFrontBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
                                        uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7);

//! \brief     Set the brightness of each back Lego LED
//! \pre       First initialize the LEDs
//! \param     l0 to l7 - LEDs associated with the back Lego
//! \return    None
extern void Leds_SetLegoBackBrightness(uint8_t l0, uint8_t l1, uint8_t l2, uint8_t l3,
                                       uint8_t l4, uint8_t l5, uint8_t l6, uint8_t l7);

//! \brief     Set the brightness of the color sensor RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetColorSensorBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the front left RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetFrontLeftBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the front right RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetFrontRightBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the back left RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetBackLeftBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the back right RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetBackRightBrightness(uint8_t red, uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the debug RGB LED
//! \pre       First initialize the LEDs
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
extern void Leds_SetDebugBrightness(uint8_t green, uint8_t blue);

//! \brief     Set the brightness of the body RGB LED
//! \pre       First initialize the LEDs
//! \param     red - Red component of the RGB LED
//! \param     green - Green component of the RGB LED
//! \param     blue - Blue component of the RGB LED
//! \return    None
void Leds_SetBodyBrightness(uint8_t red, uint8_t green, uint8_t blue);

#endif // LEDS_H_
