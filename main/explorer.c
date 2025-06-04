//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    explorer.c
//! \brief   This module provides the useful functions to use the explorer mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "explorer.h"

#include "common.h"
#include "leds.h"
#include "stm32_spi.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define INITIAL_SPEED          150
#define MAX_SPEED              500
#define MIN_SPEED            (-300)

#define SPEED_INCREMENT         50

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const char* Tag = "explorer";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunCircleLedRotation(void);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Explorer_Init(void)
{

}

//_____________________________________________________________________________

void Explorer_Start(void)
{

}

//_____________________________________________________________________________

void Explorer_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
}

//_____________________________________________________________________________

void Explorer_Run(void)
{
  static int16_t speed = INITIAL_SPEED;
	static long temp1 = 0;
	static long temp2 = 0;
  static int16_t prox[7];
  static int16_t speed_l = 0;
  static int16_t speed_r = 0;
  static uint8_t still_counter = 0;
  static uint8_t escape_action_counter = 0;

  uint8_t brightness = Common_GetBodyColorPulse();

  RunCircleLedRotation();

  // Buttons management
  Common_SetSpeedUsingButtons(&speed, SPEED_INCREMENT, MAX_SPEED, MIN_SPEED);

  /*
  if (speed >= 0)
  {
    Common_HandlePositiveSpeed(speed);
  }
  else
  {
    Common_HandleNegativeSpeed(speed);
  }
  */

  GetProximityValues(prox);

	if(speed >= 0) {
    temp1 = 0;
    temp2 = 0;
		temp1 += prox[0]>>2;
		temp1 += prox[1] * 2;
		temp1 += prox[2] * 3;
		temp1 += prox[3] * 2;
		temp1 += prox[4]>>2;
		temp2 -= prox[0];
		temp2 += prox[1] * -3;
		temp2 += prox[3] * 3;
		temp2 += prox[4];
		speed_l = speed - ((temp1 + temp2) * speed)/2000;
		speed_r = speed - ((temp1 - temp2) * speed)/2000;
		
		if(speed_l < -600) 
			speed_l = -600;
		if(speed_r < -600)
			speed_r = -600;
		if(speed_l > 600)
			speed_l = 600;
		if(speed_r > 600)
			speed_r = 600;
      
    if(Common_HandleTableEdgeDetection(brightness, brightness, 0u) == 0)
    {
      // Escape action activated, rotate right for a while.
      if(escape_action_counter > 0) {
        escape_action_counter--;
        speed_l = 200;
        speed_r = -200;
      }      
      Common_SetTargetSpeed(speed_l, speed_r);
    }      
			
	} else {
		long temp = prox[6] * speed;
		speed_l = speed + (temp/(-300));
		
		temp = prox[5] * speed;
		speed_r = speed  + (temp/(-300));
 
    if(Common_HandleTableEdgeDetection(brightness, brightness, 0u) == 0)
    {
      // Escape action activated, rotate right for a while.
      if(escape_action_counter > 0) {
        escape_action_counter--;
        speed_l = 200;
        speed_r = -200;
      }          
      Common_SetTargetSpeed(speed_l, speed_r);
    }       
    
	}

  // Check when the robot is somehow still
  if((abs(speed_l) < 40) && (abs(speed_r) < 40)) {
    still_counter++;
    if(still_counter >= 100) { // This is called at 50 hz (from behaviors), thus it means after 2 seconds
      escape_action_counter = 120; // If after 2 seconds that the robot is somehow blocked (reached a corner?) then try an escape motion => turn right for about 1.5 sec
    }
  } else {
    still_counter = 0;
  }

}

//_____________________________________________________________________________

static void RunCircleLedRotation(void)
{
  static uint8_t led_state = 0u;
  uint8_t l[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint8_t fixed;

  led_state += 2u;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7u] = MAX_BRIGHTNESS;
  l[(fixed - 1u) & 0x7u] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1u)));
  l[(fixed + 1u) & 0x7u] = (led_state & (MAX_BRIGHTNESS - 1u));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}
