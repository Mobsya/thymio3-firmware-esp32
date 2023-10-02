//_____________________________________________________________________________
//
// Copyright (C) 2023                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    attentive.c
//! \brief   This module provides the useful functions to use the attentive mode
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

#include "attentive.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------
#define SOUND_TURN_SPEED 80
#define SOUND_FRONT_SPEED 80
#define SOUND_STOP 0
#define SOUND_RUN 1
#define SOUND_TURNRIGHT 2

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------
//static const char* Tag = "attentive";
static char time = 0;
static char clap = 0;
static char direction = SOUND_STOP;
static uint8_t skip_clap_counter = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Attentive_Init(void)
{

}

//_____________________________________________________________________________

void Attentive_Start(void)
{
  direction = SOUND_STOP;
  clap = 0;
  time = 0;
  skip_clap_counter = 50;
}

//_____________________________________________________________________________

void Attentive_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Attentive_Run(void) {
	static char claptime;
	uint8_t brightness = Common_GetBodyColorPulse();
	
	if(skip_clap_counter > 0) { // Avoid confusing buttons sound with clap when entering attentive mode.
		skip_clap_counter--;	// The behaviors task is called @ 50 Hz, thus wait 1 second.
		Leds_SetBodyBrightness(0, 0, Common_GetBodyColorPulse());
		CLEAR_EVENT(EVENT_MIC);
		return;
	}

	if(IS_EVENT(EVENT_MIC)) {
		//ESP_LOGD(Tag, "clap event");
		CLEAR_EVENT(EVENT_MIC);
		if(clap == 0) {
			time = 0;
			clap = 1;
			Leds_SetCircleBrightness(MAX_BRIGHTNESS,0,0,0,0,0,0,0);
			//ESP_LOGD(Tag, "1st clap");
		} else if(clap == 1 && 2 < time && time < 30) { // Behaviors run @ 50 Hz, thus between 40 and 600 ms
			clap = 2;
			claptime = time + 1;
			Leds_SetCircleBrightness(MAX_BRIGHTNESS,MAX_BRIGHTNESS,0,0,0,0,0,MAX_BRIGHTNESS);
			//ESP_LOGD(Tag, "2nd clap");
		} else if(clap == 2 && claptime < time && time < 40) { // Behaviors run @ 50 Hz, thus at most 800 ms
			clap = 3;
			Leds_SetCircleBrightness(MAX_BRIGHTNESS,MAX_BRIGHTNESS,MAX_BRIGHTNESS,0,0,0,MAX_BRIGHTNESS,MAX_BRIGHTNESS);
			//ESP_LOGD(Tag, "3rd clap");
		}
	}
	
	if(time < 100) // Behaviors run @ 50 Hz, thus each "time" increment corresponds to 20 ms
		time++;
		
	if(time > 50) { // Behaviors run @ 50 Hz, thus after 1 second reset clap state
		clap = 0;
		Leds_SetCircleBrightness(0,0,0,0,0,0,0,0);
		//ESP_LOGD(Tag, "reset clap");
	}	
	
  // If one clap detected and 100 ms passed then handle "one clap actions"
	if((clap == 1) && (time == 5)) {		
		if(direction == SOUND_RUN) {
			//ESP_LOGD(Tag, "1 clap actions: SOUND_TURNRIGHT");
			direction = SOUND_TURNRIGHT;
			Common_SetTargetSpeed(SOUND_TURN_SPEED, -SOUND_TURN_SPEED);
		} else if(direction == SOUND_TURNRIGHT) {
			//ESP_LOGD(Tag, "1 clap actions: SOUND_RUN");
			direction = SOUND_RUN;
			Common_SetTargetSpeed(SOUND_FRONT_SPEED, SOUND_FRONT_SPEED);
		}
	}

  // If two claps detected and 600 ms passed then handle "two claps actions"
	if((clap == 2) && (time == 30)) {		
		if(direction == SOUND_STOP) {
			//ESP_LOGD(Tag, "2 claps actions: SOUND_RUN");
			direction = SOUND_RUN;
			Common_SetTargetSpeed(SOUND_FRONT_SPEED, SOUND_FRONT_SPEED);
		} else {
			//ESP_LOGD(Tag, "2 claps actions: SOUND_STOP");
			direction = SOUND_STOP;
			Common_SetTargetSpeed(0, 0);
		}
	}
	
  // If three claps detected and 800 ms passed then handle "three claps actions"
	if((clap == 3) && (time == 40)) {
		//ESP_LOGD(Tag, "3 claps actions");
		direction = SOUND_RUN;
		Common_SetTargetSpeed(SOUND_TURN_SPEED, 0);
	}	
	
	if(Common_HandleTableEdgeDetection(0u, 0u, brightness) == 0) { // No table edge detected
		Leds_SetBodyBrightness(0, 0, Common_GetBodyColorPulse()); 
	}
		
}

//_____________________________________________________________________________
