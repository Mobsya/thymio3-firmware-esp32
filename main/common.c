//_____________________________________________________________________________
//
// Copyright (C) 2020                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    common.c
//! \brief   This module provides the common mode functions
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "esp_log.h"
#include <stdio.h>
#include "common.h"
#include "esp_log.h"
#include "aseba_esp32.h"
#include "buttons.h"
#include "leds.h"
#include "stm32_spi.h"
#include "settings.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define GROUND_EDGE_OFFSET 100

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "common";
int16_t groundThr[2];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

uint8_t Common_GetBodyColorPulse(void)
{
  static int16_t pulse = 0;
  int16_t brightness = 0;

  pulse++; // Tasks run at 50 Hz => each increment every 20 ms

  if (pulse > 0)
  {
    brightness = (pulse>>2);

    if (pulse >= (int16_t)MAX_BRIGHTNESS << 2)  // To get to maximum brightness it takes (16*4)*20 = 1280 ms
    {
      pulse = -((int16_t)MAX_BRIGHTNESS << 3);  // To get to minimum birghtness (=0) it takes (16*8)*20 = 2560 ms
    }
  }
  else
  {
    brightness = -(pulse>>3);
  }

  return (uint8_t)brightness;
}

//_____________________________________________________________________________

void Common_SetTargetSpeed(int16_t left, int16_t right)
{
  vmVariables.target[0] = left;
  vmVariables.target[1] = right;
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_IncrementTargetSpeed(int16_t left, int16_t right)
{
  vmVariables.target[0] += left;
  vmVariables.target[1] += right;
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_LimitSpeed(int16_t min, int16_t max)
{
  if (vmVariables.target[0] < min)
  {
    vmVariables.target[0] = min;
  }
  else if (vmVariables.target[0] > max)
  {
    vmVariables.target[0] = max;
  }
  else
  {
    // Do nothing
  }

  if (vmVariables.target[1] < min)
  {
    vmVariables.target[1] = min;
  }
  else if (vmVariables.target[1] > max)
  {
    vmVariables.target[1] = max;
  }
  else
  {
    // Do nothing
  }
  SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);
}

//_____________________________________________________________________________

void Common_HandlePositiveSpeed(int16_t speed)
{
  static int32_t result = 0;
  static uint8_t still_counter = 0;
  static uint8_t escape_action_counter = 0;
  static int16_t direction = 0, prev_direction = -1;
  static uint8_t opposite_dir_counter = 0;
  static uint8_t last_dir_change_counter = 100;

  int32_t temp1 = 0;
  int32_t temp2 = 0;
  int16_t prox[7];
  GetProximityValues(prox);

  temp1 += (int32_t)prox[0];
  temp1 += (int32_t)prox[1];
  temp1 += (int32_t)prox[2];
  temp1 += (int32_t)prox[3];
  temp1 += (int32_t)prox[4];

  temp2 -= (((int32_t)prox[0])<<1);
  temp2 -= (((int32_t)prox[1])<<1);
  temp2 += (((int32_t)prox[3])<<1);
  temp2 += (((int32_t)prox[4])<<1);

  if((abs(temp2) < 100) && (prox[2] > 750)) { // If almost straight ahead of an obstacle and near it, then give more weight either to left or right sensors in order to avoid to block
    if(prox[1] > prox[3]) { // If left > right then give more weight to the left sensor
      temp2 -= 300;
    } else { // Otherwise give more weight to the right sensor
      temp2 += 300;
    }
  }

  //ESP_LOGI(Tag, "speed = %d, temp1 = %d, temp2 = %d", speed, temp1, temp2);
  result = (((temp1 + temp2) * speed) >> 10); // Divided by 1024
  vmVariables.target[0] = speed - result;
  result = (((temp1 - temp2) * speed) >> 10); // Divided by 1024
  vmVariables.target[1] = speed - result;

  // Add some extra conditions in order to escape situations in which the robot could block, e.g. corners.

  // Escape action activated, rotate right for a while.
  if(escape_action_counter > 0) {
    escape_action_counter--;
    vmVariables.target[0] = 200;
    vmVariables.target[1] = -200;
  }

  // Check when the robot is somehow still
  if((abs(vmVariables.target[0]) < 40) && (abs(vmVariables.target[1]) < 40)) {
    still_counter++;
    if(still_counter >= 100) { // This is called at 50 hz (from behaviors), thus it means after 2 seconds
      escape_action_counter = 120; // If after 2 seconds that the robot is somehow blocked (reached a corner?) then try an escape motion => turn right for about 1.5 sec
    }
  } else {
    still_counter = 0;
  }

  // Check when the robot continuously rotate left, right, left, right, ... within a few time.
  if((vmVariables.target[0] - vmVariables.target[1]) > 0) { // Positive = right rotation, negative = left rotation, 2=straight or still
    direction = 1;
  } else if ((vmVariables.target[0] - vmVariables.target[1]) < 0) {
    direction = -1;
  } else {
    direction = 2;
  }
  last_dir_change_counter++;
  if(((prev_direction + direction) == 0) && (last_dir_change_counter < 100)) { // If direction goes directly from left to right or viceversa within a few time (2 sec)
    opposite_dir_counter++;
    if(opposite_dir_counter >= 8) { // If the direction's change happens too many times (blocked in a corner?) then try an escape motion => turn right for about 1.5 sec
      escape_action_counter = 120;
    }
  } 
  if(last_dir_change_counter >= 100) { // If passed too much time (> 2 sec) then the robot is not blocked
    opposite_dir_counter = 0;
    last_dir_change_counter = 100;
  }
  if(prev_direction != direction) { // Change previous direction only when there is an actual direction's change because we need to account also for deceleration (direction doesn't change suddenly)
    prev_direction = direction;
    last_dir_change_counter = 0;
  }

  //ESP_LOGI(Tag, "target = %d %d", vmVariables.target[0], vmVariables.target[1]);
  Common_LimitSpeed(MIN_LIMIT_SPEED, MAX_LIMIT_SPEED);
}

//_____________________________________________________________________________

void Common_HandleNegativeSpeed(int16_t speed)
{
  int32_t temp = (int32_t)vmVariables.prox[6] * (int32_t)speed;
  vmVariables.target[0] = speed + (temp / -300);

  temp = ((int32_t)vmVariables.prox[5] * (int32_t)speed);
  vmVariables.target[1] = speed  + (temp / -300);

  Common_LimitSpeed(MIN_LIMIT_SPEED, MAX_LIMIT_SPEED);
}

//_____________________________________________________________________________

void Common_SetSpeedUsingButtons(int16_t* speed, int16_t increment, int16_t max, int16_t min)
{
  uint8_t* buttonState;

  buttonState = Buttons_GetStatus();

  when(buttonState[E_Button_Forward])
  {
    *speed = (*speed + increment);

    if (*speed > max)
    {
      *speed = max;
    }
  }

  when(buttonState[E_Button_Backward])
  {
    *speed = (*speed - increment);

    if (*speed < min)
    {
      *speed = min;
    }
  }
}

//_____________________________________________________________________________

uint8_t Common_HandleTableEdgeDetection(uint8_t red, uint8_t green, uint8_t blue)
{
  static bool first = true;

  if(first)
  {
    first = false;
    //Settings_GetGroundBlackSettings(groundThr);
    //groundThr[0] += GROUND_EDGE_OFFSET;
    //groundThr[1] += GROUND_EDGE_OFFSET;
    groundThr[0] = 300; // Calibrated ground values range is 0..1023, thus 300 is a good threshold to detect the table edge
    groundThr[1] = 300;
    ESP_LOGI(Tag, "ground thr: l=%d r=%d", groundThr[0], groundThr[1]);
  }

  if ((vmVariables.ground_delta[0] < groundThr[0]) ||
      (vmVariables.ground_delta[1] < groundThr[1]))
  {
    vmVariables.target[0] = 0;
    vmVariables.target[1] = 0;
    SetMotorTargets(vmVariables.target[0], vmVariables.target[1]);

    //Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, 0u, 0u);
    //Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, 0u, 0u);
    Leds_SetFrontLeftBrightness(red, green, blue);
    Leds_SetFrontRightBrightness(red, green, blue);
    Leds_SetBackLeftBrightness(red, green, blue);
    Leds_SetBackRightBrightness(red, green, blue);
    return 1;
  }
  else
  {
    // Yellow pulse
    Leds_SetBodyBrightness(red, green, blue);
    return 0;
  }
}

void Common_SetGroundThr(int16_t* values)
{
  groundThr[0] = values[0] + GROUND_EDGE_OFFSET;
  groundThr[1] = values[1] + GROUND_EDGE_OFFSET;
}

uint16_t Common_GetFirmwareVersionMajor(void)
{
  return FIRMWARE_VERSION_MAJOR;
}

uint16_t Common_GetFirmwareVersionMinor(void)
{
  return FIRMWARE_VERSION_MINOR;
}

uint16_t Common_GetFirmwareVersionPatch(void)
{
  return FIRMWARE_VERSION_PATCH;
}

void Common_GetFirmwareVersionString(char* versionString, size_t size)
{
  if (versionString == NULL || size == 0) {
        return;
  }
  snprintf(versionString, size, "%d.%d.%d", FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH);
}
