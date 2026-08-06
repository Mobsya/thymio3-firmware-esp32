#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "uart.h"

#include "cJSON.h"

#include "esp_log.h"

#include "efuse_id.h"
#include "errno.h"
#include "stdlib.h"
#include "inttypes.h"

#define UART_PORT UART_NUM_0
#define RX_BUF_SIZE 512

static const char *TAG = "SERIAL_PROTO";

static void send_json(cJSON *json)
{
    static const char *header = "[JSON] ";
    static const char *footer = "\n";
    char *str = cJSON_PrintUnformatted(json);
    UART_Write((uint8_t*)header, strlen(header));
    UART_Write((uint8_t*)str, strlen(str));
    UART_Write((uint8_t*)footer, strlen(footer));
    free(str);
}

static uint32_t parse_uint32(cJSON *item) {
    if (!cJSON_IsString(item))
        return 0;
    char *end;
    errno = 0;
    unsigned long v = strtoul(item->valuestring, &end, 0);
    if (errno == ERANGE)          // Overflow for unsigned long
        return 0;
    if (end == item->valuestring) // No digits found
        return 0;
    if (*end != '\0')             // Extra characters after the number
        return 0;
    if (v > UINT32_MAX)           // Doesn't fit in uint32_t
        return 0;
    return (uint32_t)v;
}

static void handle_request(char *buffer)
{
    cJSON *req = cJSON_Parse(buffer);
    if (!req) {
        cJSON *err = cJSON_CreateObject();
        cJSON_AddBoolToObject(err, "ok", false);
        cJSON_AddStringToObject(err, "error", "invalid_json");
        send_json(err);
        cJSON_Delete(err);
        return;
    }
    cJSON *cmd = cJSON_GetObjectItem(req, "cmd");
    if (!cmd || !cJSON_IsString(cmd)) {
        cJSON_Delete(req);
        return;
    }
    cJSON *resp = cJSON_CreateObject();
    cJSON_AddBoolToObject(resp, "ok", true);
    cJSON_AddStringToObject(resp, "cmd", cmd->valuestring);
    //
    // GET ID
    //
    if(strcmp(cmd->valuestring, "get_id") == 0) {
        uint32_t id = getCurrentID();
        char id_str[16];
        snprintf(id_str, sizeof(id_str), "0x%08" PRIX32, id);

        cJSON_AddStringToObject(resp, "id", id_str);
    }
    //
    // SET ID
    //
    else if(strcmp(cmd->valuestring, "set_id") == 0) {
        cJSON *id = cJSON_GetObjectItem(req,"id");
        if(!id) {
            cJSON_AddBoolToObject(resp,"ok",false);
            cJSON_AddStringToObject(resp, "error", "missing id");
        }
        else {
            uint32_t new_id = parse_uint32(id);
            int8_t result = setCurrentID(new_id);
            cJSON_AddNumberToObject(resp, "result", result);
        }
    }
    //
    // KILL ID
    //
    else if(strcmp(cmd->valuestring, "kill_id") == 0) {
        int8_t result = killCurrentID();
        cJSON_AddNumberToObject(resp, "result", result);
    }
    //
    // AVAILABLE ENTRIES
    //
    else if(strcmp(cmd->valuestring, "available_entries") == 0) {
        uint8_t entries = getAvailableEntries();
        cJSON_AddNumberToObject(resp, "entries", entries);
    }
    else {
        cJSON_AddBoolToObject(resp,"ok",false);
        cJSON_AddStringToObject(resp, "error", "unknown command");
    }
    send_json(resp);
    cJSON_Delete(resp);
    cJSON_Delete(req);
}

#define BUF_SIZE (1024)

void serial_protocol_task(void *arg) {
    // uint8_t data;
    // char buffer[RX_BUF_SIZE];

    // Computer must send SYNCH in order to bypass REPL mode of the terminal

    const char *msg = "Serial protocol task started\r\n";
    UART_Write((uint8_t*)msg, strlen(msg));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

    int pos = 0;
    while(1) {
        int len = uart_read_bytes(UART_PORT, &data, 1, pdMS_TO_TICKS(100));
        if(len <= 0) continue;
        if(data == '\n') {
            buffer[pos] = 0;
            handle_request(buffer);
            pos = 0;
        }
        else if(pos < RX_BUF_SIZE-1) buffer[pos++] = data;
        uart_write_bytes(UART_PORT, msg, strlen(msg));
    }
}