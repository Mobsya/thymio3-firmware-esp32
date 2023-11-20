//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    obedient.c
//! \brief   This module provides the useful functions to use the obedient mode
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>

#include "esp_log.h"

#include "aseba_esp32.h"
#include "codec.h"
#include "common.h"
#include "leds.h"
#include "rc5.h"
#include "buttons.h"
#include "obedient.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define RC5_SPEED_STEP 150
#define RC5_SPEED_SAT (RC5_SPEED_STEP * 4)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

// static const char* Tag = "obedient";
static int16_t rc5_speed_l;
static int16_t rc5_speed_t; // Rotation

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Obedient_Init(void)
{
}

//_____________________________________________________________________________

void Obedient_Start(void)
{
	rc5_speed_l = 0;
	rc5_speed_t = 0;
}

//_____________________________________________________________________________

void Obedient_Stop(void)
{
	Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Obedient_Run(void)
{

	static int16_t toggle = -1;
	uint8_t *buttonState;
	uint8_t brightness = Common_GetBodyColorPulse();
	Leds_SetBodyBrightness(brightness, 0, brightness);
	int16_t command = RC5_GetCommand(&toggle);	
	buttonState = Buttons_GetStatus();

	when(buttonState[E_Button_Left]) {
		rc5_speed_t = -RC5_SPEED_STEP;
	}

	when(buttonState[E_Button_Right]) {
		rc5_speed_t = RC5_SPEED_STEP;
	}

	when(buttonState[E_Button_Backward]) {
		if (rc5_speed_t)
			rc5_speed_t = 0;
		else
			rc5_speed_l -= RC5_SPEED_STEP;
	}

	when(buttonState[E_Button_Forward]) {
		if (rc5_speed_t)
			rc5_speed_t = 0;
		else
			rc5_speed_l += RC5_SPEED_STEP;
	}

	when(buttonState[E_Button_Forward] && buttonState[E_Button_Backward]) {	
		rc5_speed_l = 0;
	}

	when(buttonState[E_Button_Left] && buttonState[E_Button_Right]) {	
		rc5_speed_t = 0;
	}

	switch (command) {
		case 2:
		case 80:
		case 32:
			if (rc5_speed_t)
				rc5_speed_t = 0;
			else
				rc5_speed_l += RC5_SPEED_STEP;
			break;
		case 4:
		case 85:
		case 17:
		case 77:
			rc5_speed_t = -RC5_SPEED_STEP;
			break;
		case 8:
		case 81:
		case 33:
			if (rc5_speed_t)
				rc5_speed_t = 0;
			else
				rc5_speed_l -= RC5_SPEED_STEP;
			break;
		case 6:
		case 86:
		case 16:
		case 78:
			rc5_speed_t = RC5_SPEED_STEP;
			break;
		case 5:
		case 87:
		case 13:
			rc5_speed_t = 0;
			rc5_speed_l = 0;
			break;
		default:
			break;
	}

	if (rc5_speed_l > RC5_SPEED_SAT)
		rc5_speed_l = RC5_SPEED_SAT;
	if (rc5_speed_l < -RC5_SPEED_SAT)
		rc5_speed_l = -RC5_SPEED_SAT;
	if (rc5_speed_t > RC5_SPEED_SAT)
		rc5_speed_t = RC5_SPEED_SAT;
	if (rc5_speed_t < -RC5_SPEED_SAT)
		rc5_speed_t = -RC5_SPEED_SAT;

}

//_____________________________________________________________________________
