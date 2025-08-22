//_____________________________________________________________________________
//
// Copyright (C) 2025                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ble_spp.h
//! \brief   This module provides the useful functions to use handle BLE connection
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef BLE_SPP_H_
#define BLE_SPP_H_

// Command characteristic
#define CMD_WRITE_MOST_ACTUATORS 0x01
// Circle LEDs => 4 bytes, brightness from 0..15
// Front lego LEDs => 4 bytes, brightness from 0..15
// Rear lego LEDs => 4 bytes, brightness from 0..15
// RGB front left => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB front right => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB back left => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB back right => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// Motor left => 2 bytes -1000..1000
// Motor right => 2 bytes -1000..1000
// Sound => 1 byte (
//    0 = magic,
//    1 = Tick,
//    2 = Blop,
//    3 = Fall,
//    4 = Detection,
//    5 = Bye,
//    6 = C3,
//    7 = D3,
//    8 = E3,
//    9 = F3,
//    10 = G3,
//    11 = A3,
//    12 = B3,
//    13 = Alarm,
//    14 = Good,
//    15 = Bad)
#define CMD_WRITE_MOST_ACTUATORS_LEN 26 // Including ID

#define CMD_WRITE_OTHERS_ACTUATORS 0x02
// RGB small bottom => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// RGB small back => 2 bytes (r bit0..3, g bit4..7, b bit8..11)
// Buttons LEDs => 2 bytes, brightness from 0..15
// Receiver LED + microphone LED => 1 byte (receiver bit0..3, microphone bit4)
#define CMD_WRITE_OTHERS_ACTUATORS_LEN 8 // Including ID

// Stream characteristic
#define STREAM_WRITE_STATE 0x01
#define STREAM_WRITE_STATE_LEN 2 // Including ID
#define STREAM_NOTIFY_MOST_SENSORS 0x01
// color sensor => 4 bytes => H (2), S (1), V (1)
// ground sensors => 4 bytes => left (2), right (2)
// acceleration raw => 6 bytes => x (2), y (2), z (2)
// gyro raw => 6 bytes => x (2), y (2), z (2)
// buttons => 1 byte
// microphone volume => 2 bytes
// proximity sensors => 14 bytes => left (2), front left (2), center (2), front right (2), right (2), back left (2), back right (2)
// tv remote => 1 byte
#define STREAM_NOTIFY_MOST_SENSORS_LEN 39 // Including ID

#define STREAM_NOTIFY_OTHERS_SENSORS 0x02
// color raw values => 8 bytes (red, green, blue, clear)
// color detected => 1 byte
// ground ambient => 4 bytes => left (2), right (2)
// ground reflected => 4 bytes => left (2), right (2)
// angle degrees => 2 bytes
// events flags => 1 byte => bit0 tap detected, bit1 freefall detected, bit2 clap detected
// motor left speed => 2 bytes
// motor right speed => 2 bytes
// motor left pwm duty => 2 bytes
// motor right pwm duty => 2 bytes
// battery voltage => 2 bytes
#define STREAM_NOTIFY_OTHERS_SENSORS_LEN 31 // Including ID

// Pyhton characteristic
#define PYTHON_WRITE_LOAD 0x01
#define PYTHON_WRITE_EXEC 0x02
#define PYTHON_WRITE_STOP 0x03
#define PYTHON_WRITE_SAVE 0x04
#define PYTHON_IND_LOAD_RES 0x01
#define PYTHON_IND_END_RES 0x02

// OTA definitions
#define OTA_RINGBUF_SIZE                    8192
#define OTA_TASK_SIZE                       8192
#define BUF_LENGTH                          4098
#define OTA_IDX_NB                          4
#define CMD_ACK_LENGTH                      20

void ble_spp_init(void);
void ble_notify_python_end(uint8_t value);

#endif // BLE_SPP_H_