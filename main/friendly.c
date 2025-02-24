//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    friendly.c
//! \brief   This module provides the useful functions to use the friendly mode
//!
//! \author  Vincent Gonet, Stefano Morgani
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
#include "stm32_spi.h"
#include "friendly.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define DETECT                  100 //500
#define SPEED_INCREMENT         50
#define MAX_SPEED              370
#define MIN_SPEED            (-370)
#define TARGET_DIST 2800
#define TARGET_DIST_THR 200

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const char* Tag = "friendly";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Friendly_Init(void)
{

}

//_____________________________________________________________________________

void Friendly_Start(void)
{

}

//_____________________________________________________________________________

void Friendly_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Friendly_Run(void)
{
	static char does_see_friend = 0; // Used when communicating with another robot
  static char sound_done = 0;
  static unsigned char led_state = 0;
	static char led_delta = 1;
  int16_t max = 0;
  int16_t mi = 0;
  int16_t t;
  uint8_t brightness = Common_GetBodyColorPulse();
  int16_t speedDiff;
  int16_t speed_l = 0;
  int16_t prox[7];

  static int16_t speed = 300;

  
  GetProximityValues(prox);
  max = prox[0];
  mi = 0;
  for (uint8_t index = 1u; index < 5u; index++)
  {
    if (prox[index] > max)
    {
      max = prox[index];
      mi = index;
    }
  }

  t = 2 - mi; // Stop rotation when robot is toward the object (=> prox 2 max)
  speedDiff = t * (speed / 2);

  // >3200      => -300 (-speed)
  // 2800..3200 => 0..-200
  // 2300..2800 => 0
  // 2300..1000 => 80..265
  // <1000      => 300 (speed)
/*
  if (max > 2800) // Object near the robot, start going backward slowly
  {
    speed_l = (2800 - max) / 2;
  }

  if (max > 3200) // Object really near to the robot, go backward fast
  {
    speed_l = -speed;
  }

  if (max < 2300) // Object far from the robot, start following it slowly
  {
    t = 265 - (max - 1000) / 7;
    speed_l = t;
  }

  if (max < 1000) // Object really far from the robot, start following it fast
  {
    speed_l = speed;
  }
*/

  if((max > (TARGET_DIST-TARGET_DIST_THR)) && (max < (TARGET_DIST+TARGET_DIST_THR)))
  {
    speed_l = 0;
  } 
  else if(max > (TARGET_DIST+TARGET_DIST_THR)) // Object near the robot, go backward 
  {
    speed_l = (TARGET_DIST - max)>>3;
  } else // Object far from the robot, go forward 
  {
    speed_l = (TARGET_DIST - max)>>1;
  }

  //if(max > (TARGET_DIST+TARGET_DIST_THR)) // Object near the robot, go backward
 // {
    
  //}

/*
	if (max > 3500) 
		speed_l = (3500 - max) / 2;	
	if (max > 4000)
		speed_l = -speed;	
	if (max < 3000) {
		t = 300 - (max - 1000) / 7;
		speed_l = t;
	}	
	if (max < 2000) 
		speed_l = speed;
*/

  if (speed_l > speed)
  {
    speed_l = speed;
  }

  if (speed_l < -speed)
  {
    speed_l = -speed;
  }

  // LEDs management
	if(does_see_friend > 0 && sound_done) {
		//unsigned char rgb[3];

    //GetRainbow(rgb);

    // FIXME Leds_SetTopBrightness(rgb[0], rgb[1], rgb[2]);
    // FIXME Leds_SetBottomLeftBrightness(rgb[2], rgb[0], rgb[1]);
    // FIXME Leds_SetBottomRightBrightness(rgb[1], rgb[2], rgb[0]);
	} else {
    Leds_SetBodyBrightness(0, Common_GetBodyColorPulse(), 0);
  }


	if(does_see_friend) {
		led_state += led_delta;
    if (led_state >= 31)
    {
      led_delta = -1;
    }
    else if (led_state == 0)
    {
      led_delta = 1;
    }
    else
    {
      // Do nothing
    }

    Leds_SetCircleBrightness(0, (led_state >> 4), (led_state >> 3), led_state, MAX_BRIGHTNESS, led_state, (led_state >> 3), (led_state >> 4));
	} else {
		Leds_SetCircleBrightness(0u, 0u, 0u, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, 0u, 0u);
  }


	// Buttons management
	Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);


	// Audio management
  //when(max > DETECT) // When something detected at long distance then play a sound...remove it because add confusion
  //{
  //  Codec_Stop();
  //  Codec_PlayOnboardSound(TONE_TYPE_DETECT);
  //}

	if(speedDiff == 0 && speed_l == 0 && sound_done == 0 && max > DETECT) {
		sound_done = 1;
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_GOOD);
	}
	if(speedDiff != 0 || max < DETECT) {
		sound_done = 0;
  }

	// "Cliff" detection handling
  if(Common_HandleTableEdgeDetection(0u, brightness, 0u) == 0) { // No table edge detected
    if (max < DETECT)
    {
      if (does_see_friend)
      {
        Common_SetTargetSpeed(speed, speed);
      }
      else
      {
        Common_SetTargetSpeed(0, 0);
      }
    }
    else
    {
      Common_SetTargetSpeed((speed_l - speedDiff), (speedDiff + speed_l));
    }
  }

  /*
  if(does_see_friend)
  	does_see_friend--;

  if(IS_EVENT(EVENT_STM32)) { // IS_EVENT(EVENT_DATA)) { // Data coming from IR communication, this is not implemented in Thymio3
  	CLEAR_EVENT(EVENT_STM32);
  	does_see_friend = 0;
  	mi = 0;
  	max = vmVariables.intensity[0];
  	vmVariables.intensity[0] = 0;
  	for(int i = 1; i < 7; i++) {
  		if(vmVariables.intensity[i] > max) {
  			mi = i;
  			max = vmVariables.intensity[i];
  		}
  		vmVariables.intensity[i] = 0;
  	}
  	if(max > 3000) {
  		vmVariables.ir_tx_data = mi;
  		if(vmVariables.rx_data > 0 && vmVariables.rx_data < 4) {
  			when(mi == 2) {
  				//play_sound(SOUND_F_OK); //FIXME
  			}
  			if(mi == 2)
  				does_see_friend = 6;
  		}
  	}
  }
  */

}

//_____________________________________________________________________________
