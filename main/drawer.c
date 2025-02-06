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

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MOVEMENT_SPEED 140 //!< Movement speed forward
//#define MOVEMENT_SPEED_BW 200 //!< Movement speed backward
#define MOVEMENT_SPEED_DELTA 80 //!< Movement speed difference
#define MAX_ROTATION_SPEED 140 //!< Maximum rotation speed allowed
#define TICK_INC_STEP 25 // Based on 50 Hz behaviors update rate
#define INITIAL_MOTION_DURATION 300000 //!< Duration of a movement in us
#define MOTION_DURATION_INC_STEP 300000 // Increment of a movement duration in us
#define STAR_INITIAL_MOTION_DURATION 37 //!< Duration of a movement for star drawing (based on 50 Hz behaviors update rate)
#define STAR_MOTION_DURATION_INC_STEP 12 // Increment of a movement duration for star drawing (based on 50 Hz behaviors update rate)
#define STAR_MOTION_DURATION_INC_FACTOR 1.1 // Factor increment of a movement duration for star drawing
#define MOTION_FW_TO_BW_FACTOR 1.04 // To compensate for the difference between forward and backward actual speeds
#define MAX_ROTATION_SPEED2 100 //!< Maximum rotation speed allowed
#define DRAW_FLOWER 0
#define DRAW_STAR 1
#define STOP_DELAY 300 // ms
#define DELAY_AFTER_ROTATION 10 // 200 ms (based on 50 Hz behaviors update rate)
#define PROX_THRESHOLD 1500

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
  E_DrawStarState_WaitFwRot,
  E_DrawStarState_WaitDrawBw,
  E_DrawStarState_WaitBwRot,
} T_DrawStarState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *Tag = "drawer"; //!< Log tag

static T_State State = E_State_Idle;             //!< State of the main state machine
static int16_t DegreesPerStep[16] = {180, 120, 90, 72, 60, 52, 45, 40, 36, 33, 30, 27, 24, 21, 18, 15};
static int16_t StepsPerRotation[16] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 17, 20, 24};
static int16_t DegreesPerStepStar[16] = {0, 0, 0, 0, 0, 0, 0, 0, 45, 38, 31, 26, 22, 18, 14, 10};
//static int16_t StepsPerRotationStar[16] = {0, 0, 0, 0, 0, 0, 0, 0, 8, 10, 12, 14, 16, 20, 26, 36};
static int16_t StepsPerRotationStar[16] = {0, 0, 0, 0, 0, 0, 0, 0, 24, 30, 36, 42, 48, 60, 78, 108};
static int16_t DegreesPerStepStar2[16] = {0, 0, 0, 0, 0, 0, 0, 0, 90, 72, 45, 30, 20, 15, 10, 6};
static int16_t DegreesPerStepStar3[16] = {0, 0, 0, 0, 0, 0, 0, 0, 180, 180-90, 180-72, 180-45, 180-30, 180-20, 180-15, 180-10};
static uint8_t StepsIndex = 2;
static int16_t StepStartAngle = 0;
static int16_t StepDeltaAngle = 0;
static uint16_t StepsCounter = 0;
static uint16_t StepTick = 0; // Based on behaviors update rate of 50 Hz
static uint16_t StepTickMax = 0; // The duration of the motion when going forward (based on behaviors update rate of 50 Hz)
static uint16_t StepTickMaxBw = 0; // The duration of the motion when going backward (based on behaviors update rate of 50 Hz)
static uint64_t motionDuration = INITIAL_MOTION_DURATION; // Given in us (initial duration is 1 second)
static T_DrawFlowerState DrawState = E_DrawFlowerState_Init; //!< State of the draw state machine
static bool motionInProgress = false;
static uint8_t drawSelection = DRAW_FLOWER;
static int16_t DegreesPerStepFlower3[16] = {0, 0, 120, 90, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t StepsPerRotationFlower3[16] = {0, 0, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t DegreesPerStepStar5[16] = {0, 0, 0, 0, 180-36, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static int16_t StepsPerStar5[16] = {0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 3, 4, 5, 0, 0, 0};
static int16_t DegreesPerStepPolygon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 120, 90, 72, 60, 0, 0};
static int16_t StepsPerPolygon[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 4, 5, 6, 0, 0};
static bool useGyroCalib = true;
static uint8_t StartFromProxState = 0;
static uint8_t StartFromProxCount = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Play the sequence to draw a flower
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void DrawFlower(void);
static void DrawFlower2(void);
static void DrawFlower3(void);

//! \brief     Play the sequence to draw a star
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void DrawStar(void);
static void DrawStar2(void);
static void DrawStar3(void);
static void DrawStar4(void);
static void DrawStar5(void);

static void DrawPolygon(void);

//! \brief     Interrupt called at the end of a movement
//! \pre       First initialize the mode
//! \param     arg - Not used
//! \return    None
static void IRAM_ATTR ISR_EndOfMotion(void *arg);

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
  //TimerHw_Init(1, 0, true, INITIAL_MOTION_DURATION, ISR_EndOfMotion);
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
      //DrawFlower();
      //DrawFlower2();
      if(StepsIndex==4) {
        DrawStar5();
      } else {
        DrawFlower3();
      }
    } 
    else
    {
      //DrawStar();
      //DrawStar2();
      //DrawStar3();
      //DrawStar4();
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
static void DrawFlower(void)
{
  switch (DrawState)
  {
  case E_DrawFlowerState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    //motionDuration = INITIAL_MOTION_DURATION;
    //TimerHw_Set_Alarm(1, 0, motionDuration);    
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawFlowerState_DrawPetalFw;
    break;
    //motionInProgress = true;
    StepTick = 0;    
    DrawState = E_DrawFlowerState_WaitDrawPetalFw;
    break;

  case E_DrawFlowerState_WaitDrawPetalFw:
    StepTick++;
    if(StepTick == StepTickMax) 
    //if(motionInProgress == false)
    {
      Common_SetTargetSpeed(0, 0);
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      StepDeltaAngle = Gyroscope_GetAngleZ_deg() - StepStartAngle;
      if(StepDeltaAngle > 180)
      {
        StepDeltaAngle -= 360;
      }
      AngleController_Start(-StepDeltaAngle, MAX_ROTATION_SPEED);
      DrawState = E_DrawFlowerState_DrawPetalBw;
    }
    break;

  case E_DrawFlowerState_DrawPetalBw:
    if(AngleController_Completed()) 
    {      
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);
      StepStartAngle = Gyroscope_GetAngleZ_deg();
      Common_SetTargetSpeed(-MOVEMENT_SPEED+MOVEMENT_SPEED_DELTA, -MOVEMENT_SPEED-MOVEMENT_SPEED_DELTA);
      //TimerHw_Start(1, 0);
      //motionInProgress = true;      
      StepTick = 0;
      DrawState = E_DrawFlowerState_WaitDrawPetalBw;
    }
    break;

  case E_DrawFlowerState_WaitDrawPetalBw:
    StepTick++;
    if(StepTick == (StepTickMaxBw))
    //if(motionInProgress == false) 
    {
      Common_SetTargetSpeed(0, 0);
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      StepDeltaAngle = Gyroscope_GetAngleZ_deg() - StepStartAngle;
      if(StepDeltaAngle > 180)
      {
        StepDeltaAngle -= 360;
      }      
      AngleController_Start(-(DegreesPerStep[StepsIndex]+StepDeltaAngle), MAX_ROTATION_SPEED);
      DrawState = E_DrawFlowerState_WaitStepRot;
    }
    break;

  case E_DrawFlowerState_WaitStepRot:
    if(AngleController_Completed()) 
    {    
      //Common_SetTargetSpeed(0, 0); // Needed?
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS); 
      StepsCounter++;
      if(StepsCounter == StepsPerRotation[StepsIndex])
      {
        StepsCounter = 0;
        StepTickMax += TICK_INC_STEP;
        StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
        //motionDuration += MOTION_DURATION_INC_STEP;
        //TimerHw_Set_Alarm(1, 0, motionDuration);
        Codec_Stop();
        Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases  
      }
      StepTick = 0;
      DrawState = E_DrawFlowerState_DrawPetalFw;
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________
// Try drawing a flower by using only forward motion.
static void DrawFlower2(void)
{
  switch (DrawState)
  {
  case E_DrawFlowerState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    //motionDuration = INITIAL_MOTION_DURATION;
    //TimerHw_Set_Alarm(1, 0, motionDuration);    
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = 1;
    break;

  case 1:    
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    //TimerHw_Start(1, 0);
    //motionInProgress = true;
    StepTick = 0;    
    DrawState = 2;
    break;

  case 2:
    StepTick++;
    if(StepTick == StepTickMax) 
    {
      StepTick = 0;
      Common_SetTargetSpeed(MOVEMENT_SPEED, 0);  
      DrawState = 3;
    }
    break;

  case 3:
    StepTick++;
    if(StepTick == 500)
    {
      StepTick = 0;
      Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED); 
      DrawState = 4;
    }
    break;

  case 4:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      StepTick = 0;
      Common_SetTargetSpeed(MOVEMENT_SPEED, 0);
      DrawState = 5;
    }
    break;

  case 5:
    StepTick++;
    if(StepTick == 100)
    {
      DrawState = 1;
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
static void DrawFlower3(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawFlowerState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    //motionDuration = INITIAL_MOTION_DURATION;
    //TimerHw_Set_Alarm(1, 0, motionDuration);    
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawFlowerState_DrawPetalFw;
    break;

  case E_DrawFlowerState_DrawPetalFw:
    StepStartAngle = Gyroscope_GetAngleZ_deg();
    Common_SetTargetSpeed(MOVEMENT_SPEED+MOVEMENT_SPEED_DELTA, MOVEMENT_SPEED-MOVEMENT_SPEED_DELTA);
    //TimerHw_Start(1, 0);
    //motionInProgress = true;
    StepTick = 0;    
    DrawState = E_DrawFlowerState_WaitDrawPetalFw;
    break;

  case E_DrawFlowerState_WaitDrawPetalFw:
    StepTick++;
    if(StepTick == StepTickMax) 
    //if(motionInProgress == false)
    {
      Common_SetTargetSpeed(0, 0);
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
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
        //TimerHw_Start(1, 0);
        //motionInProgress = true;      
        StepTick = 0;
        DrawState = E_DrawFlowerState_WaitDrawPetalBw;
      }
    }
    break;

  case E_DrawFlowerState_WaitDrawPetalBw:
    StepTick++;
    if(StepTick == (StepTickMaxBw))
    //if(motionInProgress == false) 
    {
      Common_SetTargetSpeed(0, 0);
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      StepDeltaAngle = Gyroscope_GetAngleZ_deg() - StepStartAngle;
      if(StepDeltaAngle > 180)
      {
        StepDeltaAngle -= 360;
      }      
      AngleController_Start(-(DegreesPerStepFlower3[StepsIndex]+StepDeltaAngle), MAX_ROTATION_SPEED);
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
        if(StepsCounter == StepsPerRotationFlower3[StepsIndex])
        {
          StepsCounter = 0;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases  
          State = E_State_Idle;
          DrawState = E_DrawFlowerState_Init;
          break;
        }
        StepTick = 0;
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

// Increment motion distance after each complete revolution (360 degrees).
// Alternate forward and backward motion.
static void DrawStar(void)
{
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepTickMax = STAR_INITIAL_MOTION_DURATION; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
      AngleController_Start(DegreesPerStepStar[StepsIndex], MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    //Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);    
    //StepTick = 0;
    //DrawState = E_DrawStarState_WaitDrawBw;
    //break;
    if(AngleController_Completed()) 
    {
      StepsCounter++;
      if(StepsCounter == StepsPerRotationStar[StepsIndex])
      {
        StepsCounter = 0;
        StepTickMax += STAR_MOTION_DURATION_INC_STEP;
        StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      }    
      Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);    
      StepTick = 0;
      DrawState = E_DrawStarState_WaitDrawBw;
    }
    break;

  case E_DrawStarState_WaitDrawBw:
    StepTick++;
    if(StepTick == StepTickMaxBw)
    {
      Common_SetTargetSpeed(0, 0); 
      AngleController_Start(DegreesPerStepStar[StepsIndex], MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitBwRot;
    }
    break;

  case E_DrawStarState_WaitBwRot:
    //DrawState = E_DrawStarState_DrawFw;
    //break;
    if(AngleController_Completed()) 
    {
      StepsCounter++;
      if(StepsCounter == StepsPerRotationStar[StepsIndex])
      {
        StepsCounter = 0;
        StepTickMax += STAR_MOTION_DURATION_INC_STEP;
        StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      }
      StepTick = 0;
      DrawState = E_DrawStarState_DrawFw;
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Fixed motion distance and 45 degrees angle.
// Altrnate forward and backward motion.
static void DrawStar2(void)
{
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
      AngleController_Start(45, MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      //StepsCounter++;
      if(StepsCounter == StepsPerRotationStar[StepsIndex])
      {
        StepsCounter = 0;
        StepTickMax += STAR_MOTION_DURATION_INC_STEP;
        StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      }    
      Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);    
      StepTick = 0;
      DrawState = E_DrawStarState_WaitDrawBw;
    }
    break;

  case E_DrawStarState_WaitDrawBw:
    StepTick++;
    if(StepTick == StepTickMaxBw)
    {
      Common_SetTargetSpeed(0, 0); 
      AngleController_Start(45, MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitBwRot;
    }
    break;

  case E_DrawStarState_WaitBwRot:
    if(AngleController_Completed()) 
    {
      //StepsCounter++;
      if(StepsCounter == StepsPerRotationStar[StepsIndex])
      {
        StepsCounter = 0;
        StepTickMax += STAR_MOTION_DURATION_INC_STEP;
        StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      }
      StepTick = 0;
      DrawState = E_DrawStarState_DrawFw;
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Increment motion distance after each step.
// Alternate forward and backward motion.
static void DrawStar3(void)
{
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepTickMax = 30; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++; 
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      AngleController_Start(DegreesPerStepStar2[StepsIndex], MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      //StepTickMax *= STAR_MOTION_DURATION_INC_FACTOR;
      //StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      Common_SetTargetSpeed(-MOVEMENT_SPEED, -MOVEMENT_SPEED);    
      StepTick = 0;
      DrawState = E_DrawStarState_WaitDrawBw;
    }
    break;

  case E_DrawStarState_WaitDrawBw:
    StepTick++;
    if(StepTick == StepTickMaxBw)
    {
      Common_SetTargetSpeed(0, 0); 
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      AngleController_Start(DegreesPerStepStar2[StepsIndex], MAX_ROTATION_SPEED);
      DrawState = E_DrawStarState_WaitBwRot;
    }
    break;

  case E_DrawStarState_WaitBwRot:
    if(AngleController_Completed()) 
    {
      vTaskDelay(STOP_DELAY/portTICK_PERIOD_MS);   
      StepTickMax *= STAR_MOTION_DURATION_INC_FACTOR;
      StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
      StepTick = 0;
      DrawState = E_DrawStarState_DrawFw;
      Codec_Stop();
      Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases   
    }
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

// Increment motion distance after each step.
// Use only forward motion.
static void DrawStar4(void)
{
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepTickMax = 10; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
      AngleController_Start(DegreesPerStepStar3[StepsIndex], MAX_ROTATION_SPEED2);
      DrawState = E_DrawStarState_WaitFwRot;
    }
    break;

  case E_DrawStarState_WaitFwRot:
    if(AngleController_Completed()) 
    {
      StepTickMax *= STAR_MOTION_DURATION_INC_FACTOR;
      Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);    
      StepTick = 0;
      DrawState = E_DrawStarState_WaitDrawBw;
      Codec_Stop();
      Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases  
    }
    break;

  case E_DrawStarState_WaitDrawBw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0); 
      AngleController_Start(DegreesPerStepStar3[StepsIndex], MAX_ROTATION_SPEED2);
      DrawState = E_DrawStarState_WaitBwRot;
    }
    break;

  case E_DrawStarState_WaitBwRot:
    if(AngleController_Completed()) 
    {
      StepTickMax *= STAR_MOTION_DURATION_INC_FACTOR;
      StepTick = 0;
      DrawState = E_DrawStarState_DrawFw;
      Codec_Stop();
      Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion duration increases    
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
static void DrawStar5(void)
{
  static uint8_t delayMotorStopped = 0;
  switch (DrawState)
  {
  case E_DrawStarState_Init:
    Gyroscope_ResetAngle(); // Start from 0 degrees
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
      AngleController_Start(DegreesPerStepStar5[StepsIndex], MAX_ROTATION_SPEED2);
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
        if(StepsCounter == StepsPerStar5[StepsIndex])
        {
          StepsCounter = 0;
          State = E_State_Idle;
          DrawState = E_DrawStarState_Init;
          Codec_Stop();
          Codec_PlayOnboardSound(TONE_TYPE_BEEP); // Emit sound when the motion ends       
          break;
        }           
        StepTick = 0;
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
    StepTickMax = 50; // Initially set to 1 second
    StepTickMaxBw = StepTickMax*MOTION_FW_TO_BW_FACTOR;
    StepsCounter = 0;
    DrawState = E_DrawStarState_DrawFw;
    break;

  case E_DrawStarState_DrawFw:
    Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    StepTick = 0;    
    DrawState = E_DrawStarState_WaitDrawFw;
    break;

  case E_DrawStarState_WaitDrawFw:
    StepTick++;
    if(StepTick == StepTickMax)
    {
      Common_SetTargetSpeed(0, 0);
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
        StepTick = 0;
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

//_____________________________________________________________________________

static void IRAM_ATTR ISR_EndOfMotion(void *arg)
{
  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[0].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if (intr_status & BIT(0))
  {
    Common_SetTargetSpeed(0, 0);
    TIMERG1.int_clr_timers.t0 = 1;
    TimerHw_Stop(1, 0);
    motionInProgress = false;
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
}
