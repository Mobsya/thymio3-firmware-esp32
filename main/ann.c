//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ann.c
//! \brief   This module provides the useful functions to use the artificial neural network mode.
//!           The weights will be updated when obstacles are detected.              
//!
//! \author  Stefano Morgani, based on the aseba script developed by Francesco Mondada
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <string.h>
#include "esp_log.h"

#include "ann.h"
#include "buttons.h"
#include "accelerometer.h"
#include "aseba_esp32.h"
#include "codec.h"
#include "common.h"
#include "leds.h"
#include "stm32_spi.h"
#include "settings.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define LEARNING_STEP 3
#define HALF_LEARNING_STEP 1.5
#define COLLISION_SENSOR_VALUE 3300 // threshold for collision

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "ANN";

// state machine
uint8_t state = 0;	// 0: ANN forward, 1: back, 2: learning, 3: end 4: freeze, 5: pause
uint8_t prev_state = 0;
uint16_t timer = 0; // for behaviors timing (goes back)
uint16_t forget_timer = 0; // used to have a foretting timing
uint16_t white_timer = 0; // used to have a variation of color from red to white
uint8_t i = 0;
uint8_t j = 0;

// Neural net variables
float x[8] = {0, 0, 0, 0, 0, 0, 0, 0};
float y[2] = {0, 0};
float w_left[8] = {0, 0, 0, 0, 0, 0, 0, 2000};   // weights, Bias only is constant
float w_right[8] = {0, 0, 0, 0, 0, 0, 0, 2000};

// Collision memory
uint8_t collision[7] = {0};

// White color tracker
uint8_t white_level = 0;

int16_t prox_horizontal[7] = {0, 0, 0, 0, 0, 0, 0};
int16_t prox_ground_delta[2];

uint8_t period = 0;
uint8_t *btn_status;

// Scale factors
float sensor_scale = 20;
float motor_scale  = 30;

int16_t motor_left_target = 0;
int16_t motor_right_target = 0;

uint16_t learning_times = 0;

uint8_t btn_left_right_released = 1;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------
void prox(void)
{
  if(state == 0)
  {
    // check if collision
    i = 0;
    while(i < 5) // front proximity sensors
    {
      if(prox_horizontal[i] > COLLISION_SENSOR_VALUE)
      {
        // on est dans un cas de collision
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_NOTIFY);

        state = 1;
        timer = 0;
        motor_left_target = -200;
        motor_right_target= -200;
        Common_SetTargetSpeed(motor_left_target, motor_right_target);
        Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0, 0); // Red
        white_level = 0; // Reset white

        // Store collision data
        j = 0;
        for(j=0; j<=6; j++)
        {
          if(prox_horizontal[j] > COLLISION_SENSOR_VALUE)
          { 
            if(j==2)
            {
              if(prox_horizontal[1] > prox_horizontal[3])
              {
                collision[j] = 1;
              }
              else
              {
                collision[j] = 2;
              }
            }
            else
            {
              collision[j] = 1;
            }
          }
        }
      }
      i += 1;
    }
    // i = 5;
    // while(i < 7) // back proximity sensors
    // {
    //   if(prox_horizontal[i] > COLLISION_SENSOR_VALUE)
    //   {
    //     // on est dans un cas de collision
    //     Codec_Stop();
    //     Codec_PlayOnboardSound(TONE_TYPE_NOTIFY);

    //     state = 1;
    //     timer = 0;
    //     motor_left_target = 200;
    //     motor_right_target= 200;
    //     Common_SetTargetSpeed(motor_left_target, motor_right_target);
    //     Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0, 0); // Red
    //     white_level = 0; // Reset white

    //     // Store collision data
    //     j = 0;
    //     for(j=0; j<=6; j++)
    //     {
    //       if(prox_horizontal[j] > COLLISION_SENSOR_VALUE)
    //       { 
    //         collision[j] = 1;
    //       }
    //     }
    //   }
    //   i += 1;
    // }       
  }
}


void ANN_Init(void)
{

}

//_____________________________________________________________________________

void ANN_Start(void)
{
  state = 0;
  prev_state = 0;
  timer = 0;
  forget_timer = 0;
  white_level = MAX_BRIGHTNESS;
  Leds_SetBodyBrightness(MAX_BRIGHTNESS, white_level, white_level);
  motor_right_target = 0;
  motor_left_target = 0;
  learning_times = 0;
  btn_left_right_released = 1;

  // Reset all weights except bias
  i = 0;
  for(i=0; i<=6; i++)
  {
      w_left[i] = 0;
      w_right[i] = 0;
  }

  // Display level of wheights
  Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
}

//_____________________________________________________________________________

void ANN_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
  Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
  Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
  Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
}

//_____________________________________________________________________________

void ANN_Run(void)
{
  uint8_t brightness = Common_GetBodyColorPulse();
  GetProximityValues(prox_horizontal);
  prox();

  if(state == 0) {
    Leds_SetBodyBrightness(brightness, brightness, brightness);
  }

  period++;
  if(period == 5) // 100 ms => based on behaviors task running frequency of 50 hz
  {
    period = 0;

    //calcul du réseau
    timer += 100;
    forget_timer += 100;
    white_timer += 100;

    if((state == 0) || (state == 4))
    {
      i = 0;
      for(i=0; i<=6; i++)
      {
        x[i] = prox_horizontal[i] / sensor_scale;
      }
      x[7] = 1;  // Bias

      y[0] = 0;
      y[1] = 0;
      for(i=0; i<=7; i++)
      {
        y[0] += x[i] * w_left[i];
        y[1] += x[i] * w_right[i];
      }

      motor_left_target = y[0] / motor_scale;
      motor_right_target = y[1] / motor_scale;
      Common_SetTargetSpeed(motor_left_target, motor_right_target);

      // Gradually increase green for yellow if no collision
      if((state !=4) && (state != 5))
      {
        if((white_level < MAX_BRIGHTNESS) && (white_timer > 1000))
        {
          white_level += 1;
          white_timer = 0;
          //Leds_SetBodyBrightness(MAX_BRIGHTNESS, white_level, white_level);       
        }
      }
    }

    if((state == 1) && (timer >= 1000))
    {
      state = 2;
      timer = 0;
      motor_left_target = 0;
      motor_right_target = 0;
      Common_SetTargetSpeed(motor_left_target, motor_right_target);
      Leds_SetBodyBrightness(0, MAX_BRIGHTNESS, 0); // Green = learning
      
      Codec_Stop();
      Codec_PlayOnboardSound(TONE_TYPE_BEEP);

      if(learning_times < 16) {
        learning_times += 1;
      }
      Leds_SetLegoProgress(learning_times);
      
      // Learning: reinforce away from obstacle
      i = 0;
      for(i=0; i<=6; i++)
      {
        if(collision[i] >= 1)
        {
          if(i <= 1) // Left + front-left senors
          {
            w_left[i] += LEARNING_STEP;
            w_right[i] += -LEARNING_STEP; //-HALF_LEARNING_STEP;
          }
          else if((i >= 3) && (i <= 4)) // Right + front-right sensors
          {
            w_right[i] += LEARNING_STEP;
            w_left[i] += -LEARNING_STEP; //-HALF_LEARNING_STEP;
          }
          else if(i == 2) // Front sensor
          {
            if(collision[i] == 1) // stronger on left side, prefer to turn right
            {
              w_left[i] += 0; //-HALF_LEARNING_STEP + 1;
              w_right[i] += -HALF_LEARNING_STEP;
            }
            else
            {
              w_left[i] += -HALF_LEARNING_STEP;
              w_right[i] += 0; //-HALF_LEARNING_STEP + 1;
            }
          }
          /*
          else if(i == 5) // Back-right sensor
          {
            w_left[i] += -HALF_LEARNING_STEP;
            w_right[i] += LEARNING_STEP;
          }
          else if(i == 6) // Back-left sensor
          {
            w_left[i] += LEARNING_STEP;
            w_right[i] += -HALF_LEARNING_STEP;
          }
          */
        }
      }
      //erase collision
      for(i=0; i<=6; i++)
      {
          collision[i] = 0;
      }
      
      // Display level of wheights
      Leds_SetCircleBrightness(abs(w_right[2])+abs(w_left[2]), abs(w_right[3])+abs(w_right[4]), abs(w_left[3])+abs(w_left[4]), abs(w_right[5])+abs(w_left[5]), 0, abs(w_right[6])+abs(w_left[6]), abs(w_left[0])+abs(w_left[1]), abs(w_right[0])+abs(w_right[1]));
    }
    else if((state == 2) && (timer >= 300)) // end of green
    {
      state = 0;
      motor_left_target = 0;
      motor_right_target = 0;
      Common_SetTargetSpeed(motor_left_target, motor_right_target);
      Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0, 0); // Red (end of learning)
    }

    // Forgetting mechanism every 16s
    if((forget_timer >= 16000) && (state != 4) && (state != 5))
    {
      forget_timer = 0;
      i = 0;
      for(i=0; i<=6; i++)
      {
        if(w_left[i] > 0)
        {
            w_left[i] -= 1;
        }
        else if(w_left[i] < 0)
        {
            w_left[i] += 1;
        }
        if(w_right[i] > 0)
        {
            w_right[i] -= 1;
        }
        else if(w_right[i] < 0)
        {
            w_right[i] += 1;
        }
      }
      if(learning_times > 0) {
        learning_times -= 1;
      }
      // Display level of wheights
      Leds_SetCircleBrightness(abs(w_right[2])+abs(w_left[2]), abs(w_right[3])+abs(w_right[4]), abs(w_left[3])+abs(w_left[4]), abs(w_right[5])+abs(w_left[5]), 0, abs(w_right[6])+abs(w_left[6]), abs(w_left[0])+abs(w_left[1]), abs(w_right[0])+abs(w_right[1]));
      Leds_SetLegoProgress(learning_times);
    }   
    
    if(state == 5)
    {
      forget_timer = 0;
      motor_left_target = 0;
      motor_right_target = 0;
      Common_SetTargetSpeed(motor_left_target, motor_right_target);      
    }
  }


  btn_status = Buttons_GetStatus();
  if (btn_status[E_Button_Backward]) // pause
  {
   	// on passe en mode pause
		if(state != 5)
    {
      prev_state = state;
      state = 5;
    }
  }

  if (btn_status[E_Button_Forward]) // resume
  {
   	// on passe en mode learning
   	if(state == 5) // is paused
    {
      if(prev_state == 4) // was running
      {
        state = 4; // back to running
        Leds_SetBodyBrightness(MAX_BRIGHTNESS/2, MAX_BRIGHTNESS/2, 0);
      }
      else
      {
        state = 0; // back to learning
        Leds_SetBodyBrightness(0, 0, 0);
      }
      prev_state = state;
    }
  }  

  if((btn_status[E_Button_Left] || btn_status[E_Button_Right]) && (btn_left_right_released==1)) // running or learning
  {
    // on passe en mode freeze
  	if(prev_state == 4)
    {
  			state = 0;
  			Leds_SetBodyBrightness(0, 0, 0);
    }
    else
    {
  			state = 4;
  			Leds_SetBodyBrightness(MAX_BRIGHTNESS/2, MAX_BRIGHTNESS/2, 0);
    }
  	prev_state = state;
  	btn_left_right_released = 0;
  }
  else if((btn_status[E_Button_Left]==0) && (btn_status[E_Button_Right]==0))
  {
  	btn_left_right_released = 1;
  }

}
