//_____________________________________________________________________________
//
// Copyright (C) 2025                  GCtronic                  CH-6928 Manno
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    prod_serial.h
//! \brief   Production mode entered at boot through a magic sequence received
//!          on the console UART, providing a binary protocol to read and burn
//!          the robot ID stored in the ESP32 eFuse BLK3.
//!
//!          BOOT SEQUENCE
//!          -------------
//!          ProdSerial_WaitAndRun() must be called from app_main() BEFORE
//!          init_micropython(). It listens on UART0 for a short window
//!          (PROD_MAGIC_WINDOW_MS). If the magic sequence is not received the
//!          function releases the UART and returns false, and the boot goes on
//!          as usual. If the magic sequence is received the function sends a
//!          READY frame and never returns: MicroPython, the BLE stack and the
//!          intro sound are therefore never started, so the robot stays in a
//!          known state while its ID is being programmed. The robot then
//!          blinks red (500 ms on, 500 ms off) until it is reset or rebooted,
//!          so that production mode is visible on the bench.
//!
//!          The host is expected to reset the robot (DTR/RTS auto-reset
//!          circuit or reset button) and then to stream the magic sequence
//!          repeatedly until the READY frame is received.
//!
//!          FRAMES
//!          ------
//!          magic (host -> robot) : A5 54 33 50 52 4F 44 5A   ("\xA5T3PROD\x5A")
//!          ready (robot -> host) : A5 54 33 52 44 59 vv 5A   ("\xA5T3RDY" + version + "\x5A")
//!
//!          COMMANDS (host -> robot)   RESPONSES (robot -> host)
//!          ------------------------   -------------------------
//!          00                    (1)  00 id31..id0            (5)
//!          01 id31..id0          (5)  01 status               (2)
//!          02                    (1)  02 status               (2)
//!          03                    (1)  03 entries              (2)
//!          7F                    (1)  7F 00                   (2)  then reboot
//!          any other             (1)  FF cmd                  (2)
//!
//!          The 32 bits ID travels MSB first (big endian). The status byte of
//!          the SET_ID response holds an id_error_t value casted to int8_t
//!          (two's complement), the one of the KILL_ID response holds 0 on
//!          success and -1 (0xFF) on failure.
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef PROD_SERIAL_H_
#define PROD_SERIAL_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PROD_PROTO_VERSION      0x01

// Command identifiers, also echoed back as first byte of every response.
#define PROD_CMD_GET_ID         0x00
#define PROD_CMD_SET_ID         0x01
#define PROD_CMD_KILL_ID        0x02
#define PROD_CMD_GET_ENTRIES    0x03
#define PROD_CMD_REBOOT         0x7F    // Optional, leaves production mode
#define PROD_RESP_UNKNOWN       0xFF    // Followed by the offending command byte

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

/**
 * @brief Listen on the console UART for the production magic sequence.
 *
 * Must be called before any other module takes ownership of UART0 for the
 * REPL (i.e. before init_micropython()).
 *
 * @return false if the magic sequence did not arrive within the listening
 *         window, meaning the caller must go on with the normal boot.
 *         The function never returns true: once production mode is entered it
 *         loops forever serving commands (the return value only exists to keep
 *         the call site readable).
 */
bool ProdSerial_WaitAndRun(void);

#endif // PROD_SERIAL_H_
