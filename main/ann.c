//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ann.c
//! \brief   This module provides the useful functions to use the artificial neural network mode
//!           To train the robot to act put him on a black ground or in air and teach him what to do when the robot detect
//!           - or doesn't detect- something. By pressing one of the arrow buttons you assign an action to this state. by putting the robot on white ground, he will react to the environment accordingly to what you tought him. 
//!           If you're not happy about the training, you can put it back on the dark ground and modify or continue its training
//!           By pressing the center button, you can reset the robot to its initial state 
//!
//! \author  Stefano Morgani, based on the python script developed by Valentina Ferraioli
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

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MODE_PLAY 1

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
int16_t prox_horizontal[7];
int16_t prox_ground_delta[2];

float w_l[8] = {0,0,0,0,0,0,0, 0};
float w_r[8] = {0,0,0,0,0,0,0, 0};

float x[8] = {0,0,0,0,0,0,0, 0};
float y[2] = {0,0};

bool feedback_received = false;

uint8_t leds_circle[8] = {0,0,0,0,0,0,0, 0};
uint8_t leds_top[3] = {0,0,0};

// Scale factors for sensors, outputs and motors
float sensor_scale = 90;
float output_scale  = 20;
float motor_scale  = 15;

// training_state = tt -> Training
// training_state = ff -> Test 
bool training_state = false;

int16_t motor_left_target = 0;
int16_t motor_right_target = 0;

static uint8_t btn_status_prev[5] = {0};
static uint8_t btn_status[5] = {0};

// Learning rate
float alpha = 1.0;
    
// Speed increment for each learning episode
int16_t speed = 10;

// The desired output for each button: +/-speed
float y_left = 0;
float y_right = 0;

uint8_t running_mode = MODE_PLAY;

// Initialize variables
// [0] = blue, [1] = yellow
uint8_t red[3] = {0,14,0};
uint8_t green[3] = {0,9,0}; 
uint8_t blue[3] = {20,0,0}; 

bool check_long_press = false;
uint16_t pressed_counter = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

// stop the robot movement
void stop(void)
{ 
    motor_left_target = 0;
    motor_right_target = 0;
    Common_SetTargetSpeed(0, 0);
}

// activate the robot motors based on ANN outputs.
void act(void)
{
  uint8_t i = 0;
    
  // Compute dot product of inputs and weights
  y[0] = 0;
  y[1] = 0;
        
  for(i=0; i<8; i++)   
  {
    y[0] = y[0] + x[i] * w_l[i];
    y[1] = y[1] + x[i] * w_r[i];
  }
    
  motor_left_target = y[0] / motor_scale;
  motor_right_target = y[1] / motor_scale;    // True because action has been performed 
  Common_SetTargetSpeed(motor_left_target, motor_right_target);
}

// Update weights based on feedback.
void change_weights()
{
  uint8_t i = 0;

  for(i=0; i<8; i++)
  {
    w_l[i]  = w_l[i]  + (alpha*y_left*x[i]) / output_scale;
    w_r[i] = w_r[i] + (alpha*y_right*x[i]) / output_scale;
  }
  ESP_LOGI(Tag, "w_l = %f,%f,%f,%f,%f,%f,%f,%f", w_l[0], w_l[1], w_l[2], w_l[3], w_l[4], w_l[5], w_l[6], w_l[7]);
  ESP_LOGI(Tag, "w_r = %f,%f,%f,%f,%f,%f,%f,%f", w_r[0], w_r[1], w_r[2], w_r[3], w_r[4], w_r[5], w_r[6], w_r[7]);
  feedback_received = true;
}

bool long_press_detection(uint8_t button)
{
  if(button == 1)
  {
    check_long_press = true;
    pressed_counter = 0;
  }
  if(button == 0)
  {
    check_long_press = false;
  }
  ESP_LOGI(Tag, "timer_diff = %d", pressed_counter);
  return (pressed_counter > 50); // 1 seconds
}

void button_backward_evt(uint8_t button_backward)
{
  if(training_state)
  {
    bool long_press_detection_state = long_press_detection(button_backward);
    if(button_backward == 0)
    {
      if(!long_press_detection_state)
      {
        y_left  = -speed;
        y_right = -speed;
        change_weights();
      }
      else
      {
        y_left  = speed;
        y_right = speed;
        change_weights();
      }
    }
  }
}

void button_left_evt(uint8_t button_left)
{
  if(training_state)
  {
    bool long_press_detection_state = long_press_detection(button_left);
    ESP_LOGI(Tag, "long_press_detection %d", long_press_detection_state);
    if(button_left == 0)
    {
      if(!long_press_detection_state)  // positive reinforcement
      {
        y_left  = -speed;
        y_right = speed;
        change_weights();
      }
      else                         //negative reinforcement
      {
        y_left  = speed;
        y_right = -speed;
        change_weights();
      }
    }
  }
}

void button_right_evt(uint8_t button_right)
{
  if(training_state)
  {
    bool long_press_detection_state = long_press_detection(button_right);
    if(button_right == 0)
    {
      if(!long_press_detection_state)  //positive reinforcement
      {
        y_left  = speed;
        y_right = -speed;
        change_weights();
      }
      else                         //negative reinforcement
      {
        y_left  = -speed;
        y_right = speed;
        change_weights();
      }
    }
  }
}

void button_forward_evt(uint8_t button_forward)
{
  if(training_state)
  {
    bool long_press_detection_state = long_press_detection(button_forward);
    if(button_forward == 0)
    {
      if(long_press_detection_state == 0) //positive reinforcement
      {
        y_left  = speed;
        y_right = speed;
        change_weights();
      }
      else                         //negative reinforcement
      {
        y_left  = -speed;
        y_right = -speed;
        change_weights();
      }
    }
  }
}

// Process sensor inputs and compute ANN outputs.
void perceive(float sensor_scale, float motor_scale)
{    
    bool no_obstacle = true;
    uint8_t i = 0;

    for(i=0; i<7; i++)
    {
      //Get and scale sensor input if something is detected
      x[i] = prox_horizontal[i] / sensor_scale;
      if(x[i] != 0) // If any sensor detects an obstacle, set no_obstacle to False
      {
        no_obstacle = false;
      }
    }

    if(no_obstacle)  //set x[8] to 100 if no obstacles are detected 0 otherwise (default)
    {
        x[7] = 100;
    }
    else
    {
        x[7] = 0;
    }
        
    // Compute dot product of inputs and weights
    y[0] = 0;
    y[1] = 0;

    for(i=0; i<8; i++)
    {
      y[0] = y[0] + x[i] * w_l[i];
      y[1] = y[1] + x[i] * w_r[i];
    }
}

void prox(void)
{
  int8_t color = - 1;
  int16_t ground[2] = {0, 950};
  int16_t ground_min = 0;
  int8_t color_search = 0;
  
  if(running_mode == MODE_PLAY)
  {
    ground_min = 1000;
    // detecting the ground color. If black (or no ground detected) I'm in a training environment, testing otherwise
    for(color_search=1; color_search<3; color_search++)
    {                                        
      if(abs(prox_ground_delta[0] - ground[color_search - 1]) < abs(ground_min))
      {
        ground_min = prox_ground_delta[0] - ground[color_search - 1];
        leds_top[0] = red[color_search - 1];
        leds_top[1] = green[color_search - 1];
        leds_top[2] = blue[color_search - 1];
        color = color_search - 1;
      }
    }       
    if((color == 0) && !training_state)   // this way I'm checking if I am in the on a blue ground and in the not in training_state, 
                                          // if so I enable the training_state (training_state = True) - according with my background
    {
      training_state = true;
      feedback_received = false;
    }   
    else if((color == 1) && training_state)  // here I'm on a testing environment - yellow - and disable the training_state
    {
      training_state = false;
      feedback_received = false;
    }
  }
}

void visualize_weights(void)
{
    bool prox_detected = false;
    uint8_t i = 0;

    for(i=0; i<7; i++)
    {
      if(prox_horizontal[i] > 2500)
      {
        prox_detected = true;
        if(w_l[i] <= -120)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, MAX_BRIGHTNESS);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(6);
          }
        }
        else if(w_l[i] <= -80)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, MAX_BRIGHTNESS);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(7);
          }
        }
        else if(w_l[i] <= -40)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, MAX_BRIGHTNESS);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(8);
          }
        }   
        else if(w_l[i] <= -5)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, MAX_BRIGHTNESS);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(9);
          }
        }  
        else if((w_l[i] > -5) && (w_l[i] < 5))
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, 0);
        }  
        else if(w_l[i] <= 40)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, 0);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(10);
          }          
        }  
        else if(w_l[i] <= 80)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, 0);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(11);
          }
        }
        else if(w_l[i] <= 120)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, 0);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(12);
          }
        }
        else
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_0, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_1, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_2, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_3, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_0, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_1, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_2, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_3, 0);
          if(feedback_received)
          {
            Codec_Stop();
            Codec_PlayOnboardSound(12);
          }          
        }


        if(w_r[i] <= -120)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, MAX_BRIGHTNESS);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(6);
          //}
        }
        else if(w_r[i] <= -80)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(7);
          //}
        }    
        else if(w_r[i] <= -40)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(8);
          //}
        }     
        else if(w_r[i] <= -5)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(9);
          //}
        }        
        else if((w_r[i] > -5) && (w_r[i] < 5))
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
        }  
        else if(w_r[i] <= 40)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(10);
          //}
        }      
        else if(w_r[i] <= 80)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(11);
          //}
        }   
        else if(w_r[i] <= 120)
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);
          //if(feedback_received)
          //{
          //  Codec_Stop();
          //  Codec_PlayOnboardSound(12);
          //}
        }  
        else
        {
          Leds_SetSingleBrightness(E_Led_Lego_Front_4, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_5, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_6, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Front_7, MAX_BRIGHTNESS);
          Leds_SetSingleBrightness(E_Led_Lego_Back_4, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_5, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_6, 0);
          Leds_SetSingleBrightness(E_Led_Lego_Back_7, 0);          
        }                                                                     
      }
    }

    if(prox_detected == false)
    {
      Leds_SetLegoFrontBrightness(0,0,0,0,0,0,0,0);
      Leds_SetLegoBackBrightness(0,0,0,0,0,0,0,0);  
    }     

    Leds_SetCircleBrightness(leds_circle[0], leds_circle[1], leds_circle[2], leds_circle[3], leds_circle[4], leds_circle[5], leds_circle[6], leds_circle[7]);

}

// Calculate LED intensities based on weight values.
void weights_and_lights(void)
{
    float intensity_weights[8] = {0,0,0,0,0,0,0, 0};
    float max_val = w_l[0];
    float min_val = w_l[0];
    float weight_range = 0;
    uint8_t max_intensity = 32;
    uint8_t min_intensity = 0;
    uint8_t i = 0;
    uint8_t order[8] = {6,7,0,1,2,5,3,4}; // mapping of the Thymio's sensors with the respective lights. The sensors counting starts from the front left sensor as 0 to the front right sensor as 4, then the back left sensor as 5 and the right one as 6
                                          // the circle leds instead start fron the front one as 0 and then go clockwise

    // Iterate through the array to find the maximum value
    for(i=0; i<8; i++)
    {
        if(w_l[i] > max_val)
        {
            max_val = w_l[i];
        }
    }

    // Iterate through the array to find the minimum value
    for(i=0; i<8; i++)
    {
        if(w_l[i] < min_val)
        {
            min_val = w_l[i];
        }
    }
            
    weight_range = max_val - min_val;
    if(weight_range < 0)
    {
        weight_range = 1;
    }
        

    //mapping weight to led intensity
    for(i=0; i<8; i++)
    {
        if(w_l[i] == 0) //turn off the sensor 
        {
            intensity_weights[order[i]] = 0;
        }
        else
        { 
            intensity_weights[order[i]] = max_intensity; //((w_l[i] - min_val) * (max_intensity - min_intensity)) / (max_val - min_val) + min_intensity;
        }
    }

    for(i=0; i<8; i++)
    {
      leds_circle[i] = (uint8_t)intensity_weights[i];      
    }
    ESP_LOGD(Tag, "leds_circle = %d,%d,%d,%d,%d,%d,%d,%d", leds_circle[0], leds_circle[1], leds_circle[2], leds_circle[3], leds_circle[4], leds_circle[5], leds_circle[6], leds_circle[7]);
}

void immediate_feedback(void)
{ 
    static uint8_t timer_c = 0;

    if(training_state && !feedback_received)
    {
      //stop and perceive until get feedback
      stop();
      perceive(sensor_scale, motor_scale);
    }
    else if(training_state && feedback_received)
    {
      // then act: move for a few sec and wait for the next feedback
      act();
      timer_c++;
      if(timer_c == 25) // Based on 50 Hz behaviors update frequency
      {
        stop();
        timer_c = 0;
        feedback_received = false;
      }
    }

    if(!training_state)
    {
      perceive(sensor_scale, motor_scale);
      act();
      feedback_received = false;
    }
}

void ANN_Init(void)
{

}

//_____________________________________________________________________________

void ANN_Start(void)
{
  uint8_t i = 0;
  for(i=0; i<8; i++)
  {
    w_l[i] = 0;
    w_r[i] = 0;
  }
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

  GetProximityValues(prox_horizontal);
  GetGroundValues(prox_ground_delta);
  prox();

  visualize_weights();

  Leds_SetFrontLeftBrightness(leds_top[0], leds_top[1], leds_top[2]);
  Leds_SetFrontRightBrightness(leds_top[0], leds_top[1], leds_top[2]);
  Leds_SetBackLeftBrightness(leds_top[0], leds_top[1], leds_top[2]);
  Leds_SetBackRightBrightness(leds_top[0], leds_top[1], leds_top[2]);

  weights_and_lights();

  immediate_feedback();

  memcpy(btn_status, Buttons_GetStatus(), 5);
  if(btn_status[0] != btn_status_prev[0])
  {
    ESP_LOGI(Tag, "button0 event! %d", btn_status[0]);
    button_backward_evt(btn_status[0]);
  }
  if(btn_status[1] != btn_status_prev[1])
  {
    ESP_LOGI(Tag, "button1 event! %d", btn_status[1]);
    button_left_evt(btn_status[1]);
  }
  //if(btn_status[2] != btn_status_prev[2])
  //{
  //  ESP_LOGI(Tag, "button2 event! %d", btn_status[2]);
  //  button_center_evt(btn_status[2]);
  //}
  if(btn_status[3] != btn_status_prev[3])
  {
    ESP_LOGI(Tag, "button3 event! %d", btn_status[3]);
    button_forward_evt(btn_status[3]);
  }
  if(btn_status[4] != btn_status_prev[4])
  {
    ESP_LOGI(Tag, "button4 event! %d", btn_status[4]);
    button_right_evt(btn_status[4]);
  }
  memcpy(btn_status_prev, btn_status, 5);

  if(check_long_press) 
  {
    pressed_counter++; // Based on 50 Hz behaviors update frequency
  }

}
