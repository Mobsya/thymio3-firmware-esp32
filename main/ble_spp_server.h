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

/* 16 Bit SPP Service UUID */
#define BLE_SVC_SPP_UUID16                                  0xABF0

/* 16 Bit SPP Service RX Characteristic UUID */
#define BLE_SVC_SPP_RX_CHR_UUID16                              0xABF1

/* 16 Bit SPP Service TX Characteristic UUID */
#define BLE_SVC_SPP_TX_CHR_UUID16                              0xABF2

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

#ifdef __cplusplus
}
#endif

#endif
