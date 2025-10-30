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
#define PYTHON_IND_EXEC_RES 0x02
#define PYTHON_IND_SAVE_RES 0x05

#define PYTHON_EXEC_OK 0
#define PYTHON_EXEC_ERROR 1
#define PYTHON_EXEC_ALREADY_RUNNING 2
#define PYTHON_EXEC_NOT_FOUND 3

#define PYTHON_SAVE_OK 0
#define PYTHON_SAVE_NOT_FOUND 1
#define PYTHON_SAVE_ERROR 2

// OTA definitions
#define OTA_RINGBUF_SIZE                    8192
#define OTA_TASK_SIZE                       8192
#define BUF_LENGTH                          4098
#define OTA_IDX_NB                          4
#define CMD_ACK_LENGTH                      20

// Audio characteristic
#define AUDIO_WRITE_LOAD 0x01
#define AUDIO_WRITE_EXEC 0x02
#define AUDIO_WRITE_STOP 0x03
#define AUDIO_WRITE_SAVE 0x04
#define AUDIO_WRITE_REC 0x05
#define AUDIO_WRITE_TONE 0x06
#define AUDIO_IND_LOAD_RES 0x01
#define AUDIO_IND_EXEC_RES 0x02
#define AUDIO_IND_REC_RES 0x03
#define AUDIO_READ_DOWNLOAD 0x01
#define AUDIO_WRITE_EXEC_LEN 21 // ID + audio name

#define AUDIO_EXEC_OK 0
#define AUDIO_EXEC_ERROR 1
#define AUDIO_EXEC_NOT_FOUND 2
#define AUDIO_EXEC_NOT_SUPPORTED 3

#define AUDIO_REC_OK 0
#define AUDIO_REC_ERROR 1
#define AUDIO_REC_TOO_LONG 2

// Device info characteristic
#define DEV_INFO_WRITE_FIRMWARE 0x01
#define DEV_INFO_WRITE_MEMORY 0x02
#define DEV_INFO_IND_FIRMWARE_RES 0x01
#define DEV_INFO_IND_MEMORY_RES 0x02

// File system characteristic
#define FS_WRITE_LOAD 0x01
#define FS_WRITE_SAVE 0x02
#define FS_WRITE_DELETE 0x03
#define FS_WRITE_LIST 0x04
#define FS_WRITE_ERASE_ALL 0x05
#define FS_IND_LOAD_RES 0x01
#define FS_IND_SAVE_RES 0x02
#define FS_IND_DELETE_RES 0x03
#define FS_IND_LIST_RES 0x04
#define FS_IND_LIST_ERROR 0x05
#define FS_IND_ERASE_ALL_RES 0x06

#define FS_LOAD_OK 0
#define FS_LOAD_CRC_ERR 1
#define FS_LOAD_NOT_COMPLETE 2
#define FS_LOAD_WRONG_SEQ 3
#define FS_LOAD_TOO_BIG 4

#define FS_SAVE_OK 0
#define FS_SAVE_NOT_FOUND 1
#define FS_SAVE_NO_SPACE 2
#define FS_SAVE_ERROR 3

#define FS_DELETE_OK 0
#define FS_DELETE_NOT_FOUND 1
#define FS_DELETE_ERROR 2

#define FS_ERASE_ALL_OK 0
#define FS_ERASE_ALL_ERROR 1

void ble_spp_init(void);
void ble_indicate_python_load(uint8_t value);
void ble_indicate_python_exec(uint8_t value);
void ble_indicate_python_save(uint8_t value);
void ble_indicate_audio_load(uint8_t value);
void ble_indicate_audio_exec(uint8_t value);
void ble_indicate_audio_rec(uint8_t value);
void ble_indicate_fs(uint8_t type, uint8_t value);
void ble_indicate_fs_list(uint8_t *data, uint16_t len);
void ble_indicate_fs_list_err(void);
void ble_indicate_dev_info_mem(uint8_t *data, uint16_t len);

#endif // BLE_SPP_H_