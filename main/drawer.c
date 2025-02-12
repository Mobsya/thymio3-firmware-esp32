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
#define DRAW_FLOWER 0
#define DRAW_STAR 1
#define STOP_DELAY 300 // ms
#define DELAY_AFTER_ROTATION 10 // 200 ms (based on 50 Hz behaviors update rate)
#define PROX_THRESHOLD 1500
#define DEFAULT_FW_MOTION_DURATION 6000000 // Timer ticks for forward duration

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_State_Idle,
  E_State_Drawing,
} T_State;

//! \details States of the play state machine for flower
typedef enum
{
  E_DrawFlowerState_Init,
  E_DrawFlowerState_DrawPetalFw,
  E_DrawFlowerState_WaitDrawPetalFw,
  E_DrawFlowerState_DrawPetalBw,
  E_DrawFlowerState_WaitDrawPetalBw,
  E_DrawFlowerState_WaitStepRot,
} T_DrawFlowerState;

//! \details States of the play state machine for star
typedef enum
{
  E_DrawStarState_Init,
  E_DrawStarState_DrawFw,
  E_DrawStarState_WaitDrawFw,
  E_DrawStarState_WaitFwRot
} T_DrawStarState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *Tag = "drawer"; //!< Log tag

static T_State State = E_State_Idle;             //!< State of the main state machine
static uint8_t StepsIndex = 2;
static int16_t StepStartAngle = 0;
static int16_t StepDeltaAngle = 0;
static uint16_t StepsCounter = 0;
static T_DrawFlowerState DrawState = E_DrawFlowerState_Init; //!< State of the draw state machine
static uint8_t drawSelection = DRAW_FLOWER;
static int16_t DegreesPerStepFlower[16] = {0, 0, 120, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t StepsPerRotationFlower[16] = {0, 0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t DegreesPerStepStar[16] = {0, 0, 0, 0, 180-36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t StepsPerStar[16] = {0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 3, 4, 5, 0, 0, 0};
static int16_t DegreesPerStepPolygon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 120, 90, 72, 60, 0, 0};
static int16_t StepsPerPolygon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 0, 0};
//static bool useGyroCalib = true;
static uint8_t StartFromProxState = 0;
static uint8_t StartFromProxCount = 0;
static uint64_t motionDurations[2]; // Timer ticks to travels forward and backward

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Play the sequence to draw a flower
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void DrawFlower(void);

//! \brief     Play the sequence to draw a star
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void DrawStar(void);

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
      if((GetProximityValue(1) > (PROX_THRESHOLD+200)) && (GetProximityValue(2) > (PROX_THRESHOLD+200)) && (GetProximityValue(3) > (PROX_THRESHOLD+200))) // Hand detected
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
      if((GetProximityValue(1) < (PROX_THRESHOLD-200)) && (GetProximityValue(2) < (PROX_THRESHOLD-200)) && (GetProximityValue(3) < (PROX_THRESHOLD-200))) // Hand removed
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
  StepsIndex = 10;
  drawSelection = DRAW_STAR;
  Accelerometer_ClearTapStatus(); // Clear any tap made before entering this mode
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
  StepsIndex = 10;
  drawSelection = DRAW_STAR;
  Accelerometer_ClearTapStatus(); // Clear any tap made before entering this mode
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
      if(drawSelection == DRAW_FLOWER)
      {
        if(StepsIndex == 2)
        {
          StepsIndex = 4;
        } else {
          StepsIndex--;
        }
      }       
      else
      {
        if(StepsIndex == 10)
        {
          StepsIndex = 13;
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
      if(drawSelection == DRAW_FLOWER) 
      {
        if(StepsIndex == 4)
        {
          StepsIndex = 2;
        } else {
          StepsIndex++;
        }
      }
      else
      {
        if(StepsIndex == 13)
        {
          StepsIndex = 10;
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
        drawSelection = DRAW_STAR;
        StepsIndex = 10;
      }
      else
      {
        drawSelection = DRAW_FLOWER;
        StepsIndex = 2;
      }
    }
    when(buttonState[E_Button_Backward])
    {
      if(StepsIndex < 8)
      {
        drawSelection = DRAW_STAR;
        StepsIndex = 10;
      }
      else
      {
        drawSelection = DRAW_FLOWER;
        StepsIndex = 2;
      }
    }    
    when(Accelerometer_IsTapDetected() || StartFromProx())
    {     
      State = E_State_Drawing;
      if(drawSelection == DRAW_FLOWER) 
      {
        DrawState = E_DrawFlowerState_Init;
      } 
      else 
      {
        DrawState = E_DrawStarState_Init;
      }
    }
    break;

  case E_State_Drawing:
    if(drawSelection == DRAW_FLOWER) 
    {
      if(StepsIndex==4) {
        DrawStar();
      } else {
        DrawFlower();
      }
    } 
    else
    {
      DrawPolygon(); 
    }
    when(Accelerometer_IsTapDetected())
    {
      State = E_State_Idle;
      Common_SetTargetSpeed(0, 0);
      AngleController_Stop();
    }
    break;

  default:
    // Do nothing
    break;
  }
  
}

//_____________________________________________________________________________
// Draw flower by using forward and backward motion.
// Draw only once with predefined size.
static void DrawFlower(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawFlowerState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees 
    StepsCounter = 0;
    DrawState = E_DrawFlowerState_DrawPetalFw;
    break;

  case E_DrawFlowerState_DrawPetalFw:
    StepStartAngle = Gyroscope_GetAngleZ_deg();
    Common_SetTargetSpeed(MOVEMENT_SPEED+MOVEMENT_SPEED_DELTA, MOVEMENT_SPEED-MOVEMENT_SPEED_DELTA);
    Behavior_SetMotionInProgress(true);
    TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[0]);
    TimerHw_Reset_Counter(1, 1);  
    TimerHw_Start(1, 1);
    DrawState = E_DrawFlowerState_WaitDrawPetalFw;
    break;

  case E_DrawFlowerState_WaitDrawPetalFw:
    if(!Behavior_IsMotionInProgress())
    {
      StepDeltaAngle = Gyroscope_GetAngleZ_deg() - StepStartAngle;
      if(StepDeltaAngle > 180)
      {
        StepDeltaAngle -= 360;
      }
      AngleController_Start(-StepDeltaAngle, MAX_ROTATION_SPEED);
      delayMotorStopped = 0;
      DrawState = E_DrawFlowerState_DrawPetalBw;
    }
    break;

  case E_DrawFlowerState_DrawPetalBw:
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION)
      {
        StepStartAngle = Gyroscope_GetAngleZ_deg();
        Common_SetTargetSpeed(-MOVEMENT_SPEED+MOVEMENT_SPEED_DELTA, -MOVEMENT_SPEED-MOVEMENT_SPEED_DELTA);
        Behavior_SetMotionInProgress(true);
        TimerHw_Set_Alarm_Ticks(1, 1, motionDurations[1]);
        TimerHw_Reset_Counter(1, 1);  
        TimerHw_Start(1, 1);
        DrawState = E_DrawFlowerState_WaitDrawPetalBw;
      }
    }
    break;

  case E_DrawFlowerState_WaitDrawPetalBw:
    if(!Behavior_IsMotionInProgress()) 
    {  
      StepDeltaAngle = Gyroscope_GetAngleZ_deg() - StepStartAngle;
      if(StepDeltaAngle > 180)
      {
        StepDeltaAngle -= 360;
      }      
      AngleController_Start(-(DegreesPerStepFlower[StepsIndex]+StepDeltaAngle), MAX_ROTATION_SPEED);
      delayMotorStopped = 0;
      DrawState = E_DrawFlowerState_WaitStepRot;
    }
    break;

  case E_DrawFlowerState_WaitStepRot:
    if(AngleController_Completed()) 
    {    
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION)
      {
        StepsCounter++;
        if(StepsCounter == StepsPerRotationFlower[StepsIndex])
        {
          StepsCounter = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases  
          State = E_State_Idle;
          DrawState = E_DrawFlowerState_Init;
          break;
        }
        DrawState = E_DrawFlowerState_DrawPetalFw;
      }
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Draw star only once.
// Use only forward motion.
static void DrawStar(void)
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
      AngleController_Start(DegreesPerStepStar[StepsIndex], MAX_ROTATION_SPEED2);
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
        if(StepsCounter == StepsPerStar[StepsIndex])
        {
          StepsCounter = 0;
          State = E_State_Idle;
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

// Draw polygon only once.
// Use only forward motion.
static void DrawPolygon(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepsCounter = 0;
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
      AngleController_Start(DegreesPerStepPolygon[StepsIndex], MAX_ROTATION_SPEED2);
      delayMotorStopped = 0;
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      delayMotorStopped++;
      if(delayMotorStopped == DELAY_AFTER_ROTATION) {
        StepsCounter++;
        if(StepsCounter == StepsPerPolygon[StepsIndex])
        {
          StepsCounter = 0;
          State = E_State_Idle;
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


