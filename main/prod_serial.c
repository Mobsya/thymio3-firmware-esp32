//_____________________________________________________________________________
//
// Copyright (C) 2025                  GCtronic                  CH-6928 Manno
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    prod_serial.c
//! \brief   Binary protocol used on the production bench to read and burn the
//!          robot ID in the ESP32 eFuse BLK3. See prod_serial.h for the frame
//!          description.
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_system.h"

#include "prod_serial.h"
#include "efuse_id.h"
#include "leds.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define PROD_UART               UART_NUM_0
#define PROD_RX_BUF_SIZE        256     // Minimum accepted by the UART driver is 129

// How long the robot listens for the magic sequence at every boot.
//
// The host keeps the port open across the reset and streams the magic sequence
// from that moment on, so this only has to cover the jitter between the end of
// the reset and the moment this window opens. Every millisecond here is also
// paid on a normal boot, as a delay before MicroPython starts.
#define PROD_MAGIC_WINDOW_MS    1000

// After the READY frame has been sent, everything still coming in is discarded
// for this amount of time. This absorbs the magic sequences the host had
// already put on the wire when it saw the READY frame, so that they are not
// mistaken for commands.
#define PROD_SETTLE_MS          200

// Maximum time granted to the host to deliver the parameters of a command once
// its first byte has been received.
#define PROD_ARG_TIMEOUT_MS     500

// Red blink signalling production mode: 500 ms on, 500 ms off.
#define PROD_LED_HALF_PERIOD_MS 500
#define PROD_LED_TASK_STACK     2048
#define PROD_LED_TASK_PRIO      1

// First byte of the magic sequence, recognised again inside the command loop.
#define PROD_MAGIC_LEAD         0xA5

static const uint8_t prod_magic[] = { 0xA5, 'T', '3', 'P', 'R', 'O', 'D', 0x5A };
static const uint8_t prod_ready[] = { 0xA5, 'T', '3', 'R', 'D', 'Y', PROD_PROTO_VERSION, 0x5A };

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *TAG = "PROD_SERIAL";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void prod_led_set_red(bool on);
static void prod_led_task(void *arg);
static void prod_led_start(void);
static bool prod_wait_magic(void);
static void prod_drain(uint32_t duration_ms);
static bool prod_read_exact(uint8_t *buf, size_t len, uint32_t timeout_ms);
static void prod_send(const uint8_t *buf, size_t len);
static void prod_announce_ready(void);
static void prod_command_loop(void);

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

bool ProdSerial_WaitAndRun(void)
{
    bool driver_owned = false;
    bool already_installed = uart_is_driver_installed(PROD_UART);

    if (!already_installed) {
        if (uart_driver_install(PROD_UART, PROD_RX_BUF_SIZE, 0, 0, NULL, 0) != ESP_OK) {
            return false;
        }
        driver_owned = true;
    }

    if (!prod_wait_magic()) {
        // Normal boot: hand the port back to MicroPython in a clean state.
        if (driver_owned) {
            uart_driver_delete(PROD_UART);
        }
        return false;
    }

    //Leds_SetFrontLeftBrightness(MAX_BRIGHTNESS, 0, 0);

    // From here on UART0 carries a binary stream: a single log line would
    // corrupt it, so logging is silenced whatever the level set by app_main().
    //esp_log_level_set("*", ESP_LOG_NONE);

    // Visual feedback: the robot blinks red as long as it stays in production
    // mode, so an operator can tell at a glance that it is not a normal boot.
    prod_led_start();

    prod_announce_ready();

    //Leds_SetFrontLeftBrightness(0, MAX_BRIGHTNESS, 0);

    prod_command_loop();    // Never returns

    Leds_SetFrontLeftBrightness(0, 0, MAX_BRIGHTNESS);

    return true;
}

//_____________________________________________________________________________

/**
 * @brief Drive the whole RGB ring in red, or switch it off.
 *
 * This is the only place bound to the Leds module: adapt the two calls below
 * if the LED API changes.
 */
static void prod_led_set_red(bool on)
{
    Leds_SetBodyBrightness(on ? MAX_BRIGHTNESS : 0, 0, 0);
}

//_____________________________________________________________________________

/**
 * @brief Blink red for as long as the robot stays in production mode.
 */
static void prod_led_task(void *arg)
{
    bool on = false;

    (void)arg;

    for (;;) {
        on = !on;
        prod_led_set_red(on);
        vTaskDelay(pdMS_TO_TICKS(PROD_LED_HALF_PERIOD_MS));
    }
}

//_____________________________________________________________________________

static void prod_led_start(void)
{
    xTaskCreate(prod_led_task, "prod_led", PROD_LED_TASK_STACK, NULL,
                PROD_LED_TASK_PRIO, NULL);
}

//_____________________________________________________________________________


/**
 * @brief Look for the magic sequence in the incoming stream.
 *
 * The matching is done byte per byte on a sliding position because the host
 * streams the sequence continuously and the listening window may well open in
 * the middle of one of them.
 *
 * @return true if the whole sequence has been recognised before the window
 *         expired.
 */
static bool prod_wait_magic(void)
{
    size_t matched = 0;
    uint8_t c;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(PROD_MAGIC_WINDOW_MS);
    //Leds_SetLegoProgress(0);

    while ((int32_t)(xTaskGetTickCount() - deadline) < 0) {
        if (uart_read_bytes(PROD_UART, &c, 1, pdMS_TO_TICKS(10)) != 1) {
            continue;
        }
        if (c == prod_magic[matched]) {
            matched++;
            if (matched == sizeof(prod_magic)) {
                //Leds_SetLegoProgress(matched);
                return true;
            }
        } else {
            // Restart the match from the byte just received.
            matched = (c == prod_magic[0]) ? 1 : 0;
        }
    }
    //Leds_SetLegoProgress(matched);
    return false;
}

//_____________________________________________________________________________

/**
 * @brief Discard everything received during the given amount of time.
 */
static void prod_drain(uint32_t duration_ms)
{
    uint8_t scratch[32];
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(duration_ms);

    while ((int32_t)(xTaskGetTickCount() - deadline) < 0) {
        uart_read_bytes(PROD_UART, scratch, sizeof(scratch), pdMS_TO_TICKS(10));
    }
    uart_flush_input(PROD_UART);
}

//_____________________________________________________________________________

/**
 * @brief Read exactly len bytes or give up.
 *
 * @return true if all the requested bytes have been received in time. On false
 *         the bytes already read are dropped and the caller resynchronises on
 *         the next command byte.
 */
static bool prod_read_exact(uint8_t *buf, size_t len, uint32_t timeout_ms)
{
    size_t received = 0;
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (received < len) {
        TickType_t now = xTaskGetTickCount();
        if ((int32_t)(now - deadline) >= 0) {
            return false;
        }
        int len_read = uart_read_bytes(PROD_UART, &buf[received], len - received, deadline - now);
        if (len_read > 0) {
            received += (size_t)len_read;
        }
    }

    return true;
}

//_____________________________________________________________________________

static void prod_send(const uint8_t *buf, size_t len)
{
    uart_write_bytes(PROD_UART, (const char *)buf, len);
    uart_wait_tx_done(PROD_UART, pdMS_TO_TICKS(100));
}

//_____________________________________________________________________________

/**
 * @brief Tell the host that production mode is active.
 *
 * Everything received while the host was still streaming the magic sequence is
 * then discarded, so that those bytes are not mistaken for commands.
 */
static void prod_announce_ready(void)
{
    //Leds_SetFrontRightBrightness(MAX_BRIGHTNESS, 0, 0);
    prod_send(prod_ready, sizeof(prod_ready));
    //Leds_SetFrontRightBrightness(0, MAX_BRIGHTNESS, 0);
    prod_drain(PROD_SETTLE_MS);
    //Leds_SetFrontRightBrightness(0, 0, MAX_BRIGHTNESS);
}

//_____________________________________________________________________________

/**
 * @brief Serve the production commands until the robot is reset or rebooted.
 */
static void prod_command_loop(void)
{
    uint8_t cmd;
    uint8_t arg[4];
    uint8_t resp[5];

    for (;;) {
        if (uart_read_bytes(PROD_UART, &cmd, 1, portMAX_DELAY) != 1) {
            continue;
        }

        switch (cmd) {

        case PROD_MAGIC_LEAD: {
            // The host streams the magic sequence until it sees a READY frame.
            // Getting a whole sequence here means that the first announcement
            // was missed, or that the host started while the robot was already
            // in production mode because the reset over DTR/RTS did not take
            // effect. Announce again instead of rejecting the bytes, so that
            // the host always converges without needing a working reset line.
            uint8_t tail[sizeof(prod_magic) - 1];
            if (prod_read_exact(tail, sizeof(tail), PROD_ARG_TIMEOUT_MS) &&
                memcmp(tail, &prod_magic[1], sizeof(tail)) == 0) {
                prod_announce_ready();
            } else {
                resp[0] = PROD_RESP_UNKNOWN;
                resp[1] = PROD_MAGIC_LEAD;
                prod_send(resp, 2);
            }
            break;
        }

        case PROD_CMD_GET_ID: {
            uint32_t id = getCurrentID();
            resp[0] = PROD_CMD_GET_ID;
            resp[1] = (uint8_t)(id >> 24);
            resp[2] = (uint8_t)(id >> 16);
            resp[3] = (uint8_t)(id >> 8);
            resp[4] = (uint8_t)(id);
            prod_send(resp, 5);
            break;
        }

        case PROD_CMD_SET_ID: {
            if (!prod_read_exact(arg, sizeof(arg), PROD_ARG_TIMEOUT_MS)) {
                resp[0] = PROD_RESP_UNKNOWN;
                resp[1] = PROD_CMD_SET_ID;
                prod_send(resp, 2);
                break;
            }
            uint32_t new_id = ((uint32_t)arg[0] << 24) |
                              ((uint32_t)arg[1] << 16) |
                              ((uint32_t)arg[2] << 8)  |
                              ((uint32_t)arg[3]);
            id_error_t status = setCurrentID(new_id);
            resp[0] = PROD_CMD_SET_ID;
            resp[1] = (uint8_t)(int8_t)status;
            prod_send(resp, 2);
            break;
        }

        case PROD_CMD_KILL_ID: {
            int8_t status = killCurrentID();
            resp[0] = PROD_CMD_KILL_ID;
            resp[1] = (uint8_t)status;
            prod_send(resp, 2);
            break;
        }

        case PROD_CMD_GET_ENTRIES: {
            uint8_t entries = getAvailableEntries();
            resp[0] = PROD_CMD_GET_ENTRIES;
            resp[1] = entries;
            prod_send(resp, 2);
            break;
        }

        case PROD_CMD_REBOOT: {
            resp[0] = PROD_CMD_REBOOT;
            resp[1] = 0;
            prod_send(resp, 2);
            vTaskDelay(pdMS_TO_TICKS(50));  // Let the response leave the FIFO
            esp_restart();
            break;
        }

        default:
            ESP_LOGW(TAG, "Unknown command 0x%02X", cmd);
            resp[0] = PROD_RESP_UNKNOWN;
            resp[1] = cmd;
            prod_send(resp, 2);
            break;
        }
    }
}

//_____________________________________________________________________________
