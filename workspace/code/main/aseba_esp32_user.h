//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    aseba_esp32_user.h
//! \brief   This module provides the useful functions to use the Aseba ESP32 user features
//!
//! \author  Vincent Gonet
//!
//! \version $Id: aseba_esp32_user.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef ASEBA_ESP32_USER_H_
#define ASEBA_ESP32_USER_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include "stdint.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define FLASH_END             0x15800

#define PRIO_COMMUNICATION          4

#define PRODUCT_ID                  8

// Send queue minimum size: 512+6+4+1
#define SEND_QUEUE_SIZE      (512+6+4+1)

// Recv queue minimum size: 512+6+4+1+USB_MTU
#define RECV_QUEUE_SIZE      (512+6+4+1+64)

// This is the number of "private" variable the Aseba script can have
#define VM_VARIABLES_FREE_SPACE   512

// This is the maximum number of argument an Aseba event can receive */
#define VM_VARIABLES_ARG_SIZE      32

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct _vmVariables
{
  int16_t id;  // NodeID
  int16_t source; // source
  int16_t args[VM_VARIABLES_ARG_SIZE];  // args
  int16_t fwversion[2];   // fwversion
  int16_t productid;  // Product ID
  int16_t buttons[5];
  int16_t buttons_state[5];
  int16_t buttons_mean[5];
  int16_t buttons_noise[5];
  int16_t prox[7];
  int16_t sensor_data[7];
  int16_t intensity[7];
  int16_t rx_data;
  int16_t ir_tx_data;
  int16_t ground_ambiant[2];
  int16_t ground_reflected[2];
  int16_t ground_delta[2];
  int16_t target[2];
  int16_t vbat[2];
  int16_t imot[2];
  int16_t uind[2];
  int16_t pwm[2];
  int16_t acc[3];
  int16_t ntc;
  int16_t rc5_address;
  int16_t rc5_command;
  int16_t sound_level;
  int16_t sound_tresh;
  int16_t sound_mean;
  int16_t timers[2];
  int16_t acc_tap;
  int16_t sd_present;
  /*****
    ---> PUT YOUR VARIABLES HERE <---
  ******/
  int16_t color[4];
  int16_t gyro[3];
  int16_t ip[4];
  int16_t settings[2];
  int16_t freeSpace[VM_VARIABLES_FREE_SPACE];
};

enum Event
{
  EVENT_B_BACKWARD = 0,
  EVENT_B_LEFT,
  EVENT_B_CENTER,
  EVENT_B_FORWARD,
  EVENT_B_RIGHT,
  EVENT_BUTTONS,
  EVENT_PROX,
  EVENT_DATA,
  EVENT_TAP,
  EVENT_ACC,
  EVENT_GYRO,
  EVENT_MIC,
  EVENT_SOUND_FINISHED,
  EVENT_TEMPERATURE,
  EVENT_RC5,
  EVENT_MOTOR,
  EVENT_COLOR,
  // Must be consecutive
  EVENT_TIMER0,
  EVENT_TIMER1,
  // Maximum count: 32
  EVENT_COUNT // Do not touch
};

// The content of this structure is implementation-specific
// The glue provide a way to store and retrive it from flash.
// The only way to write it is to do it from inside the VM (Native function)
// The native function access it as a integer array. So, use only int inside this structure
struct private_settings
{
  /* ADD here the settings to save into flash */
  /* The minimum size is one integer, the maximum size is 95 integer (check done at compilation) */
  int sound_shift;
  int prox_min[7];
  int mot256[2];
  int prox_ground_max[2];
  int settings[80];
};

// Persistent data used by the thymio device manager,
// saved and retrieved with thymio-specific protocol messages from version 12+
struct thymio_device_info
{
  /* The minimum size is one integer, the maximum size is 95 integer (check done at compilation) */
  /* a uid is 128 bits = 16 bytes */
  unsigned char uuid[16];
  /* A long name so we can put reasonably long utf8 name in it, first reserved for the size*/
  unsigned char friendly_name[56];
};

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

#endif // ASEBA_ESP32_USER_H_
