//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    responsive.c
//! \brief   This module provides the useful functions to use the responsive mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"

#include "responsive.h"

#include "accelerometer.h"
#include "angle_controller.h"
#include "aseba_esp32.h"
#include "behavior.h"
#include "buttons.h"
#include "codec.h"
#include "common.h"
#include "fifo.h"
#include "gyroscope.h"
#include "leds.h"
#include "rc5.h"
#include "stm32_i2c.h"
#include "timer_sw.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define SEQUENCE_BUFFER_SIZE           50u  //!< Number of actions stored in the FIFO

// Duration of the pulse = 1500000 [us] -> TIMER_SCALE * 1500000 [us] = 7500000 (timer_group)
#define MOVEMENT_DURATION_us      7500000u
//#define MOVEMENT_DURATION_us      1500000u  //!< Duration of a movement
#define STOP_DURATION_us           500000u  //!< Delay at the end of a movement

#define MOVEMENT_SPEED                300   //!< Movement speed

#define MAX_ROTATION_SPEED            500   //!< Maximum rotation speed allowed

#define ROTATION_ANGLE              16383   //!< Rotation angle corresponding to 90° (0x3FFF)

#define COLLISION_THRESHOLD           700   //!< Collision threshold

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details States of the state machine
typedef enum
{
  E_State_Record,
  E_State_Erase,
  E_State_Play
} T_State;

//! \details States of the play state machine
typedef enum
{
  E_PlayState_Replay,
  E_PlayState_Movement,
  E_PlayState_Rotation,
  E_PlayState_Collision
} T_PlayState;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "responsive";

static T_FifoBytes* SequenceFifo = NULL;

static bool RecordSequenceIsFinished = false;
static bool MovementIsInProgress = false;
static bool RotationIsInProgress = false;
static bool FirstRecording = false;
static bool ObstacleIsDetected = false;

static uint16_t Position = 0u;  //!< Consume position of the FIFO

static T_TimerSw* StopTimer = NULL;      //!< Used to stop the robot in responsive mode

static T_State State = E_State_Record;
static T_PlayState PlayState = E_PlayState_Replay;

static uint8_t Current = 0u;
static int16_t AngleTarget = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Record the sequence
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RecordSequence(void);

//! \brief     Erase the sequence
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void EraseSequence(void);

//! \brief     Play the sequence
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void PlaySequence(void);

static void HandleReplay(void);

static void HandleMovement(void);

static void HandleRotation(void);

//! \brief     Handle the collision
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void HandleCollision(void);

//! \brief     Run the record animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RunRecordAnimation(void);

//! \brief     Run the erase animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RunEraseAnimation(void);

//! \brief     Run the play animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RunPlayAnimation(uint8_t next);

//! \brief     Run the pause animation
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void RunPauseAnimation(void);

static void IRAM_ATTR ISR_EndOfMovement(void* para);

//! \brief     Callback called at the end of the delay added after a movement
//! \pre       First initialize the mode
//! \param     None
//! \return    None
static void Callback_TimerStop(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Responsive_Init(void)
{
  uint8_t buffer[SEQUENCE_BUFFER_SIZE];

  SequenceFifo = Fifo8bits_Create(buffer, SEQUENCE_BUFFER_SIZE);

  RecordSequenceIsFinished = false;
  MovementIsInProgress = false;
  RotationIsInProgress = false;

  TimerHw_Init(1, 0, true, MOVEMENT_DURATION_us, ISR_EndOfMovement);
  StopTimer = TimerSw_Create(STOP_DURATION_us, Callback_TimerStop);
}

//_____________________________________________________________________________

void Responsive_Start(void)
{
  RecordSequenceIsFinished = false;
  Accelerometer_ClearTapStatus();  // Clear any tap made before entering this mode
}

//_____________________________________________________________________________

void Responsive_Stop(void)
{
  vmVariables.target[0] = 0;
  vmVariables.target[1] = 0;
}

//_____________________________________________________________________________

void Responsive_Run(void)
{
  switch (State)
  {
    case E_State_Record:
      RecordSequence();
      break;

    case E_State_Erase:
      EraseSequence();
      break;

    case E_State_Play:
      PlaySequence();
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void RecordSequence(void)
{
  uint8_t* buttonState;
  uint8_t  data = 0u;
  int16_t command = 0;

  static int16_t toggle = 0;

  RunRecordAnimation();

  buttonState = Buttons_GetStatus();

  if (RC5_IsNewMessageReceived(&toggle))
  {
    command = RC5_GetCommand();
  }

  when(buttonState[E_Button_Backward] || (command == E_Command_DownArrow))
  {
    data |= (1 << E_Button_Backward);
    Fifo8bits_Write(SequenceFifo, &data, 1u);
  }

  when(buttonState[E_Button_Left] || (command == E_Command_LeftArrow))
  {
    data |= (1 << E_Button_Left);
    Fifo8bits_Write(SequenceFifo, &data, 1u);
  }

  when(buttonState[E_Button_Forward] || (command == E_Command_UpArrow))
  {
    data |= (1 << E_Button_Forward);
    Fifo8bits_Write(SequenceFifo, &data, 1u);
  }

  when(buttonState[E_Button_Right] || (command == E_Command_RightArrow))
  {
    data |= (1 << E_Button_Right);
    Fifo8bits_Write(SequenceFifo, &data, 1u);
  }

  when(Accelerometer_IsTapDetected() || (command == E_Command_Go))
  {
    RecordSequenceIsFinished = true;

    // Initialize the position to play the sequence
    Fifo8bits_SetConsumePosition(SequenceFifo, Position);

    if (!FirstRecording)
    {
      FirstRecording = true;
      Position = Fifo8bits_GetConsumePosition(SequenceFifo);
    }

    State = E_State_Play;

    ESP_LOGE(Tag, "End of recording");
  }

  when(command == E_Command_Stop)
  {
    State = E_State_Erase;
  }
}

//_____________________________________________________________________________

static void EraseSequence(void)
{
  Position = Fifo8bits_GetConsumePosition(SequenceFifo);
  FirstRecording = false;
  RunEraseAnimation();

  State = E_State_Record;
}

//_____________________________________________________________________________

static void PlaySequence(void)
{
  switch (PlayState)
  {
    case E_PlayState_Replay:
      HandleReplay();
      break;

    case E_PlayState_Movement:
      HandleMovement();
      break;

    case E_PlayState_Rotation:
      HandleRotation();
      break;

    case E_PlayState_Collision:
      HandleCollision();
      break;

    default:
      // Do nothing
      break;
  }
}

//_____________________________________________________________________________

static void HandleReplay(void)
{
  uint8_t next = 0u;

  if (!Fifo8bits_IsEmpty(SequenceFifo))
  {
    if (!MovementIsInProgress && !RotationIsInProgress)
    {
      Fifo8bits_Read(SequenceFifo, &Current, 1);
      Fifo8bits_Peek(SequenceFifo, &next, 1);

      if (Current == (1u << E_Button_Backward))
      {
        TimerHw_Start(1, 0);
        MovementIsInProgress = true;
        vmVariables.target[0] = -MOVEMENT_SPEED;  // TODO Check why the speed in backward direction is slower
        vmVariables.target[1] = -MOVEMENT_SPEED;
        PlayState = E_PlayState_Movement;
      }

      if (Current == (1u << E_Button_Forward))
      {
        TimerHw_Start(1, 0);
        MovementIsInProgress = true;
        vmVariables.target[0] = MOVEMENT_SPEED;
        vmVariables.target[1] = MOVEMENT_SPEED;
        PlayState = E_PlayState_Movement;
      }

      if (Current == (1u << E_Button_Left))
      {
        RotationIsInProgress = true;
        AngleTarget = ROTATION_ANGLE;  // + 90°
        Gyroscope_ResetAngle();
        PlayState = E_PlayState_Rotation;
      }

      if (Current == (1u << E_Button_Right))
      {
        RotationIsInProgress = true;
        AngleTarget = -ROTATION_ANGLE;  // -90°
        Gyroscope_ResetAngle();
        PlayState = E_PlayState_Rotation;
      }

      RunPlayAnimation(next);
    }
  }
  else  // Handle the last movement or rotation of the sequence
  {
    ESP_LOGE(Tag, "Last movement in progress");
    //if (!MovementIsInProgress)
    {
      RecordSequenceIsFinished = false;  // Allow a new buttons recording sequence
      State = E_State_Record;
      ESP_LOGE(Tag, "Last movement finished");
    }
  }
}

//_____________________________________________________________________________

static void HandleMovement(void)
{
  T_ProxIR proxIR = STM32_GetProxIRValue();

  if (proxIR.FrontCenter > COLLISION_THRESHOLD)
  {
    TimerHw_Stop(1, 0);
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;

    PlayState = E_PlayState_Collision;
  }
  else if (ObstacleIsDetected)
  {
    TimerHw_Start(1, 0);

    if (Current == (1u << E_Button_Forward))
    {
      vmVariables.target[0] = MOVEMENT_SPEED;
      vmVariables.target[1] = MOVEMENT_SPEED;
    }
    else if (Current == (1u << E_Button_Backward))
    {
      vmVariables.target[0] = -MOVEMENT_SPEED;
      vmVariables.target[1] = -MOVEMENT_SPEED;
    }

    ObstacleIsDetected = false;
  }
  else
  {
    // Wait the end of the movement
  }
}

//_____________________________________________________________________________

static void HandleRotation(void)
{
  T_ProxIR proxIR = STM32_GetProxIRValue();

  if (proxIR.FrontCenter > COLLISION_THRESHOLD)
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;

    PlayState = E_PlayState_Collision;
  }
  else if (AngleController_Update(AngleTarget, MAX_ROTATION_SPEED) == 0)
  {
    ObstacleIsDetected = false;
    RotationIsInProgress = false;

    PlayState = E_PlayState_Replay;
  }
}

//_____________________________________________________________________________

static void HandleCollision(void)
{
  static bool first = true;

  T_ProxIR proxIR = STM32_GetProxIRValue();

  ObstacleIsDetected = true;
  RunPauseAnimation();

  if (first)
  {
    Codec_PlayMP3FileFromFlash(E_SystemSound_Detection);
    first = false;
  }

  if (proxIR.FrontCenter < 10)
  {
    Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, MAX_BRIGHTNESS);
    first = true;

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
  }
}

//_____________________________________________________________________________

static void RunRecordAnimation(void)
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

static void RunEraseAnimation(void)
{
  Codec_PlayMP3FileFromFlash(E_SystemSound_Detection);
}

//_____________________________________________________________________________

static void RunPlayAnimation(uint8_t next)
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

static void RunPauseAnimation(void)
{
  Leds_SetBodyBrightness(MAX_BRIGHTNESS, 0u, 0u);
}

//_____________________________________________________________________________

static void IRAM_ATTR ISR_EndOfMovement(void* para)
{
  // Retrieve the interrupt status and the counter value
  // from the timer that reported the interrupt
  uint32_t intr_status = TIMERG1.int_st_timers.val;
  TIMERG1.hw_timer[0].update = 1;

  // Clear the interrupt and update the alarm time for the timer with without reload
  if (intr_status & BIT(0))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;

    //PlayState = E_PlayState_Delay;
    TimerSw_StartTimerOnce(StopTimer, STOP_DURATION_us);

    TIMERG1.int_clr_timers.t0 = 1;
    timer_pause(1, 0);
  }

  // After the alarm has been triggered, we need enable it again, so it is triggered the next time
  TIMERG1.hw_timer[0].config.alarm_en = TIMER_ALARM_EN;
}

//_____________________________________________________________________________

static void Callback_TimerStop(void* arg)
{
  MovementIsInProgress = false;
  PlayState = E_PlayState_Replay;
}
