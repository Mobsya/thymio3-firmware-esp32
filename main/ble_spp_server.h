/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#ifndef H_BLESPPSERVER_
#define H_BLESPPSERVER_

#include <stdbool.h>
#include "nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 16 Bit Thymio Service UUID */
#define BLE_SVC_THYMIO_UUID16                                  0xABF0

/* 16 Bit SPP Service commands Characteristic UUID */
#define BLE_SVC_CMD_CHR_UUID16                              0xABF1

/* 16 Bit SPP Service sensors stream Characteristic UUID */
#define BLE_SVC_SENSORS_STREAM_CHR_UUID16                   0xABF2

/* 16 Bit SPP Service python scripts Characteristic UUID */
#define BLE_SVC_PYTHON_CHR_UUID16                           0xABF3

/* 16 Bit SPP Service audio Characteristic UUID */
#define BLE_SVC_AUDIO_CHR_UUID16                           0xABF4

/* 16 Bit SPP Service device info Characteristic UUID */
#define BLE_SVC_DEVICE_INFO_CHR_UUID16                      0xABF5

/* 16 Bit SPP Service file system Characteristic UUID */
#define BLE_SVC_FILE_SYSTEM_CHR_UUID16                      0xABF6

#define BLE_OTA_SERVICE_UUID                0x8018
#define RECV_FW_UUID                        0x8020
#define OTA_BAR_UUID                        0x8021
#define COMMAND_UUID                        0x8022
#define CUSTOMER_UUID                       0x8023

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

#ifdef __cplusplus
}
#endif

#endif
