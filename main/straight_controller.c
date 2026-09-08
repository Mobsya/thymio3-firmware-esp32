//_____________________________________________________________________________
//
// Copyright (C) 2026                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    straight_controller.c
//! \brief   This module holds the heading of the robot during a straight motion
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"
#include "esp_timer.h"

#include "straight_controller.h"

#include "angle_controller.h"
#include "behavior.h"
#include "common.h"
#include "gyroscope.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define KP 0.1f  //!< Proportional gain, the error is in raw gyroscope ticks
#define KI 0.2f  //!< Integral gain, useful at high speed
#define KD 0.0f  //!< Derivative gain

#define D_FILTER     1.0f    //!< Low-pass on the derivative, 0.0 = frozen, 1.0 = no filter
#define MAX_INTEGRAL 1000.0f //!< Anti-windup limit on the integral term [ticks*s]
#define MAX_SPEED    ((float)MAX_LIMIT_SPEED) //!< Speed limit of the motors

#define NOMINAL_DT 0.02f //!< Behaviors run at 50 Hz
#define MAX_DT     0.1f  //!< A longer period means the loop was preempted, use the nominal one instead

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

//static const char* Tag = "straight_controller";

static bool running = false;        //!< True while the controller drives the motors
static int32_t targetAngle = 0;     //!< Heading held by the controller, in gyroscope ticks
static int16_t baseSpeed = 0;       //!< Base speed of both motors
static float maxCorrection = 0.0f;  //!< Max speed correction applied to each motor
static float integral = 0.0f;       //!< Integral term [ticks*s]
static float derivative = 0.0f;     //!< Filtered derivative term
static float lastError = 0.0f;      //!< Error of the previous iteration [ticks]
static int64_t lastTimeUs = 0;      //!< Timestamp of the previous iteration
static bool firstLoop = true;       //!< True until the first iteration is done

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static float Clamp(float value, float limit);
static void ApplySpeeds(int16_t base, float correction);
static void Arm(int32_t targetTicks, int16_t speed);

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

//! \brief     Limit a value to the range [-limit, +limit]
//! \param     value - Value to limit
//! \param     limit - Limit, its sign is ignored
//! \return    The limited value
static float Clamp(float value, float limit)
{
  if (limit < 0.0f) // A negative limit would invert the logic
  {
    limit = -limit;
  }

  if (value > limit)
  {
    return limit;
  }

  if (value < -limit)
  {
    return -limit;
  }

  return value;
}

//_____________________________________________________________________________

//! \brief     Apply the correction to both motors
//! \details   Only the difference between the two motors steers the robot, so a
//!            pair exceeding the speed limit is shifted as a whole instead of
//!            being clipped on one side, which would change the correction
//!            actually applied.
//! \param     base - Base speed of both motors, negative to go backward
//! \param     correction - Speed correction, positive to turn counterclockwise
//! \return    None
static void ApplySpeeds(int16_t base, float correction)
{
  float left = (float)base - correction;
  float right = (float)base + correction;
  float overflow = ((left > right) ? left : right) - MAX_SPEED;
  float underflow = 0.0f;

  if (overflow > 0.0f)
  {
    left -= overflow;
    right -= overflow;
  }

  underflow = -MAX_SPEED - ((left < right) ? left : right);
  if (underflow > 0.0f)
  {
    left += underflow;
    right += underflow;
  }

  Common_SetTargetSpeed((int16_t)((left < 0.0f) ? (left - 0.5f) : (left + 0.5f)),
                        (int16_t)((right < 0.0f) ? (right - 0.5f) : (right + 0.5f)));
}

//_____________________________________________________________________________

//! \brief     Arm the controller for a new straight motion
//! \param     targetTicks - Heading to hold, in gyroscope ticks
//! \param     speed - Base speed of both motors, negative to go backward
//! \return    None
static void Arm(int32_t targetTicks, int16_t speed)
{
  targetAngle = targetTicks;
  baseSpeed = speed;

  // A correction larger than the base speed would reverse one motor, turning the
  // straight motion into a rotation.
  maxCorrection = (speed < 0) ? (float)(-(int32_t)speed) : (float)speed;

  integral = 0.0f;
  derivative = 0.0f;
  lastError = 0.0f;
  firstLoop = true;
  lastTimeUs = esp_timer_get_time();

  // Written last: the update task only reads the parameters above once this
  // flag is set.
  running = true;
}

//_____________________________________________________________________________

void StraightController_Start(int16_t speed)
{
  Arm(Gyroscope_GetAngleZ(), speed);
}

//_____________________________________________________________________________

void StraightController_StartAbsolute(int16_t headingDeg, int16_t speed)
{
  Arm(((int32_t)headingDeg * rotation_angle_90_) / 90, speed);
}

//_____________________________________________________________________________

void StraightController_Stop(void)
{
  running = false;
}

//_____________________________________________________________________________

bool StraightController_IsRunning(void)
{
  return running;
}

//_____________________________________________________________________________

void StraightController_Update(void)
{
  int64_t now = 0;
  float dt = 0.0f;
  float error = 0.0f;
  float rawDerivative = 0.0f;
  float correction = 0.0f;

  if (!running)
  {
    return;
  }

  if (!Behavior_IsMotionRunning())
  {
    // The travel distance is reached: the timer ISR already released the motors,
    // so the controller stops without touching the speeds, leaving the stop
    // pause that follows the motion untouched.
    running = false;
    return;
  }

  // The real period is measured instead of being assumed, so that a preempted
  // cycle does not distort the integral and the derivative.
  now = esp_timer_get_time();
  dt = (float)(now - lastTimeUs) / 1000000.0f;
  lastTimeUs = now;

  if ((dt <= 0.0f) || (dt > MAX_DT))
  {
    dt = NOMINAL_DT;
  }

  // The error is kept in raw gyroscope ticks, as the gains were tuned.
  // Positive error => the robot has to turn counterclockwise to come back.
  error = (float)(targetAngle - Gyroscope_GetAngleZ());

  if (firstLoop)
  {
    firstLoop = false;
    lastError = error;
  }

  integral = Clamp(integral + (error * dt), MAX_INTEGRAL);
  rawDerivative = (error - lastError) / dt;
  derivative = (D_FILTER * rawDerivative) + ((1.0f - D_FILTER) * derivative);
  lastError = error;

  // Positive correction => turn left, negative correction => turn right
  correction = Clamp((KP * error) + (KI * integral) + (KD * derivative), maxCorrection);

  // Checked again right before writing: the timer ISR may have stopped the
  // motion while this iteration was being computed.
  if (Behavior_IsMotionRunning())
  {
    ApplySpeeds(baseSpeed, correction);
  }
}
