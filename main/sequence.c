//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sequence.c
//! \brief   This module provides the useful functions to use the sequence mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "sequence.h"

#include "accelerometer.h"
#include "angle_controller.h"
#include "buttons.h"
#include "codec.h"
#include "common.h"
#include "gyroscope.h"
#include "leds.h"
#include "rc5.h"
#include "stm32_spi.h"
#include "timer_sw.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SEQUENCE_BUFFER_SIZE 50u //!< Number of actions stored in the FIFO

// Duration of the movement = 1500000 [us] -> TIMER_SCALE * 1500000 [us] = 7500000 (timer_group)
#define MOVEMENT_DURATION 9000000 //7500000uLL //!< Duration of a movement

#define STOP_DURATION_us 500000u //!< Delay at the end of a movement

#define MOVEMENT_SPEED 300 //!< Movement speed
#define MOVEMENT_SPEED_BW 312 // Backward speed compensated

#define MAX_ROTATION_SPEED 500 //!< Maximum rotation speed allowed

#define COLLISION_THRESHOLD 700   //!< Collision threshold
#define NO_COLLISION_THRESHOLD 10 //!< No collision threshold

#define GROUND_IR_THRESHOLD    280 //130
#define PROX_THRESHOLD 1500

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_State_Record,
  E_State_Play,
  E_State_Erase_All_In_Progress,
  E_State_Erase_Last_Step_In_Progress
} T_State;

//! \details States of the play state machine
typedef enum
{
  E_PlayState_Prepare,
  E_PlayState_Replay,
  E_PlayState_Movement,
  E_PlayState_Rotation,
  E_PlayState_Collision
} T_PlayState;

//! \details States of the play state machine
typedef enum
{
  E_Collision_None,
  E_Collision_Detected,
  E_Collision_Handled
} T_Collision;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *Tag = "sequence"; //!< Log tag

static bool RecordSequenceIsFinished = false; //!< True if the the record is finished
static bool MovementIsInProgress = false;     //!< True if the a movement is in progress
static bool RotationIsInProgress = false;     //!< True if the a rotation is in progress
static bool ObstacleIsDetected = false;       //!< True if an obstacle is detected

static T_TimerSw *StopTimer = NULL; //!< Timer used to add a delay at the end of a movement

static T_State State = E_State_Record;             //!< State of the main state machine
static T_PlayState PlayState = E_PlayState_Replay; //E_PlayState_Prepare; //!< State of the play state machine

static uint8_t Current = 0u; //!< Current value of the sequence table

static uint8_t Sequence[SEQUENCE_BUFFER_SIZE] = {0u}; //!< Sequence table
static uint8_t WrPos = 0u;                            //!< Write position cursor
static uint8_t RdPos = 0u;                            //!< Read position cursor

static uint8_t RotationCompletedDelay = 0;    // Used to make a pause after a rotation of STOP_DURATION_us as done with forward/backward movements.
static uint8_t PlayStatePrepareCount = 0;
static bool PlayStatePrepareToPlay = false;
static uint8_t LegoLedBlinkCount = 0;
static uint8_t StartFromProxState = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Record the sequence
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RecordSequence(void);

//! \brief     Process the erase action
//! \pre       First initialize the mode
//! \param     command - Command of the remote control
//! \return    None
static void ProcessEraseAction(uint8_t command);

//! \brief     Play the sequence
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void PlaySequence(void);

//! \brief     Handle the preparation to replay (red, orange, green)
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandlePrepare(void);

//! \brief     Handle the sequence replay
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandleReplay(void);

//! \brief     Handle the current movement
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandleMovement(void);

//! \brief     Handle the current rotation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandleRotation(void);

//! \brief     Handle the collision
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandleCollision(void);

//! \brief     Check if a collision occurs
//! \pre       First initialize the mode
//! \param     None
//! \return    Status of the collision
static T_Collision CheckCollisionStatus(void);

//! \brief     Launch the record animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void LaunchRecordAnimation(void);

//! \brief     Launch the table overflow animation
//! \pre       First initialize the mode
//! \param     buttonState - State of the buttons
//! \param     command - Command of the remote control
//! \return    None
static void LaunchOverflowAnimation(uint8_t *buttonState, uint8_t command);

//! \brief     Launch the play animation
//! \pre       First initialize the mode
//! \param     next - Next movement or rotation
//! \return    None
static void LaunchPlayAnimation(uint8_t next);

//! \brief     Launch the pause animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void LaunchPauseAnimation(void);

//! \brief     Interrupt called at the end of a movement
//! \pre       First initialize the mode
//! \param     arg - Not used
//! \return    None
static void IRAM_ATTR ISR_EndOfMovement(void *arg);

//! \brief     Callback called at the end of the delay added after a movement
//! \pre       First initialize the mode
//! \param     arg - Not used
//! \return    None
static void Callback_TimerStop(void *arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sequence_Init(void)
{
  RecordSequenceIsFinished = false;
  MovementIsInProgress = false;
  RotationIsInProgress = false;

  TimerHw_Init(1, 0, true, MOVEMENT_DURATION, ISR_EndOfMovement);
  StopTimer = TimerSw_Create(STOP_DURATION_us, Callback_TimerStop);

  WrPos = 0u;
  RdPos = 0u;
}

//_____________________________________________________________________________

void Sequence_Start(void)
{
  State = E_State_Record;
  RdPos = 0u;
  MovementIsInProgress = false;
  RotationIsInProgress = false;
  RecordSequenceIsFinished = false;
  //PlayState = E_PlayState_Prepare;
  PlayState = E_PlayState_Replay;
  Accelerometer_ClearTapStatus(); // Clear any tap made before entering this mode
}

//_____________________________________________________________________________

void Sequence_Stop(void)
{
  Common_SetTargetSpeed(0, 0);
  AngleController_Stop();
}

//_____________________________________________________________________________

void Sequence_Run(void)
{
  static int16_t toggle = -1;
  int16_t command = RC5_GetCommand(&toggle);  
  uint8_t brightness = Common_GetBodyColorPulse();
  if((State == E_State_Erase_All_In_Progress) || (State == E_State_Erase_Last_Step_In_Progress)) {
    // Leds handle in "ProcessEraseAction"   
  } else {
    /*
    if(PlayStatePrepareToPlay) {
      if(PlayStatePrepareCount == 0) {// Based on behaviors update rate of 50 hz =>  Red
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, 0, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, 0, 0);      
      } else if(PlayStatePrepareCount == 50) { // Based on behaviors update rate of 50 hz =>  orange
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS/2, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS/2, 0);
      } else if(PlayStatePrepareCount == 100) { // Based on behaviors update rate of 50 hz =>  green
        Leds_SetFrontBrightness(0, MAX_BRIGHTNESS, 0);
        Leds_SetBackBrightness(0, MAX_BRIGHTNESS, 0);
      }
    } else {
    */
      Leds_SetFrontBrightness(brightness, 0, brightness);
      Leds_SetBackBrightness(0, brightness, brightness);
    //}
  }

  switch (State)
  {
  case E_State_Record:
    RecordSequence();
    break;

  case E_State_Play:
    PlaySequence();
    break;

  case E_State_Erase_All_In_Progress:
    Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);
    break;

  case E_State_Erase_Last_Step_In_Progress:
    Leds_SetCircleBrightness(0, 0, 0, 0, 0, 0, 0, 0);  
    break;

  default:
    // Do nothing
    break;
  }

  ProcessEraseAction(command);
  
}

//_____________________________________________________________________________

static void UpdateLegoLeds(uint8_t totalSteps, uint8_t currStep)
{  
  if(State == E_State_Record) { // Recording sequence
    Leds_SetLegoProgress(totalSteps);    
  } else { // Play sequence
    LegoLedBlinkCount++;
    if(currStep>=1 && currStep<=8) 
    {
      if(LegoLedBlinkCount == 12) {
        Leds_SetLegoProgress(totalSteps);
        Leds_SetSingleBrightness(32-currStep, MAX_BRIGHTNESS);
      } else if(LegoLedBlinkCount == 24) {
        Leds_SetLegoProgress(totalSteps);
        Leds_SetSingleBrightness(32-currStep, 0);
        LegoLedBlinkCount = 0;
      }   
    } 
    else if (currStep>=9 && currStep<=16)
    {
      if(LegoLedBlinkCount == 12) {
        Leds_SetLegoProgress(totalSteps);
        Leds_SetSingleBrightness(48-currStep, MAX_BRIGHTNESS);
      } else if(LegoLedBlinkCount == 24) {
        Leds_SetLegoProgress(totalSteps);
        Leds_SetSingleBrightness(48-currStep, 0);
        LegoLedBlinkCount = 0;
      }
    } else {
      LegoLedBlinkCount = 0;
    }
  }

}

//_____________________________________________________________________________

static void RecordSequence(void)
{
  static int16_t toggle = -1;
  uint8_t *buttonState;
  uint8_t data = 0u;
  int16_t command = RC5_GetCommand(&toggle);

  buttonState = Buttons_GetStatus();

  if (WrPos < SEQUENCE_BUFFER_SIZE)
  {
    LaunchRecordAnimation();

    when(buttonState[E_Button_Backward] || (command == E_Command_DownArrow) || (command == E_Command_8))
    {
      data |= (1u << E_Button_Backward);
      Sequence[WrPos] = data;
      ESP_LOGI(Tag, "Val: %d, Pos: %d", Sequence[WrPos], WrPos);
      WrPos++;
    }

    when(buttonState[E_Button_Left] || (command == E_Command_LeftArrow) || (command == E_Command_4))
    {
      data |= (1u << E_Button_Left);
      Sequence[WrPos] = data;
      ESP_LOGI(Tag, "Val: %d, Pos: %d", Sequence[WrPos], WrPos);
      WrPos++;
    }

    when(buttonState[E_Button_Forward] || (command == E_Command_UpArrow) || (command == E_Command_2))
    {
      data |= (1u << E_Button_Forward);
      Sequence[WrPos] = data;
      ESP_LOGI(Tag, "Val: %d, Pos: %d", Sequence[WrPos], WrPos);
      WrPos++;
    }

    when(buttonState[E_Button_Right] || (command == E_Command_RightArrow) || (command == E_Command_6))
    {
      data |= (1u << E_Button_Right);
      Sequence[WrPos] = data;
      ESP_LOGI(Tag, "Val: %d, Pos: %d", Sequence[WrPos], WrPos);
      WrPos++;
    }

    UpdateLegoLeds(WrPos, RdPos);
  }
  else // The table is full
  {
    LaunchOverflowAnimation(buttonState, command);
  }

  when(command == E_Command_Go)
  {
    RecordSequenceIsFinished = true;
    State = E_State_Play;
    //PlayStatePrepareCount = 0;
    //PlayStatePrepareToPlay = true;
    //PlayState = E_PlayState_Prepare;
    PlayState = E_PlayState_Replay;
    ESP_LOGI(Tag, "End of recording");
  }

  switch(StartFromProxState)
  {
    case 0:
      if((GetProximityValue(1) > (PROX_THRESHOLD+200)) && (GetProximityValue(2) > (PROX_THRESHOLD+200)) && (GetProximityValue(3) > (PROX_THRESHOLD+200)))
      {
        PlayStatePrepareCount++;  // Based on behaviors update rate of 50 hz
        if(PlayStatePrepareCount == 5) // After 100 ms
        {
          StartFromProxState = 1;
        }
      } else {
        PlayStatePrepareCount = 0;
      }
      break;
    
    case 1:
      if((GetProximityValue(1) < (PROX_THRESHOLD-200)) && (GetProximityValue(2) < (PROX_THRESHOLD-200)) && (GetProximityValue(3) < (PROX_THRESHOLD-200)))
      {
        PlayStatePrepareCount++;  // Based on behaviors update rate of 50 hz
        if(PlayStatePrepareCount == 25) // After 0.5 second
        {
          RecordSequenceIsFinished = true;
          State = E_State_Play;
          PlayState = E_PlayState_Replay;
          StartFromProxState = 0;
        }
      } else {
        PlayStatePrepareCount = 0;
      }
      break;
  }

}

//_____________________________________________________________________________

static void ProcessEraseAction(uint8_t command)
{
  int16_t accelerationY = Accelerometer_GetAccelerationY();
  int16_t accelerationZ = Accelerometer_GetAccelerationZ();

  static uint8_t eraseAllState = 0;
  static uint8_t countEraseAll = 0u;
  static uint8_t eraseLastStepState = 0;
  static uint8_t countEraseLastStep = 0;
  static uint8_t countEraseExit = 0;
  static uint8_t ledCounter = 0;

  //printf("acc z = %d\r\n", accelerationZ);

  // To erase the last step, place the Thymio on the left or right side.
  switch(eraseLastStepState) 
  {
    case 0: // Check accelerometer value
      if (abs(accelerationY) >= 15000)
      {
        countEraseLastStep++;

        if (countEraseLastStep > 50) // After  second
        {
          eraseLastStepState = 1;
          countEraseLastStep = 0;
          State = E_State_Erase_Last_Step_In_Progress;
          ledCounter = 0;
          countEraseExit = 0;
        }
      }
      else
      {
        countEraseLastStep = 0u;
      }
      break;
    case 1: // Show the user that the last step will be erase in a few seconds
      countEraseLastStep++;
      if(countEraseLastStep > 100) { // After 2 seconds remove last step
        if(WrPos > 0) {
          WrPos--;
        }
        RdPos = 0u;
        UpdateLegoLeds(WrPos, RdPos);
        Codec_PlayOnboardSound(TONE_TYPE_MAGIC);
        eraseLastStepState = 2;
        countEraseExit = 0;
        Leds_SetFrontBrightness(0, 0, 0);
        Leds_SetBackBrightness(0, 0, 0);        
      }
      if (abs(accelerationY) < 15000) // Exit erase sequence
      {
        countEraseExit++;
        if(countEraseExit > 4)
        {
          eraseLastStepState = 0;
          State = E_State_Record;
          RdPos = 0;  // Restart from the beginning if already started
        }
      } 
      else 
      {
        countEraseExit = 0;
      }    
      if(ledCounter < 20) 
      {
        ledCounter++;
      }
      else
      {
        ledCounter = 0;
      }
      if(ledCounter < 10) {
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS/2, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, MAX_BRIGHTNESS/2, 0);         
      } else {
        Leds_SetFrontBrightness(0, 0, 0);
        Leds_SetBackBrightness(0, 0, 0);           
      }    
      break;
    case 2: // Wait the robot to return in normal position.
      if (abs(accelerationY) < 15000) // Exit erase sequence
      {
        countEraseExit++;
        if(countEraseExit > 4)
        {
          eraseLastStepState = 0;
          State = E_State_Record;
          RdPos = 0;  // Restart from the beginning if already started          
        }
      } 
      else 
      {
        countEraseExit = 0;
      }  
      break;
    default:
      break;
  }

  // To erase completely the sequence, place the Thymio upside-down or press the stop button on the remote control.
  switch(eraseAllState) {
    case 0: // Check accelerometer value and tv remote commands
      if (accelerationZ <= -13000)
      {
        countEraseAll++;
        if (countEraseAll > 50) // After 1 second
        {
          eraseAllState = 1;
          countEraseExit = 0;
          State = E_State_Erase_All_In_Progress;
          countEraseAll = 0u;
        }
      }
      else
      {
        countEraseAll = 0u;
      }  
      // When receiving a command via tv remote, then erase immediately
      if ((command == E_Command_Stop) || (command == E_Command_5))
      {
        eraseAllState = 4;
        countEraseAll = 0u;
      }
      break;
    case 1: // Play a sound telling the user the sequence will be erased in a few seconds 
      Codec_PlayOnboardSound(TONE_TYPE_ALARM);
      eraseAllState = 2;
      ledCounter = 0;
      break;
    case 2: // Wait for the sound to finish while showing the user that the erase sequence will be performed in a few seconds
      if(Codec_IsSoundFinished()) {
        eraseAllState = 3;
      }
      if (accelerationZ > -13000) // Exit erase sequence
      {
        countEraseExit++;
        if(countEraseExit > 4)
        {
          eraseAllState = 0;
          State = E_State_Record;
          RdPos = 0;  // Restart from the beginning if already started          
        }
      } 
      else 
      {
        countEraseExit = 0;
      }
      if(ledCounter < 20) 
      {
        ledCounter++;
      }
      else
      {
        ledCounter = 0;
      }
      if(ledCounter < 10) {
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, 0, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, 0, 0);         
      } else {
        Leds_SetFrontBrightness(0, 0, 0);
        Leds_SetBackBrightness(0, 0, 0);           
      }
      break;
    case 3: // Give the user some time to exit the erase sequence, otherwise after 2 seconds perform a complete erase
      countEraseAll++;
      if (countEraseAll > 100) // After 2 second erase
      {
        eraseAllState = 4;
        ledCounter = 0;
        countEraseAll = 0u;
        countEraseExit = 0;
      }
      if (accelerationZ > -13000) // Exit erase sequence
      {
        countEraseExit++;
        if(countEraseExit > 4)
        {
          eraseAllState = 0;
          State = E_State_Record;
          RdPos = 0;  // Restart from the beginning if already started
        }
      } 
      else 
      {
        countEraseExit = 0;
      }
      if(ledCounter < 10) 
      {
        ledCounter++;
      }
      else
      {
        ledCounter = 0;
      }
      if(ledCounter < 5) {
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, 0, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, 0, 0);         
      } else {
        Leds_SetFrontBrightness(0, 0, 0);
        Leds_SetBackBrightness(0, 0, 0);           
      }
      break;
    case 4: // Erase the sequence, this is immediate but show the user red for 2 seconds and then wait the robot to return in normal position.
      WrPos = 0u;
      RdPos = 0u;      
      if(ledCounter < 100) 
      {
        Leds_SetFrontBrightness(MAX_BRIGHTNESS, 0, 0);
        Leds_SetBackBrightness(MAX_BRIGHTNESS, 0, 0);        
        ledCounter++;
      }
      if(ledCounter == 100) {
        Leds_SetFrontBrightness(0, 0, 0);
        Leds_SetBackBrightness(0, 0, 0); 
        UpdateLegoLeds(WrPos, RdPos);
      }
      if (accelerationZ > -13000) // Exit erase sequence
      {
        countEraseExit++;
        if(countEraseExit > 4)
        {
          eraseAllState = 0;
          State = E_State_Record;
        }
      } 
      else 
      {
        countEraseExit = 0;
      }      
      break;
    default:
      break;
  }

}

//_____________________________________________________________________________

static void PlaySequence(void)
{
  switch (PlayState)
  {
  case E_PlayState_Prepare:
    HandlePrepare();
    break;
  case E_PlayState_Replay:
    HandleReplay();
    UpdateLegoLeds(WrPos, RdPos);
    break;

  case E_PlayState_Movement:
    HandleMovement();
    UpdateLegoLeds(WrPos, RdPos);
    break;

  case E_PlayState_Rotation:
    HandleRotation();
    UpdateLegoLeds(WrPos, RdPos);
    break;

  case E_PlayState_Collision:
    HandleCollision();
    UpdateLegoLeds(WrPos, RdPos);
    break;

  default:
    // Do nothing
    break;
  }
}

//_____________________________________________________________________________

static void HandlePrepare(void)
{
  /*
  PlayStatePrepareCount++;  // Based on behaviors update rate of 50 hz
  if ((GetGroundValue(0) < GROUND_IR_THRESHOLD) ||
      (GetGroundValue(1) < GROUND_IR_THRESHOLD)) // Robot raised, not start but instead come back to recording mode
  {
    PlayStatePrepareToPlay = false;
    RecordSequenceIsFinished = false; // Allow a new buttons recording sequence
    RdPos = 0u;
    State = E_State_Record;
    //ESP_LOGI(Tag, "Replay aborted");
    Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
    Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);    
  }
  if(PlayStatePrepareCount == 150) 
  {
    PlayState = E_PlayState_Replay;
    PlayStatePrepareToPlay = false;
  }
  */
}

//_____________________________________________________________________________

static void HandleReplay(void)
{
  uint8_t next = 0u;

  if (RdPos < WrPos)
  {
    ESP_LOGI(Tag, "Val: %d, Pos: %d", Sequence[RdPos], RdPos);    

    if (!MovementIsInProgress && !RotationIsInProgress)
    {
      Current = Sequence[RdPos];
      next = Sequence[RdPos + 1u];
      RdPos++;

      //UpdateLegoLeds(RdPos);

      if (Current == (1u << E_Button_Backward))
      {
        TimerHw_Start(1, 0);
        MovementIsInProgress = true;
        Common_SetTargetSpeed(-MOVEMENT_SPEED_BW, -MOVEMENT_SPEED_BW); // TODO Check why the speed in backward direction is slower
        PlayState = E_PlayState_Movement;
      }

      if (Current == (1u << E_Button_Forward))
      {
        TimerHw_Start(1, 0);
        MovementIsInProgress = true;
        Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
        PlayState = E_PlayState_Movement;
      }

      if (Current == (1u << E_Button_Left))
      {
        RotationIsInProgress = true;
        AngleController_Start(90, MAX_ROTATION_SPEED);
        RotationCompletedDelay = 0;
        PlayState = E_PlayState_Rotation;
      }

      if (Current == (1u << E_Button_Right))
      {
        RotationIsInProgress = true;
        AngleController_Start(-90, MAX_ROTATION_SPEED);
        RotationCompletedDelay = 0;
        PlayState = E_PlayState_Rotation;
      }

      LaunchPlayAnimation(next);
    }
  }
  else // Handle the last movement or rotation of the sequence
  {
    RecordSequenceIsFinished = false; // Allow a new buttons recording sequence
    RdPos = 0u;
    State = E_State_Record;
    ESP_LOGI(Tag, "Last movement is finished");
    Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
    Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
  }
}

//_____________________________________________________________________________

static void HandleMovement(void)
{
  if (CheckCollisionStatus() == E_Collision_Detected)
  {
    TimerHw_Stop(1, 0);
    Common_SetTargetSpeed(0, 0);

    PlayState = E_PlayState_Collision;
  }
  // An obstacle has been detected but it no longer obstructs the passage
  else if (ObstacleIsDetected)
  {
    TimerHw_Start(1, 0);

    if (Current == (1u << E_Button_Forward))
    {
      Common_SetTargetSpeed(MOVEMENT_SPEED, MOVEMENT_SPEED);
    }
    else if (Current == (1u << E_Button_Backward))
    {
      Common_SetTargetSpeed(-MOVEMENT_SPEED_BW, -MOVEMENT_SPEED_BW);
    }
    else
    {
      // Do nothing
    }

    ObstacleIsDetected = false;
  }
  else
  {
    // Continue the movement
  }
}

//_____________________________________________________________________________

static void HandleRotation(void)
{
  if (CheckCollisionStatus() == E_Collision_Detected)
  {
    Common_SetTargetSpeed(0, 0);

    PlayState = E_PlayState_Collision;
  }
  else if (AngleController_Completed())
  {
    RotationCompletedDelay++;
    if(RotationCompletedDelay >= (STOP_DURATION_us/20000)) // Based on behaviors update rate of 50 hz
    {
      ObstacleIsDetected = false;
      RotationIsInProgress = false;

      PlayState = E_PlayState_Replay;
    }
  }
  else
  {
    // Continue the rotation
  }
}

//_____________________________________________________________________________

static void HandleCollision(void)
{
  static bool first = true;

  ObstacleIsDetected = true;
  LaunchPauseAnimation();

  if (first)
  {
    Codec_Stop();
    Codec_PlayOnboardSound(TONE_TYPE_DETECT);
    first = false;
  }

  if (CheckCollisionStatus() == E_Collision_Handled)
  {
    Leds_SetFrontBrightness(MAX_BRIGHTNESS, 0, MAX_BRIGHTNESS);
    Leds_SetBackBrightness(0, MAX_BRIGHTNESS, MAX_BRIGHTNESS);

    if (MovementIsInProgress)
    {
      PlayState = E_PlayState_Movement;
    }
    else if (RotationIsInProgress)
    {
      PlayState = E_PlayState_Rotation;
    }
    else
    {
      // Do nothing
    }

    first = true;
  }
}

//_____________________________________________________________________________

static T_Collision CheckCollisionStatus(void)
{
  T_ProxIR proxIR = STM32_GetProxIRValues();
  T_Collision status = E_Collision_None;

  if ((proxIR.FrontLeft > COLLISION_THRESHOLD) ||
      (proxIR.FrontLeftCenter > COLLISION_THRESHOLD) ||
      (proxIR.FrontCenter > COLLISION_THRESHOLD) ||
      (proxIR.FrontRightCenter > COLLISION_THRESHOLD) ||
      (proxIR.FrontRight > COLLISION_THRESHOLD))
  {
    status = E_Collision_Detected;
  }
  else if ((proxIR.FrontLeft < NO_COLLISION_THRESHOLD) &&
           (proxIR.FrontLeftCenter < NO_COLLISION_THRESHOLD) &&
           (proxIR.FrontCenter < NO_COLLISION_THRESHOLD) &&
           (proxIR.FrontRightCenter < NO_COLLISION_THRESHOLD) &&
           (proxIR.FrontRight < NO_COLLISION_THRESHOLD))
  {
    status = E_Collision_Handled;
  }
  else
  {
    // Do nothing
  }

  return status;
}

//_____________________________________________________________________________

static void LaunchRecordAnimation(void)
{
  static uint8_t led_state = 0u;
  uint8_t l[8] = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u};
  uint8_t fixed;

  led_state += 2u;
  fixed = (led_state / MAX_BRIGHTNESS);

  l[fixed & 0x7u] = MAX_BRIGHTNESS;
  l[(fixed - 2u) & 0x7u] = (MAX_BRIGHTNESS - (led_state & (MAX_BRIGHTNESS - 1u)));
  l[(fixed + 2u) & 0x7u] = (led_state & (MAX_BRIGHTNESS - 1u));
  l[(fixed + 4u) & 0x7u] = (led_state & (MAX_BRIGHTNESS - 1u));

  Leds_SetCircleBrightness(l[0], l[1], l[2], l[3], l[4], l[5], l[6], l[7]);
}

//_____________________________________________________________________________

static void LaunchOverflowAnimation(uint8_t *buttonState, uint8_t command)
{
  static uint8_t brightness = 0u;

  Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u);

  when(buttonState[E_Button_Backward] || (command == E_Command_DownArrow) ||
       buttonState[E_Button_Left] || (command == E_Command_LeftArrow) ||
       buttonState[E_Button_Forward] || (command == E_Command_UpArrow) ||
       buttonState[E_Button_Right] || (command == E_Command_RightArrow))
  {
    brightness = 1u;
  }

  // Launch the animation when the user tries to add a movement while the table is already full
  if (brightness > 0u)
  {
    brightness++;

    Leds_SetBodyBrightness(brightness, 0u, 0u);

    if (brightness == MAX_BRIGHTNESS)
    {
      brightness = 0u;
    }
  }
}

//_____________________________________________________________________________

static void LaunchPlayAnimation(uint8_t next)
{
  if (next == 0u)
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS, 0u);
  }

  if (next == (1u << E_Button_Backward))
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u);
  }

  if (next == (1u << E_Button_Forward))
  {
    Leds_SetCircleBrightness(MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
  }

  if (next == (1u << E_Button_Left))
  {
    Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, MAX_BRIGHTNESS, 0u);
  }

  if (next == (1u << E_Button_Right))
  {
    Leds_SetCircleBrightness(0u, 0u, MAX_BRIGHTNESS, 0u, 0u, 0u, 0u, 0u);
  }
}

//_____________________________________________________________________________

static void LaunchPauseAnimation(void)
{
  Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_EndOfMovement(void *arg)
{
  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[0].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if (intr_status & BIT(0))
  {
    Common_SetTargetSpeed(0, 0);

    TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_us);

    TIMERG1.int_clr_timers.t0 = 1;
    TimerHw_Stop(1, 0);
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
}

//_____________________________________________________________________________

static void Callback_TimerStop(void *arg)
{
  MovementIsInProgress = false;
  PlayState = E_PlayState_Replay;
}
