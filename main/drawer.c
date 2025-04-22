//_____________________________________________________________________________
//
// Copyright (C) 2024                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    drawer.c
//! \brief   This module provides the useful functions to use the drawer mode
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "drawer.h"

#include "accelerometer.h"
#include "angle_controller.h"
#include "buttons.h"
#include "codec.h"
#include "common.h"
#include "gyroscope.h"
#include "leds.h"
#include "rc5.h"
#include "stm32_spi.h"
#include "timer_hw.h"
#include "timer_sw.h"
#include "color_sensor.h"
#include "settings.h"
#include "behavior.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MOVEMENT_SPEED 140 //!< Movement speed forward
#define MOVEMENT_SPEED_DELTA 80 //!< Movement speed difference
#define MAX_ROTATION_SPEED 140 //!< Maximum rotation speed allowed
#define MAX_ROTATION_SPEED2 100 //!< Maximum rotation speed allowed
#define DRAW_POLYGONS 0
#define DRAW_SHAPES 1
#define STOP_DELAY 300 // ms
#define DELAY_AFTER_ROTATION 10 // 200 ms (based on 50 Hz behaviors update rate)
#define PROX_THRESHOLD 1500
#define DEFAULT_FW_MOTION_DURATION 6000000 // Timer ticks for forward duration
#define LED_ANIMATION_DELAY 15 // 300 ms (based on 50 Hz behaviors update rate)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_State_Idle,
  E_State_Drawing,
  E_State_LedAnimation,
} T_State;

//! \details States of the play state machine for star
typedef enum
{
  E_DrawStarState_Init,
  E_DrawStarState_DrawFw,
  E_DrawStarState_WaitDrawFw,
  E_DrawStarState_WaitFwRot
} T_DrawStarState;

//! \details States of the play state machine for star
typedef enum
{
  E_DrawPolygonState_Init,
  E_DrawPolygonState_DrawFw,
  E_DrawPolygonState_WaitDrawFw,
  E_DrawPolygonState_WaitFwRot
} T_DrawPolygonState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *Tag = "drawer"; //!< Log tag
static uint8_t ledAnimationState = 0;
static uint8_t ledAnimationCounter = 0;
static T_State State = E_State_Idle;             //!< State of the main state machine
static uint8_t StepsIndex = 0;
static uint16_t StepsCounter = 0;
static uint8_t drawSelection = DRAW_POLYGONS;
uint8_t DrawState = 0;
// Polygons
static T_DrawPolygonState DrawPolygonState = E_DrawPolygonState_Init; //!< State of the draw state machine
static int16_t DegreesPerStepPolygon[16] = {0, 120, 120, 90, 72, 60, 51, 45, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t StepsPerPolygon[16] = {1, 2, 3, 4, 5, 6, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0};
//static bool useGyroCalib = true;
static uint8_t StartFromProxState = 0;
static uint8_t StartFromProxCount = 0;
static uint64_t motionDurations[2]; // Timer ticks to travels forward and backward

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void DrawCircle(void);
static void DrawRectangle(void);
static void DrawDiamond(void);
static void DrawTrapezoid(void);
static void DrawStar5(void);
static void DrawParallelogram(void);
static void DrawStar7(void);
static void Draw8shape(void);
static void DrawPolygon(void);

//! \brief     Change body color based on color detected by color sensor.
//! \param     None
//! \return    None
void HandleBodyColor(T_Color color, uint8_t brightness);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

bool StartFromProx()
{
  switch(StartFromProxState)
  {
    case 0:
      if((GetProximityValue(5) > (PROX_THRESHOLD+200)) && (GetProximityValue(6) > (PROX_THRESHOLD+200))) // Hand detected
      {
        StartFromProxCount++;  // Based on behaviors update rate of 50 hz
        if(StartFromProxCount == 5) // After 100 ms
        {
          StartFromProxState = 1;
        }
      } else {
        StartFromProxCount = 0;
      }
      break;
    
    case 1:
      if((GetProximityValue(5) < (PROX_THRESHOLD-200)) && (GetProximityValue(6) < (PROX_THRESHOLD-200))) // Hand removed
      {
        StartFromProxCount++;  // Based on behaviors update rate of 50 hz
        if(StartFromProxCount == 25) // After 0.5 second
        {
          StartFromProxState = 0;
          StartFromProxCount = 0;
          return true;

        }
      } else {
        StartFromProxCount = 0;
      }
      break;
  }
  return false;
}

void Drawer_Init(void)
{
}

//_____________________________________________________________________________

void Drawer_Start(void)
{
  Leds_SetLegoFrontBrightness(0,0,0,0,0,0,0,0);
  Leds_SetLegoBackBrightness(0,0,0,0,0,0,0,0);
  State = E_State_Idle;
  StepsIndex = 0;
  drawSelection = DRAW_POLYGONS;
  motionDurations[0] = DEFAULT_FW_MOTION_DURATION;
  float fwBwFactor = Settings_GetMotFwBwSettings();
  motionDurations[1] = (float)DEFAULT_FW_MOTION_DURATION*fwBwFactor;
  //printf("mot durations: fw=%lld, bw=%lld (%f)", motionDurations[0], motionDurations[1], fwBwFactor);
  //fflush(stdout);
}

//_____________________________________________________________________________

void Drawer_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
  AngleController_Stop();
  State = E_State_Idle;
  StepsIndex = 0;
  drawSelection = DRAW_POLYGONS;
}

//_____________________________________________________________________________

void Drawer_Run(void)
{
  uint8_t brightness = Common_GetBodyColorPulse();
  uint8_t *buttonState = Buttons_GetStatus();

  HandleBodyColor(ColorSensor_GetColor(), brightness);

  if(StepsIndex < 8) {
    //Leds_SetLegoProgress(StepsIndex+1);
    Leds_SetLegoFrontProgress(StepsIndex+1);
    Leds_SetLegoBackProgress(0);
  } else {
    Leds_SetLegoFrontProgress(0);
    Leds_SetLegoBackProgress(StepsIndex+1-8);
  }

  ESP_LOGE(Tag, "buttonState[E_Button_Forward] = %d", buttonState[E_Button_Forward]);

  switch (State)
  {
  case E_State_Idle:
    when(buttonState[E_Button_Left])
    {
      if(drawSelection == DRAW_POLYGONS)
      {
        if(StepsIndex == 0)
        {
          StepsIndex = 7;
        } else {
          StepsIndex--;
        }
      }       
      else
      {
        if(StepsIndex == 8)
        {
          StepsIndex = 15;
        } else {
          StepsIndex--;
        }
        /*
        // For debugging purposes: activate and deactivate the continuous gyro calibration to see the difference in the square drawing
        if(StepsIndex == 11) {
          if(useGyroCalib)
          {
            useGyroCalib = false;
            Gyroscope_DisableContinuousCalib();
            Gyroscope_ResetCalibration();
            Leds_SetCircleBrightness(0, 0, 0, 0, 0 ,0 ,0, 0);
          } else {
            useGyroCalib = true;
            Gyroscope_EnableContinuousCalib();
            Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS, 0 ,MAX_BRIGHTNESS ,0);            
          }
        } 
        */       
      }      
    }
    when(buttonState[E_Button_Right])
    {
      if(drawSelection == DRAW_POLYGONS) 
      {
        if(StepsIndex == 7)
        {
          StepsIndex = 0;
        } else {
          StepsIndex++;
        }
      }
      else
      {
        if(StepsIndex == 15)
        {
          StepsIndex = 8;
        } else {
          StepsIndex++;
        }
      }
      /*
      // For debugging purposes: activate and deactivate the continuous gyro calibration to see the difference in the square drawing
      if(StepsIndex == 11) {
        if(useGyroCalib)
        {
          useGyroCalib = false;
          Gyroscope_DisableContinuousCalib();
          Gyroscope_ResetCalibration();
          Leds_SetCircleBrightness(0, 0, 0, 0, 0 ,0 ,0, 0);
        } else {
          useGyroCalib = true;
          Gyroscope_EnableContinuousCalib();
          Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS, 0 ,MAX_BRIGHTNESS ,0);            
        }
      } 
      */
    }
    when(buttonState[E_Button_Forward])
    {
      if(StepsIndex < 8)
      {
        drawSelection = DRAW_SHAPES;
        StepsIndex = 8;
      }
      else
      {
        drawSelection = DRAW_POLYGONS;
        StepsIndex = 0;
      }
    }
    when(buttonState[E_Button_Backward])
    {
      if(StepsIndex < 8)
      {
        drawSelection = DRAW_SHAPES;
        StepsIndex = 8;
      }
      else
      {
        drawSelection = DRAW_POLYGONS;
        StepsIndex = 0;
      }
    }    
    when(StartFromProx())
    {     
      State = E_State_Drawing;
      if(drawSelection == DRAW_POLYGONS) 
      {
        DrawPolygonState = E_DrawPolygonState_Init;
      } 
      else 
      {
        DrawState = 0;
      }
    }
    break;

  case E_State_Drawing:
    if(drawSelection == DRAW_POLYGONS) 
    {
      DrawPolygon(); 
    } 
    else
    {
      if(StepsIndex == 8)
      {
        DrawCircle();
      }
      else if(StepsIndex == 9)
      {
        DrawRectangle();
      }
      else if(StepsIndex == 10)
      {
        DrawDiamond();
      }
      else if(StepsIndex == 11)
      {
        DrawTrapezoid();
      }      
      else if(StepsIndex == 12)
      {
        DrawStar5();
      }
      else if(StepsIndex == 13)
      {
        DrawParallelogram();
      }
      else if(StepsIndex == 14)
      {
        DrawStar7();
      }  
      else if(StepsIndex == 15)
      {
        Draw8shape();
      }                
    }
    break;

  case E_State_LedAnimation:
    if(ledAnimationState == 0)
    {
      Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
      ledAnimationCounter = 0;
      ledAnimationState = 1;
    }
    else if(ledAnimationState == 1)
    {
      ledAnimationCounter++;
      if(ledAnimationCounter >= LED_ANIMATION_DELAY)
      {
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
        ledAnimationCounter = 0;
        ledAnimationState = 2;
      }
    }
    else if(ledAnimationState == 2)
    {
      ledAnimationCounter++;
      if(ledAnimationCounter >= LED_ANIMATION_DELAY)
      {
        Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        ledAnimationCounter = 0;
        ledAnimationState = 3;
      }
    }  
    else if(ledAnimationState == 3)
    {
      ledAnimationCounter++;
      if(ledAnimationCounter >= LED_ANIMATION_DELAY)
      {
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
        ledAnimationCounter = 0;
        ledAnimationState = 4;
      }
    }    
    else if(ledAnimationState == 4)
    {
      ledAnimationCounter++;
      if(ledAnimationCounter >= LED_ANIMATION_DELAY)
      {
        Leds_SetCircleBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS, MAX_BRIGHTNESS);
        ledAnimationCounter = 0;
        ledAnimationState = 5;
      }
    }  
    else if(ledAnimationState == 5)
    {
      ledAnimationCounter++;
      if(ledAnimationCounter >= LED_ANIMATION_DELAY)
      {
        Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
        ledAnimationCounter = 0;
        ledAnimationState = 0;
        State = E_State_Idle;
      }
    }       
    break;

  default:
    // Do nothing
    break;
  }
  
}

//_____________________________________________________________________________
// Draw circle only once.
// Use only forward motion.
static void DrawCircle(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    DrawState = 1;
    break;

  case 1:
    Common_SetTargetSpeed(MOVEMENT_SPEED, 0);
    delayMotorStopped = 0;
    DrawState = 2;
    break;

  case 2:
    if(abs(Gyroscope_GetAngleZ_deg()) >= 360)
    {
      Common_SetTargetSpeed(0, 0);
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        ledAnimationCounter = 0;
        ledAnimationState = 0;
        State = E_State_LedAnimation;
        DrawState = 0;
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
        break;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw rectangle only once.
// Use only forward motion.
static void DrawRectangle(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
    DrawState = 1;
    break;

  case 1: // Start forward motion
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    if((StepsCounter==1) || (StepsCounter==3)) // long edge
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]*3/2);
    }
    else // short edge
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    }
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = 2;
    break;

  case 2: // Wait forward motion end, start rotation
    if(!Behavior_IsMotionInProgress())
    {
      AngleController_Start(90, MAX_ROTATION_SPEED2);
      delayMotorStopped = 0;
      DrawState = 3;
    }
    break;

  case 3: // Wait rotation end, restart or stop
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == 4)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = 1;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw diamond only once.
// Use only forward motion.
static void DrawDiamond(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
    DrawState = 1;
    break;

  case 1: // Start forward motion
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = 2;
    break;

  case 2: // Wait forward motion end, start rotation
    if(!Behavior_IsMotionInProgress())
    {
      if((StepsCounter==1) || (StepsCounter==3)) // acute edge
      {
        AngleController_Start(60, MAX_ROTATION_SPEED2);
      }
      else // obtuse angle
      {
        AngleController_Start(120, MAX_ROTATION_SPEED2);
      }
      delayMotorStopped = 0;
      DrawState = 3;
    }
    break;

  case 3: // Wait rotation end, restart or stop
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == 4)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = 1;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw trapezoid only once.
// Use only forward motion.
static void DrawTrapezoid(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
    DrawState = 1;
    break;

  case 1: // Start forward motion
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    if((StepsCounter==3)) // major base
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]*5/2);
    }
    else // others edeges
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    }
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = 2;
    break;

  case 2: // Wait forward motion end, start rotation
    if(!Behavior_IsMotionInProgress())
    {
      if((StepsCounter==0) || (StepsCounter==1))
      {
        AngleController_Start(42, MAX_ROTATION_SPEED2);
      }
      else
      {
        AngleController_Start(138, MAX_ROTATION_SPEED2);
      }
      delayMotorStopped = 0;
      DrawState = 3;
    }
    break;

  case 3: // Wait rotation end, restart or stop
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == 4)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = 1;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Draw 5-pointed star only once.
// Use only forward motion.
static void DrawStar5(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    if(!Behavior_IsMotionInProgress())
    {
      AngleController_Start((180-36), MAX_ROTATION_SPEED2);
      delayMotorStopped = 0;
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION)
      {
        StepsCounter++;
        if(StepsCounter == 5)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = E_DrawStarState_Init;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = E_DrawStarState_DrawFw;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw parallelogram only once.
// Use only forward motion.
static void DrawParallelogram(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
    DrawState = 1;
    break;

  case 1: // Start forward motion
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    if((StepsCounter==1) || (StepsCounter==3)) // long edge
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]*3/2);
    }
    else // short edge
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    }
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = 2;
    break;

  case 2: // Wait forward motion end, start rotation
    if(!Behavior_IsMotionInProgress())
    {
      if((StepsCounter==1) || (StepsCounter==3)) // acute edge
      {
        AngleController_Start(60, MAX_ROTATION_SPEED2);
      }
      else // obtuse angle
      {
        AngleController_Start(120, MAX_ROTATION_SPEED2);
      }
      delayMotorStopped = 0;
      DrawState = 3;
    }
    break;

  case 3: // Wait rotation end, restart or stop
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == 4)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = 1;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Draw 7-pointed star only once.
// Use only forward motion.
static void DrawStar7(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    if(!Behavior_IsMotionInProgress())
    {
      AngleController_Start((180-26), MAX_ROTATION_SPEED2);
      delayMotorStopped = 0;
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION)
      {
        StepsCounter++;
        if(StepsCounter == 7)
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawState = E_DrawStarState_Init;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawState = E_DrawStarState_DrawFw;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw 8 shape only once.
// Use only forward motion.
static void Draw8shape(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case 0:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    DrawState = 1;
    break;

  case 1:
    Common_SetTargetSpeed(MOVEMENT_SPEED, 0);
    delayMotorStopped = 0;
    DrawState = 2;
    break;

  case 2:
    if(abs(Gyroscope_GetAngleZ_deg()) >= 360)
    {
      Common_SetTargetSpeed(0, 0);
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        Gyroscope_ResetAngle(); // Start from 0 degrees
        Common_SetTargetSpeed(0, MOVEMENT_SPEED);
        delayMotorStopped = 0;
        DrawState = 3;     
        break;
      }
    }
    break;

  case 3:
    if(abs(Gyroscope_GetAngleZ_deg()) >= 360)
    {
      Common_SetTargetSpeed(0, 0);
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        ledAnimationCounter = 0;
        ledAnimationState = 0;
        State = E_State_LedAnimation;
        DrawState = 0;
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
        break;
      }
    }
    break;    

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Draw polygon only once.
// Use only forward motion.
static void DrawPolygon(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawPolygonState)
  {
  case E_DrawPolygonState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
    DrawPolygonState = E_DrawPolygonState_DrawFw;
    break;

  case E_DrawPolygonState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    Behavior_SetMotionInProgress(true);
    if(StepsIndex >= 5) // From pentagon onward, reduce the size to be able to draw within an A4 sheet
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]/2);
    }
    else
    {
      TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    }
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawPolygonState = E_DrawPolygonState_WaitDrawFw;
    break;

  case E_DrawPolygonState_WaitDrawFw:
    if(!Behavior_IsMotionInProgress())
    {
      if(DegreesPerStepPolygon[StepsIndex] == 0) // Line special case
      {
        StepsCounter = 0;
        ledAnimationCounter = 0;
        ledAnimationState = 0;
        State = E_State_LedAnimation;
        DrawPolygonState = E_DrawPolygonState_Init;
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
        break;
      }
      AngleController_Start(DegreesPerStepPolygon[StepsIndex], MAX_ROTATION_SPEED2);
      delayMotorStopped = 0;
      DrawPolygonState = E_DrawPolygonState_WaitFwRot;
    }
    break;

  case E_DrawPolygonState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == StepsPerPolygon[StepsIndex])
        {
          StepsCounter = 0;
          ledAnimationCounter = 0;
          ledAnimationState = 0;
          State = E_State_LedAnimation;
          DrawPolygonState = E_DrawPolygonState_Init;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }
        DrawPolygonState = E_DrawPolygonState_DrawFw;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

void HandleBodyColor(T_Color color, uint8_t brightness) {
  if(color == E_Color_Red)
  {
    Leds_SetBodyBrightness(brightness, 0u, 0u);
  }
  else if(color == E_Color_Yellow)
  {
    Leds_SetBodyBrightness(brightness, brightness, 0u);
  }
  else if(color == E_Color_Green)
  {
    Leds_SetBodyBrightness(0u, brightness, 0u);
  }
  else if(color == E_Color_Blue)
  {
    Leds_SetBodyBrightness(0u, 0u, brightness);
  }
  else if(color == E_Color_Purple)
  {
    Leds_SetBodyBrightness(brightness, 0u, brightness);
  }
  else if(color == E_Color_White)
  {
    Leds_SetBodyBrightness(brightness, brightness, brightness);
  }
  else //if(color == E_Color_Unknown)
  {
    Leds_SetBodyBrightness(0u, 0u, 0u);
  }
}


