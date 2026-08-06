//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    serial_protocol.h
//! \brief   This module provides the protocol communication via the serial port
//!          in order to read or program the Thymio3's ID in the ESP32 eFuse.
//!
//!          That's just a wrapper for using the functions of efuse_id via the
//!          usb port of Thymio3. The protocol is based on JSON messages.
//!
//! \author  Daniel Burnier
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef SERIAL_PROTOCOL_H_
#define SERIAL_PROTOCOL_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------
/* Define of the protocol:

 * Request:
 * {
 *   "cmd": "get_id"
 * }
 * Response:
 * {
 *   "ok": true,
 *   "cmd": "get_id",
 *   "id": "AB12345"
 * }
 *
 * Request:
 * {
 *   "cmd": "set_id",
 *   "id": "@A12345"
 * }
 * Response:
 * {
 *   "ok": true,
 *   "cmd": "set_id",
 *   "result": 0
 * }
 *
 * Request:
 * {
 *   "cmd": "kill_id"
 * }
 * Response:
 * {
 *   "ok": true,
 *   "cmd": "kill_id",
 *   "result": 0
 * }
 *
 */

/**
 * @brief Get id of Thymio3.
 * * This function returns the id of the Thymio3 based on the actual entry in the
 * eFuse.
 * 
 * @return The id of the Thymio3 in the current entry
 * Else 0x00000000 if the first available entry is not used (no id programmed yet)
 * Else 0xFFFFFFFF if all entries are killed
 */
void serial_protocol_task(void *arg);

#endif // SERIAL_PROTOCOL_H_