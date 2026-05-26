//_____________________________________________________________________________
//
// Copyright (C) 2025                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ble_spp.c
//! \brief   This module provides the useful functions to use handle BLE connection
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________



#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_timer.h"
#include "soc/efuse_reg.h"
/* BLE */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
#include "esp_ota_ops.h"
#include "ble_ota.h"
#include "ble_spp_server.h"
#include "ble_spp.h"
#include "esp_mac.h" 
#include "services/gatt/ble_svc_gatt.h"
#include "leds.h"
#include "stm32_spi.h"
#include "color_sensor.h"
#include "accelerometer.h"
#include "gyroscope.h"
#include "rc5.h"
#include "mode.h"
#include "utility.h"
#include "buttons.h"
#include "mp_component.h"
#include "esp_rom_crc.h"
#include "aseba_esp32.h"
#include "codec.h"
#include "behavior.h"

static const char *TAG = "THYMIO_BLUETOOTH";

#define MAX_BT_RX_BUFF (CMD_WRITE_MOST_ACTUATORS_LEN)
#define MAX_BT_TX_BUFF (STREAM_NOTIFY_MOST_SENSORS_LEN)

// Python characteristic definitions
#define FILE_LOAD_OK 0
#define FILE_LOAD_CRC_ERR 1
#define FILE_LOAD_NOT_COMPLETE 2
#define FILE_LOAD_WRONG_SEQ 3
#define FILE_LOAD_TOO_BIG 4

// Audio characteristic definitions
#define AUDIO_PLAY_UNDEF 0
#define AUDIO_PLAY_LOADED 1
#define AUDIO_PLAY_RECORDED 2

// File system characteristic definitions
#define FILENAME_MAX_LEN 30
#define FS_MAX_TOTAL_CHUNK_SIZE 500 // Maximum total size of each BLE indication chunk (including header)
#define FS_LIST_FIRST_PACKET_HEADER_LEN 9 //First Packet Header: ID(1) + size(2) + CRC32(4) + seq_id(2) = 9 bytes
#define FS_SUBSEQUENT_PACKET_HEADER_LEN 2 // Subsequent Packet Header: seq_id(2) = 2 bytes
#define FS_DOWNLOAD_FIRST_PACKET_HEADER_LEN 11 //First Packet Header: ID(1) + size(4) + CRC32(4) + seq_id(2) = 9 bytes

// Micropython stdout definitions
#define STDOUT_RINGBUF_SIZE 2048
#define STDOUT_MAX_CHUNK_SIZE 240
#define STDOUT_ACCUM_BUF_SIZE 512 // Define a size for the temporary/accumulation buffer. This should be larger than the 100-byte threshold.
// Define the send conditions: stdout buffer sent when either it is filled with at least 100 bytes or no more data arrived within 1 second
#define STDOUT_SEND_THRESHOLD_BYTES 100
#define STDOUT_SEND_TIMEOUT_MS 1000


EXT_RAM_ATTR uint8_t bt_rx_data[MAX_BT_RX_BUFF]; // Received commands from the device (e.g. from phone)
EXT_RAM_ATTR uint8_t bt_rx_data_temp[MAX_BT_RX_BUFF]; // Double buffer for parsing the data while receiving new data without corruption
EXT_RAM_ATTR uint8_t bt_tx_data[MAX_BT_TX_BUFF]; // Data sent to the device (e.g. to phone)
bool bt_cmd_received = false;
uint8_t bt_cmd_len = 0;
bool bt_most_sensors_stream_en = false;
bool bt_others_sensors_stream_en = false;
EXT_RAM_ATTR static uint8_t rx_buff_temp[500];
#define INDICATION_ACK_BIT (1 << 0)
#define INDICATION_ERR_BIT (1 << 1)
static EventGroupHandle_t ind_ack_event_group = NULL;

static int ble_spp_server_gap_event(struct ble_gap_event *event, void *arg);
static uint8_t own_addr_type;
static bool is_connect = false;
uint16_t connection_handle;
static uint16_t ble_sensors_stream_val_handle;

// Python variables
static uint16_t ble_python_val_handle;
EXT_RAM_ATTR char mp_script[MAX_MP_SCRIPT_LEN] = {0};
uint16_t mp_script_tot_len = 0;
uint16_t mp_script_curr_len = 0;
bool mp_receiving_script = false;
uint32_t mp_script_crc = 0;
uint16_t mp_script_seq_id = 0;
uint16_t mp_script_seq_id_prev = 0;
uint16_t mp_script_timeout = 0;
bool mp_script_ready = false;

// Audio variables
static uint16_t ble_audio_val_handle;
uint8_t *audio_data = NULL;
char audio_name[21]; // Max is 20 bytes + null terminator
uint32_t audio_tot_len = 0;
uint32_t audio_curr_len = 0;
bool receiving_audio = false;
uint32_t audio_crc = 0;
uint16_t audio_seq_id = 0;
uint16_t audio_seq_id_prev = 0;
uint16_t audio_timeout = 0;
bool audio_ready = false;
uint8_t audio_play_type = AUDIO_PLAY_UNDEF;
uint32_t sampleRate = 0;
uint16_t numChannels = 0;
uint16_t bitsPerChannel = 0;
uint16_t mp3Ver = 0;
uint32_t id3Size = 0;
bool audio_playing = false;
bool audio_recording = false;

// OTA variables
static bool counter = false;
static uint16_t ota_handle_table[OTA_IDX_NB];
static uint16_t attribute_handle;
static uint16_t receive_fw_val;
static uint16_t ota_status_val;
static uint16_t command_val;
static uint16_t custom_val;
static bool start_ota = false;
static uint32_t cur_sector = 0;
static uint32_t cur_packet = 0;
static uint8_t *fw_buf = NULL;
static uint32_t fw_buf_offset = 0;
static uint32_t ota_total_len = 0;
static uint32_t ota_block_size = BUF_LENGTH;
esp_ble_ota_notification_check_t ota_notification = {
    .recv_fw_ntf_enable = false,
    .process_bar_ntf_enable = false,
    .command_ntf_enable = false,
    .customer_ntf_enable = false,
};
static RingbufHandle_t s_ringbuf = NULL;
SemaphoreHandle_t notify_sem;
static esp_ota_handle_t out_handle;

// File system variables
static uint16_t ble_fs_val_handle;
static uint8_t *fs_data = NULL;
char fs_filename[FILENAME_MAX_LEN] = {0};
static uint32_t fs_tot_len = 0;
static uint32_t fs_curr_len = 0;
static bool fs_receiving_file = false;
static uint32_t fs_crc = 0;
static uint16_t fs_seq_id = 0;
static uint16_t fs_seq_id_prev = 0;
static uint16_t fs_timeout = 0;
static bool fs_file_ready = false;

// Device info variables
static uint16_t ble_dev_info_val_handle;

// Micropython stdout variables
static RingbufHandle_t s_stdout_ringbuf = NULL;
static uint16_t ble_python_stdout_val_handle;

// Initial CRC value
// Note: ESP32's ROM functions for CRC require some bitwise manipulation to match standard CRC32 implementations.
// The initial value is usually 0xFFFFFFFF for CRC-32/ISO-HDLC.
// For the ESP32 ROM function, you need to bitwise NOT the initial value.
static uint32_t initial_crc = ~(0xFFFFFFFF);
uint32_t calculated_crc = 0;


void ble_store_config_init(void);
static uint16_t crc16_ccitt(const unsigned char *buf, int len);
static esp_ble_ota_char_t find_ota_char_and_desr_by_handle(uint16_t handle);
static int esp_ble_ota_notification_data(uint16_t conn_handle, uint16_t attr_handle, uint8_t cmd_ack[], esp_ble_ota_char_t ota_char);
size_t write_to_ringbuf(const uint8_t *data, size_t size);

/*
 * This is a workaround for the missing os_mbuf_len function in NimBLE.
 * It is not present in NimBLE 1.3, but is present in NimBLE 1.4.
 * This function is used to get the length of an os_mbuf.
 */
uint16_t os_mbuf_len(const struct os_mbuf *om)
{
    uint16_t len;

    len = 0;
    while (om != NULL) {
        len += om->om_len;
        om = SLIST_NEXT(om, om_next);
    }

    return len;
}

static void esp_ble_ota_fill_handle_table(void)
{
    ota_handle_table[RECV_FW_CHAR] = receive_fw_val;
    ota_handle_table[OTA_STATUS_CHAR] = ota_status_val;
    ota_handle_table[CMD_CHAR] = command_val;
    ota_handle_table[CUS_CHAR] = custom_val;
}

/**
 * Logs information about a connection to the console.
 */
static void ble_spp_server_print_conn_desc(struct ble_gap_conn_desc *desc)
{
    MODLOG_DFLT(INFO, "handle=%d our_ota_addr_type=%d our_ota_addr=",
                desc->conn_handle, desc->our_ota_addr.type);
    //print_addr(desc->our_ota_addr.val);
    MODLOG_DFLT(INFO, " our_id_addr_type=%d our_id_addr=",
                desc->our_id_addr.type);
    //print_addr(desc->our_id_addr.val);
    MODLOG_DFLT(INFO, " peer_ota_addr_type=%d peer_ota_addr=",
                desc->peer_ota_addr.type);
    //print_addr(desc->peer_ota_addr.val);
    MODLOG_DFLT(INFO, " peer_id_addr_type=%d peer_id_addr=",
                desc->peer_id_addr.type);
    //print_addr(desc->peer_id_addr.val);
    MODLOG_DFLT(INFO, " conn_itvl=%d conn_latency=%d supervision_timeout=%d "
                "encrypted=%d authenticated=%d bonded=%d\n",
                desc->conn_itvl, desc->conn_latency,
                desc->supervision_timeout,
                desc->sec_state.encrypted,
                desc->sec_state.authenticated,
                desc->sec_state.bonded);
}

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void ble_spp_server_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    const char *name;
    int rc;

    /**
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info).
     *     o Advertising tx power.
     *     o Device name.
     *     o 16-bit service UUIDs (alert notifications).
     */

    memset(&fields, 0, sizeof fields);

    /* Advertise two flags:
    *     o Discoverability in forthcoming advertisement (general)
    *     o BLE-only (BR/EDR unsupported).
    */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                BLE_HS_ADV_F_BREDR_UNSUP;

    /* Indicate that the TX power level field should be included; have the
    * stack fill this value automatically.  This is done by assigning the
    * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
    */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    fields.uuids16 = (ble_uuid16_t[]) {
        BLE_UUID16_INIT(BLE_SVC_THYMIO_UUID16),
        BLE_UUID16_INIT(BLE_OTA_SERVICE_UUID)        
    };
    fields.num_uuids16 = 2;
    fields.uuids16_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising. */
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(own_addr_type, NULL, BLE_HS_FOREVER,
                        &adv_params, ble_spp_server_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * ble_spp_server uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  ble_spp_server.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int
ble_spp_server_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;
    esp_ble_ota_char_t ota_char;

    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
	/* A new connection was established or a connection attempt failed. */
        MODLOG_DFLT(INFO, "connection %s; status=%d ",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);
        if (event->connect.status == 0) {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(rc == 0);
            ble_spp_server_print_conn_desc(&desc);
	        is_connect=true;
	        connection_handle = event->connect.conn_handle;

            // Define the connection parameters to avoid disconnection when calling "esp_ota_begin".
            struct ble_gap_upd_params params = {
                .itvl_min = 10,         // Minimum connection interval: 10 * 1.25ms = 12.5ms
                .itvl_max = 40,         // Maximum connection interval: 40 * 1.25ms = 50ms
                .latency = 0,           // Slave latency: 0
                .supervision_timeout = 800,  // Supervision timeout: 800 * 10ms = 8 seconds needed because esp_ota_begin takes about 5 seconds.
                .max_ce_len = 0,
                .min_ce_len = 0
            };
            ble_gap_update_params(event->connect.conn_handle, &params);  
        }
        MODLOG_DFLT(INFO, "\n");
        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            ble_spp_server_advertise();
        }
        enter_micropython_mode();
        esp_ble_ota_fill_handle_table();
        Codec_PlayOnboardSound(TONE_TYPE_BLUETOOTH_CONNEXION);
        Leds_SetDebugBrightness(0, 0, MAX_BRIGHTNESS);
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        ble_spp_server_print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        mp_receiving_script = false;

        /* Connection terminated; resume advertising. */
        ble_spp_server_advertise();
        turnOffAllSensors();
        exit_micropython_mode();
        if(start_ota)
        {
            start_ota = false;
        }
        is_connect = false;
        xEventGroupSetBits(ind_ack_event_group, INDICATION_ERR_BIT);
        Leds_SetDebugBrightness(0, 0, 0);
        return 0;

    case BLE_GAP_EVENT_CONN_UPDATE:
        /* The central has updated the connection parameters. */
        MODLOG_DFLT(INFO, "connection updated; status=%d ",
                    event->conn_update.status);
        rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
        assert(rc == 0);
        ble_spp_server_print_conn_desc(&desc);
        MODLOG_DFLT(INFO, "\n");
        return 0;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        MODLOG_DFLT(INFO, "advertise complete; reason=%d",
                    event->adv_complete.reason);
        ble_spp_server_advertise();
        return 0;

    case BLE_GAP_EVENT_SUBSCRIBE:
        ota_char = find_ota_char_and_desr_by_handle(event->subscribe.attr_handle);
        ESP_LOGI(TAG, "client subscribe ble_gap_event, ota_char: %d", ota_char);

        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d "
                 "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                 event->subscribe.conn_handle,
                 event->subscribe.attr_handle,
                 event->subscribe.reason,
                 event->subscribe.prev_notify,
                 event->subscribe.cur_notify,
                 event->subscribe.prev_indicate,
                 event->subscribe.cur_indicate);

        switch (ota_char) {
            case RECV_FW_CHAR:
                ota_notification.recv_fw_ntf_enable = true;
                break;
            case OTA_STATUS_CHAR:
                ota_notification.process_bar_ntf_enable = true;
                break;
            case CMD_CHAR:
                ota_notification.command_ntf_enable = true;
                break;
            case CUS_CHAR:
                ota_notification.customer_ntf_enable = true;
                break;
            case INVALID_CHAR:
                break;
        }
        return 0;

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
        return 0;

    case BLE_GAP_EVENT_NOTIFY_TX:
        MODLOG_DFLT(INFO, "notify tx event; conn_handle=%d attr_handle=%d status=%d "
                    "ind=%d\n",
                    event->notify_tx.conn_handle,
                    event->notify_tx.attr_handle,
                    event->notify_tx.status,
                    event->notify_tx.indication);
        /*
        if(event->notify_tx.indication)
        {
            xEventGroupSetBits(ind_ack_event_group, INDICATION_ACK_BIT);
        }
        
        if(event->notify_tx.status == 0)
        {
            xEventGroupSetBits(ind_ack_event_group, INDICATION_ACK_BIT);
        }
        else
        {
            xEventGroupSetBits(ind_ack_event_group, INDICATION_ERR_BIT);
        }
        */
        return 0;

    default:
	return 0;
    }
}

static void
ble_spp_server_on_reset(int reason)
{
    ESP_LOGE(TAG, "Resetting state; reason=%d\n", reason);
}

static void
ble_spp_server_on_sync(void)
{
    int rc;

    rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    /* Figure out address to use while advertising (no privacy for now) */
    rc = ble_hs_id_infer_auto(0, &own_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* Printing ADDR */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(own_addr_type, addr_val, NULL);

    ESP_LOGI(TAG, "Device Address:%02x:%02x:%02x:%02x:%02x:%02x",
            addr_val[5],
            addr_val[4],
            addr_val[3],
            addr_val[2],
            addr_val[1],
            addr_val[0]);
    /* Begin advertising. */
    ble_spp_server_advertise();
}

void ble_spp_server_host_task(void *param)
{
    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

bool is_wav(const uint8_t *bytes) {
    return (bytes[0] == 'R' && bytes[1] == 'I' &&
            bytes[2] == 'F' && bytes[3] == 'F' &&
            bytes[8] == 'W' && bytes[9] == 'A' &&
            bytes[10] == 'V' && bytes[11] == 'E');
}

bool is_mp3(const uint8_t *bytes) {
    // Check optional ID3 tag
    if (bytes[0] == 'I' && bytes[1] == 'D' && bytes[2] == '3')
        return true;

    // Check frame header
    if (bytes[0] == 0xFF && (bytes[1] & 0xE0) == 0xE0)
        return true;

    return false;
}

/* Callback function for custom service */
static int  ble_svc_gatt_handler(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid;

    uuid = ctxt->chr->uuid;

    // Determine which characteristic is being accessed by examining its 16-bit UUID.
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_CMD_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                //MODLOG_DFLT(INFO, "Data received in write event,conn_handle = %x,attr_handle = %x", conn_handle, attr_handle);
                ESP_LOGI(TAG, "CMD buf len = %d (%d) [%d]", ctxt->om->om_len, ctxt->om->om_pkthdr_len, OS_MBUF_PKTLEN(ctxt->om));
                ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                memset(bt_rx_data, 0x00, MAX_BT_RX_BUFF);
                memcpy(bt_rx_data, ctxt->om->om_data, ctxt->om->om_len);
                bt_cmd_len = ctxt->om->om_len;
                bt_cmd_received = true;
                break;

            default:
                //MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    }
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_PYTHON_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                //MODLOG_DFLT(INFO, "Data received in write event,conn_handle = %x,attr_handle = %x", conn_handle, attr_handle);
                ESP_LOGI(TAG, "PY buf len = %d (%d) [%d]s", ctxt->om->om_len, ctxt->om->om_pkthdr_len, OS_MBUF_PKTLEN(ctxt->om));
                ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                ble_hs_mbuf_to_flat(ctxt->om, rx_buff_temp, 500, NULL);
                ESP_LOG_BUFFER_HEX(TAG, rx_buff_temp, OS_MBUF_PKTLEN(ctxt->om));

                if(mp_receiving_script)
                {
                    mp_script_timeout = 0; // Reset timeout
                    mp_script_seq_id = (ctxt->om->om_data[0] << 8) | ctxt->om->om_data[1];
                    if(mp_script_seq_id != (mp_script_seq_id_prev+1))
                    {
                        mp_receiving_script = false;
                        ble_indicate_python_load(FILE_LOAD_WRONG_SEQ);
                        Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        break;      
                    }
                    mp_script_seq_id_prev++;
                    // copy remaining chunk of data
                    os_mbuf_copydata(ctxt->om, 2, OS_MBUF_PKTLEN(ctxt->om) - 2, mp_script + mp_script_curr_len);
                    mp_script_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 2; // Each packet contains also a sequence id (2 bytes)
                    if(mp_script_curr_len == mp_script_tot_len)
                    {
                        mp_receiving_script = false;                        
                        // Check the integrity of the data once all the script is received.
                        calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len);
                        calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                        ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                        //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        if(calculated_crc == mp_script_crc)
                        {
                            ble_indicate_python_load(FILE_LOAD_OK);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEEXECSUCESS);
                            mp_script_ready = true;
                        }
                        else
                        {
                            ble_indicate_python_load(FILE_LOAD_CRC_ERR);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        }
                    }             
                }
                else
                {
                    if(ctxt->om->om_data[0] == PYTHON_WRITE_LOAD)
                    {
                        mp_script_timeout = 0; // Reset timeout
                        mp_script_tot_len = (ctxt->om->om_data[1] << 8) | ctxt->om->om_data[2];
                        mp_script_crc = (ctxt->om->om_data[3] << 24) | (ctxt->om->om_data[4] << 16) | (ctxt->om->om_data[5] << 8) | ctxt->om->om_data[6];
                        mp_script_seq_id = (ctxt->om->om_data[7] << 8) | ctxt->om->om_data[8];
                        ESP_LOGI(TAG,"tot len=%d, crc=%x, seq=%d", mp_script_tot_len, mp_script_crc, mp_script_seq_id);
                        mp_script_seq_id_prev = 0;
                        if(mp_script_tot_len > MAX_MP_SCRIPT_LEN)
                        {
                            ble_indicate_python_load(FILE_LOAD_TOO_BIG);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        }
                        else if(mp_script_seq_id != 0)
                        {
                            ble_indicate_python_load(FILE_LOAD_WRONG_SEQ);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        }
                        else
                        {
                            mp_script_ready = false;
                            memset(mp_script, 0x0, MAX_MP_SCRIPT_LEN);  // Reset buffer
                            mp_script_curr_len = 0;
                            mp_receiving_script = true;
                            // copy first chunk of data
                            os_mbuf_copydata(ctxt->om, 9, OS_MBUF_PKTLEN(ctxt->om) - 9, mp_script + mp_script_curr_len);
                            mp_script_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 9; // Firt packet contains also command id (1); script len (2), crc (4), sequence id (2) = 9 bytes                        
                            if(mp_script_curr_len == mp_script_tot_len) // All the script data received in the first packet (small script)
                            {
                                mp_receiving_script = false;                        
                                // Check the integrity of the data once all the script is received.
                                calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len);
                                calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                                ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                                //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                if(calculated_crc == mp_script_crc)
                                {
                                    ble_indicate_python_load(FILE_LOAD_OK);
                                    Codec_PlayOnboardSound(TONE_TYPE_CODEEXECSUCESS);
                                    mp_script_ready = true;
                                }
                                else
                                {
                                    ble_indicate_python_load(FILE_LOAD_CRC_ERR);
                                    Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                                }
                            }
                        }

                    } else if(ctxt->om->om_data[0] == PYTHON_WRITE_EXEC)
                    {
                        if(mp_script_ready)
                        {
                            mp_exec_script_from_ram(mp_script);
                        }
                        else
                        {
                            ble_indicate_python_exec(PYTHON_EXEC_NOT_FOUND);
                        }

                    } else if(ctxt->om->om_data[0] == PYTHON_WRITE_STOP)
                    {
                        mp_stop_script();
                    } else if(ctxt->om->om_data[0] == PYTHON_WRITE_SAVE)
                    {
                        if(ctxt->om->om_len != 2) // command + script id
                        {
                            ble_indicate_python_save(PYTHON_SAVE_ERROR);
                            break;
                        }
                        if(ctxt->om->om_data[1] > 7) // script id range is 0..7
                        {
                            ble_indicate_python_save(PYTHON_SAVE_ERROR);
                            break;
                        }
                        if(mp_script_ready)
                        {
                            mp_save_script(mp_script, ctxt->om->om_data[1], mp_script_tot_len);
                        }
                        else
                        {
                            ble_indicate_python_save(PYTHON_SAVE_NOT_FOUND);
                        }
                    } else if(ctxt->om->om_data[0] == PYTHON_WRITE_RESET)
                    {
                        mp_reset();
                    }
                    break;
                }

            case BLE_GATT_ACCESS_OP_READ_CHR:
                //MODLOG_DFLT(INFO, "Callback for read");
                break;

            default:
                MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    } 
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_SENSORS_STREAM_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                if(ctxt->om->om_data[0] == STREAM_WRITE_STATE)
                {
                    if(ctxt->om->om_len == STREAM_WRITE_STATE_LEN) // Check correct size is received
                    {
                        if((ctxt->om->om_data[1] & 0x01) == 0x01) // Enable most sensors stream
                        {
                            bt_most_sensors_stream_en = true;
                        }
                        else // Disable most sensors stream
                        {
                            bt_most_sensors_stream_en = false;
                        }
                        if((ctxt->om->om_data[1] & 0x02) == 0x02) // Enable others sensors stream
                        {
                            bt_others_sensors_stream_en = true;
                        }
                        else // Disable others sensors stream
                        {
                            bt_others_sensors_stream_en = false;
                        }
                    }
                }
                break;

            default:
                //MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    }
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_AUDIO_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                //MODLOG_DFLT(INFO, "Data received in write event,conn_handle = %x,attr_handle = %x", conn_handle, attr_handle);
                ESP_LOGI(TAG, "AUDIO buf len = %d (%d) [%d]s", ctxt->om->om_len, ctxt->om->om_pkthdr_len, OS_MBUF_PKTLEN(ctxt->om));
                //ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                ble_hs_mbuf_to_flat(ctxt->om, rx_buff_temp, 500, NULL);
                //ESP_LOG_BUFFER_HEX(TAG, rx_buff_temp, OS_MBUF_PKTLEN(ctxt->om));

                if(receiving_audio)
                {
                    audio_timeout = 0; // Reset timeout
                    audio_seq_id = (ctxt->om->om_data[0] << 8) | ctxt->om->om_data[1];
                    ESP_LOGI(TAG, "seq id = %d, curr len = %d (tot=%d)", audio_seq_id, audio_curr_len, audio_tot_len);
                    if(audio_seq_id != (audio_seq_id_prev+1))
                    {
                        receiving_audio = false;
                        ble_indicate_audio_load(FILE_LOAD_WRONG_SEQ);
                        break;      
                    }
                    audio_seq_id_prev++;
                    // copy remaining chunk of data
                    os_mbuf_copydata(ctxt->om, 2, OS_MBUF_PKTLEN(ctxt->om) - 2, audio_data + audio_curr_len);
                    audio_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 2; // Each packet contains also a sequence id (2 bytes)
                    if(audio_curr_len == audio_tot_len)
                    {
                        receiving_audio = false;                        
                        // Check the integrity of the data once all the script is received.
                        calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)audio_data, audio_tot_len);
                        calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                        ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                        //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        if(calculated_crc == audio_crc)
                        {
                            ble_indicate_audio_load(FILE_LOAD_OK);
                            audio_ready = true;
                            audio_play_type = AUDIO_PLAY_LOADED;
                        }
                        else
                        {
                            ble_indicate_audio_load(FILE_LOAD_CRC_ERR);
                        }
                    }             
                }
                else
                {
                    if(ctxt->om->om_data[0] == AUDIO_WRITE_LOAD)
                    {
                        audio_timeout = 0; // Reset timeout
                        audio_tot_len = (ctxt->om->om_data[1] << 24) | (ctxt->om->om_data[2] << 16) | (ctxt->om->om_data[3] << 8) | ctxt->om->om_data[4];
                        audio_crc = (ctxt->om->om_data[5] << 24) | (ctxt->om->om_data[6] << 16) | (ctxt->om->om_data[7] << 8) | ctxt->om->om_data[8];
                        audio_seq_id = (ctxt->om->om_data[9] << 8) | ctxt->om->om_data[10];
                        ESP_LOGI(TAG,"tot len=%d, crc=%x, seq=%d", audio_tot_len, audio_crc, audio_seq_id);
                        audio_seq_id_prev = 0;
                        if(audio_tot_len > MAX_RECORD_SIZE)
                        {
                            ble_indicate_audio_load(FILE_LOAD_TOO_BIG);
                        }
                        else if(audio_seq_id != 0)
                        {
                            ble_indicate_audio_load(FILE_LOAD_WRONG_SEQ);
                        }
                        else
                        {
                            audio_ready = false;
                            //memset(mp_script, 0x0, MAX_MP_SCRIPT_LEN);  // Reset buffer
                            audio_data = Codec_GetRecordPtr();
                            audio_curr_len = 0;
                            receiving_audio = true;
                            // copy first chunk of data
                            os_mbuf_copydata(ctxt->om, 11, OS_MBUF_PKTLEN(ctxt->om) - 11, audio_data + audio_curr_len);
                            audio_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 11; // Firt packet contains also command id (1); audio len (4), crc (4), sequence id (2) = 11 bytes
                            if(audio_curr_len == audio_tot_len) // All the audio data received in the first packet (small audio)
                            {
                                receiving_audio = false;                        
                                // Check the integrity of the data once all the script is received.
                                calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)audio_data, audio_tot_len);
                                calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                                ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                                //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                if(calculated_crc == audio_crc)
                                {
                                    ble_indicate_audio_load(FILE_LOAD_OK);
                                    audio_ready = true;
                                    audio_play_type = AUDIO_PLAY_LOADED;
                                }
                                else
                                {
                                    ble_indicate_audio_load(FILE_LOAD_CRC_ERR);
                                }
                            }
                        }

                    } 
                    else if(ctxt->om->om_data[0] == AUDIO_WRITE_EXEC)
                    {
                        if(OS_MBUF_PKTLEN(ctxt->om) == AUDIO_WRITE_EXEC_LEN)
                        {
                            if(audio_playing || audio_recording)
                            {
                                ESP_LOGI(TAG, "Already playing or recording");
                                ble_indicate_audio_exec(AUDIO_EXEC_ERROR);
                                break;
                            }
                            memset(audio_name, 0x0, 21);
                            strncpy((char*)audio_name, (char*)&(ctxt->om->om_data[1]), 20);
                            if(strlen(audio_name) == 0) // Play from RAM
                            {
                                if(audio_ready)
                                {
                                    if(audio_play_type == AUDIO_PLAY_LOADED)
                                    {
                                        if(is_wav(audio_data)) // Wav audio
                                        {
                                            numChannels = audio_data[22]+(audio_data[23]<<8);
                                            sampleRate = audio_data[24]+(audio_data[25]<<8)+(audio_data[26]<<16)+(audio_data[27]<<24);        
                                            bitsPerChannel = audio_data[34]+(audio_data[35]<<8);
                                            ESP_LOGI(TAG, "wav ch=%d, rate=%d, bits=%d\n", numChannels, sampleRate, bitsPerChannel);
                                            if((numChannels==1) && (sampleRate==12000) && (bitsPerChannel==16)) {
                                                ESP_LOGI(TAG, "Playing wav file from RAM");
                                                if(Codec_PlayWAVFile(audio_data, audio_tot_len) != ESP_OK)
                                                {
                                                    ESP_LOGI(TAG, "Play error");
                                                    ble_indicate_audio_exec(AUDIO_EXEC_ERROR);
                                                }
                                                else
                                                {
                                                    audio_playing = true;
                                                }                                                
                                            }
                                            else
                                            {
                                                ESP_LOGI(TAG, "Format not supported");
                                                ble_indicate_audio_exec(AUDIO_EXEC_NOT_SUPPORTED);
                                            }
                                        }
                                        else if(is_mp3(audio_data))// mp3 audio
                                        {
                                            if(audio_data[0] == 0x49) { // ID3 header detected
                                                id3Size = audio_data[9] + (audio_data[8]<<7) + (audio_data[7]<<14) + (audio_data[6]<<21);
                                                mp3Ver = (audio_data[11+id3Size]&0x18)>>3;
                                                numChannels = (audio_data[13+id3Size]&0xC0)>>6;
                                                sampleRate = (audio_data[12+id3Size]&0x0C)>>2;            
                                            } else if(audio_data[0] == 0xFF) { // Mp3 header (no ID3 included)
                                                mp3Ver = (audio_data[1]&0x18)>>3;
                                                numChannels = (audio_data[3]&0xC0)>>6;
                                                sampleRate = (audio_data[2]&0x0C)>>2;
                                            } else {
                                                ESP_LOGI(TAG, "Format not supported");
                                                ble_indicate_audio_exec(AUDIO_EXEC_NOT_SUPPORTED);
                                            }
                                            ESP_LOGI(TAG, "mp3 ver=%d, rate=%d, ch=%d\n", mp3Ver, sampleRate, numChannels);
                                            if((mp3Ver==0) && (numChannels==3) && (sampleRate==1)) {
                                                ESP_LOGI(TAG, "Playing mp3 file from RAM");
                                                if(Codec_PlayMP3File(audio_data, audio_tot_len) != ESP_OK) {
                                                    ESP_LOGI(TAG, "Play error");
                                                    ble_indicate_audio_exec(AUDIO_EXEC_ERROR);              
                                                }
                                                else
                                                {
                                                    audio_playing = true;
                                                }
                                            } else {
                                                ESP_LOGI(TAG, "Format not supported");
                                                ble_indicate_audio_exec(AUDIO_EXEC_NOT_SUPPORTED);            
                                            }
                                        }
                                        else // audio not supported
                                        {
                                            ESP_LOGI(TAG, "Audio not supported");
                                            ble_indicate_audio_exec(AUDIO_EXEC_NOT_SUPPORTED);
                                        }
                                    }
                                    else if(audio_play_type == AUDIO_PLAY_RECORDED)
                                    {
                                        ESP_LOGI(TAG, "Playing recorded audio from RAM");
                                        if(Codec_PlayRecorded() != ESP_OK) {
                                            ESP_LOGI(TAG, "Play error");
                                            ble_indicate_audio_exec(AUDIO_EXEC_ERROR); 
                                        }
                                        else
                                        {
                                            audio_playing = true;
                                        }
                                    }
                                }
                                else
                                {
                                    ESP_LOGI(TAG, "Audio not available in RAM");
                                    ble_indicate_audio_exec(AUDIO_EXEC_NOT_FOUND);
                                }

                            }
                            else // Play from internal storage
                            {

                            }
                        }

                    } 
                    else if(ctxt->om->om_data[0] == AUDIO_WRITE_STOP)
                    {
                        Codec_Stop();
                        ble_indicate_audio_exec(AUDIO_EXEC_OK);
                        audio_playing = false;
                    }
                    else if(ctxt->om->om_data[0] == AUDIO_WRITE_SAVE)
                    {
                        
                    }
                    else if(ctxt->om->om_data[0] == AUDIO_WRITE_REC)
                    {
                        uint8_t duration = ctxt->om->om_data[1];
                        if(audio_playing || audio_recording)
                        {
                            ESP_LOGI(TAG, "Already playing or recording");
                            ble_indicate_audio_rec(AUDIO_REC_ERROR);
                            break;
                        }
                        if(duration > 10)
                        {
                            ble_indicate_audio_rec(AUDIO_REC_TOO_LONG);
                        }
                        else
                        {
                            audio_ready = false;
                            audio_play_type = AUDIO_PLAY_RECORDED;                            
                            Codec_RecordWAVFile(duration);
                            audio_recording = true;                            
                        }
                    } 
                    else if(ctxt->om->om_data[0] == AUDIO_WRITE_TONE)
                    {
                        uint16_t freq = (ctxt->om->om_data[1] << 8) | ctxt->om->om_data[2];
                        uint32_t duration = (ctxt->om->om_data[3] << 8) | ctxt->om->om_data[4];
                        duration = duration*100; // Convert to ms
                        ESP_LOGI(TAG, "Play tone freq=%d, dur=%d\n", freq, duration);
                        if(Codec_PlayTone(freq, duration) != ESP_OK) {
                            ESP_LOGI(TAG, "Play tone error");
                            ble_indicate_audio_exec(AUDIO_EXEC_ERROR);          
                        }
                        else
                        {
                            audio_playing = true;
                        }   
                    }   
                }
                break;

            case BLE_GATT_ACCESS_OP_READ_CHR:
                //MODLOG_DFLT(INFO, "Callback for read");
                break;

            default:
                MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    }
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_FILE_SYSTEM_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                //MODLOG_DFLT(INFO, "Data received in write event,conn_handle = %x,attr_handle = %x", conn_handle, attr_handle);
                //ESP_LOGI(TAG, "FS buf len = %d (%d) [%d]", ctxt->om->om_len, ctxt->om->om_pkthdr_len, OS_MBUF_PKTLEN(ctxt->om));
                //ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                ble_hs_mbuf_to_flat(ctxt->om, rx_buff_temp, 500, NULL);
                //ESP_LOG_BUFFER_HEX(TAG, rx_buff_temp, OS_MBUF_PKTLEN(ctxt->om));

                if(fs_receiving_file)
                {
                    fs_timeout = 0; // Reset timeout
                    fs_seq_id = (ctxt->om->om_data[0] << 8) | ctxt->om->om_data[1];
                    //ESP_LOGI(TAG, "seq id = %d, curr len = %d (tot=%d)", fs_seq_id, fs_curr_len, fs_tot_len);
                    if(fs_seq_id != (fs_seq_id_prev+1))
                    {
                        fs_receiving_file = false;
                        ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_WRONG_SEQ);
                        Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        free(fs_data);
                        fs_data = NULL;
                        break;      
                    }
                    fs_seq_id_prev++;
                    // copy remaining chunk of data
                    os_mbuf_copydata(ctxt->om, 2, OS_MBUF_PKTLEN(ctxt->om) - 2, fs_data + fs_curr_len);
                    fs_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 2; // Each packet contains also a sequence id (2 bytes)
                    if(fs_curr_len == fs_tot_len)
                    {
                        fs_receiving_file = false;                        
                        // Check the integrity of the data once all the script is received.
                        calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)fs_data, fs_tot_len);
                        calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                        ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                        //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                        if(calculated_crc == fs_crc)
                        {
                            ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_OK);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEEXECSUCESS);
                            fs_file_ready = true;
                            // Do not free fs_data here as it could be used later for saving
                        }
                        else
                        {
                            ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_CRC_ERR);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                            free(fs_data);
                            fs_data = NULL;
                        }
                    }             
                }
                else
                {
                    if(ctxt->om->om_data[0] == FS_WRITE_LOAD)
                    {
                        fs_timeout = 0; // Reset timeout
                        fs_tot_len = (ctxt->om->om_data[1] << 24) | (ctxt->om->om_data[2] << 16) | (ctxt->om->om_data[3] << 8) | ctxt->om->om_data[4];
                        fs_crc = (ctxt->om->om_data[5] << 24) | (ctxt->om->om_data[6] << 16) | (ctxt->om->om_data[7] << 8) | ctxt->om->om_data[8];
                        fs_seq_id = (ctxt->om->om_data[9] << 8) | ctxt->om->om_data[10];
                        ESP_LOGI(TAG,"FS tot len=%d, crc=%x, seq=%d", fs_tot_len, fs_crc, fs_seq_id);
                        fs_seq_id_prev = 0;
                        if(fs_data != NULL) // Free any previous allocated buffer
                        {
                            free(fs_data);
                            fs_data = NULL;
                        }
                        fs_data = malloc(fs_tot_len);
                        if(fs_data == NULL)
                        {
                            ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_TOO_BIG);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                        }
                        else if(fs_seq_id != 0)
                        {
                            ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_WRONG_SEQ);
                            Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                            free(fs_data);
                            fs_data = NULL;
                        }
                        else
                        {
                            fs_file_ready = false;
                            fs_curr_len = 0;
                            fs_receiving_file = true;
                            // copy first chunk of data
                            os_mbuf_copydata(ctxt->om, 11, OS_MBUF_PKTLEN(ctxt->om) - 11, fs_data + fs_curr_len);
                            fs_curr_len += OS_MBUF_PKTLEN(ctxt->om) - 11; // Firt packet contains also command id (1); file len (4), crc (4), sequence id (2) = 11 bytes                        
                            if(fs_curr_len == fs_tot_len) // All the file data received in the first packet (small file)
                            {
                                fs_receiving_file = false;                        
                                // Check the integrity of the data once all the script is received.
                                calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)fs_data, fs_tot_len);
                                calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
                                ESP_LOGI(TAG, "calc crc=%x", calculated_crc);
                                //ESP_LOGI(TAG, "le) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_le(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                //ESP_LOGI(TAG, "be) crc1=%x, crc2=%x, crc3=%x, crc4=%x",  esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(~initial_crc, (uint8_t*)mp_script, mp_script_tot_len), ~esp_rom_crc32_be(initial_crc, (uint8_t*)mp_script, mp_script_tot_len));
                                if(calculated_crc == fs_crc)
                                {
                                    ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_OK);
                                    Codec_PlayOnboardSound(TONE_TYPE_CODEEXECSUCESS);
                                    fs_file_ready = true;
                                    // Do not free fs_data here as it could be used later for saving
                                }
                                else
                                {
                                    ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_CRC_ERR);
                                    Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
                                    free(fs_data);
                                    fs_data = NULL;
                                }
                            }
                        }
                    } 
                    else if(ctxt->om->om_data[0] == FS_WRITE_SAVE)
                    {
                        memset(fs_filename, 0, FILENAME_MAX_LEN);
                        strncpy(fs_filename, (const char*)&ctxt->om->om_data[1], FILENAME_MAX_LEN);
                        fs_filename[FILENAME_MAX_LEN-1] = '\0';
                        ESP_LOGI(TAG, "FS save filename: %s and size %d", fs_filename, fs_tot_len);
                        if(fs_file_ready)
                        {
                            mp_save_file(fs_data, fs_filename, fs_tot_len);
                        }
                        else
                        {
                            ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_NOT_FOUND);
                        }
                    }
                    else if(ctxt->om->om_data[0] == FS_WRITE_DELETE)
                    {
                        memset(fs_filename, 0, FILENAME_MAX_LEN);
                        strncpy(fs_filename, (const char*)&ctxt->om->om_data[1], FILENAME_MAX_LEN);
                        fs_filename[FILENAME_MAX_LEN-1] = '\0';
                        ESP_LOGI(TAG, "FS delete filename: %s", fs_filename);
                        mp_delete_file(fs_filename);
                    }         
                    else if(ctxt->om->om_data[0] == FS_WRITE_LIST)
                    {
                        mp_list_files();
                    }   
                    else if(ctxt->om->om_data[0] == FS_WRITE_ERASE_ALL)
                    {
                        
                    }
                    else if(ctxt->om->om_data[0] == FS_WRITE_DOWNLOAD)
                    {
                        memset(fs_filename, 0, FILENAME_MAX_LEN);
                        strncpy(fs_filename, (const char*)&ctxt->om->om_data[1], FILENAME_MAX_LEN);
                        fs_filename[FILENAME_MAX_LEN-1] = '\0';
                        ESP_LOGI(TAG, "FS donwload filename: %s", fs_filename);
                        mp_read_file(fs_filename);                     
                    }   
                    else if(ctxt->om->om_data[0] == FS_WRITE_DOWNLOAD_ACK)
                    {
                        xEventGroupSetBits(ind_ack_event_group, INDICATION_ACK_BIT);
                    }   
                    else if(ctxt->om->om_data[0] == FS_WRITE_MEM_FREE)
                    {
                        if(fs_data != NULL) // Free any previous allocated buffer
                        {
                            free(fs_data);
                            fs_data = NULL;
                        }
                        fs_file_ready = false;
                        mp_list_files_free_buffer();
                        mp_read_file_free_buffer();
                    }                    
                    break;
                }

            case BLE_GATT_ACCESS_OP_READ_CHR:
                //MODLOG_DFLT(INFO, "Callback for read");
                break;

            default:
                MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    }    
    if (ble_uuid_cmp(uuid, BLE_UUID16_DECLARE(BLE_SVC_DEVICE_INFO_CHR_UUID16)) == 0) {
        switch (ctxt->op) {
            case BLE_GATT_ACCESS_OP_WRITE_CHR:
                //MODLOG_DFLT(INFO, "Data received in write event,conn_handle = %x,attr_handle = %x", conn_handle, attr_handle);
                ESP_LOGI(TAG, "FS buf len = %d (%d) [%d]", ctxt->om->om_len, ctxt->om->om_pkthdr_len, OS_MBUF_PKTLEN(ctxt->om));
                ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                ble_hs_mbuf_to_flat(ctxt->om, rx_buff_temp, 500, NULL);
                ESP_LOG_BUFFER_HEX(TAG, rx_buff_temp, OS_MBUF_PKTLEN(ctxt->om));

                if(ctxt->om->om_data[0] == DEV_INFO_WRITE_FIRMWARE)
                {
                    mp_firmware_info();
                } 
                else if(ctxt->om->om_data[0] == DEV_INFO_WRITE_MEMORY)
                {
                    mp_mem_info();
                }                                              
                break;

            case BLE_GATT_ACCESS_OP_READ_CHR:
                //MODLOG_DFLT(INFO, "Callback for read");
                break;

            default:
                MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    }    
    return 0;    

}

static esp_ble_ota_char_t find_ota_char_and_desr_by_handle(uint16_t handle)
{
    esp_ble_ota_char_t ret = INVALID_CHAR;

    for (int i = 0; i < OTA_IDX_NB ; i++) {
        if (handle == ota_handle_table[i]) {
            switch (i) {
            case RECV_FW_CHAR_VAL_IDX:
                ret = RECV_FW_CHAR;
                break;
            case OTA_STATUS_CHAR_VAL_IDX:
                ret = OTA_STATUS_CHAR;
                break;
            case CMD_CHAR_VAL_IDX:
                ret = CMD_CHAR;
                break;
            case CUS_CHAR_VAL_IDX:
                ret = CUS_CHAR;
                break;
            default:
                ret = INVALID_CHAR;
                break;
            }
        }
    }
    return ret;
}

static int esp_ble_ota_notification_data(uint16_t conn_handle, uint16_t attr_handle, uint8_t cmd_ack[], esp_ble_ota_char_t ota_char)
{
    struct os_mbuf *txom;
    bool notify_enable = false;
    int rc;
    txom = ble_hs_mbuf_from_flat(cmd_ack, CMD_ACK_LENGTH);

    switch (ota_char) {
    case RECV_FW_CHAR:
        if (ota_notification.recv_fw_ntf_enable) {
            notify_enable = true;
        }
        break;
    case OTA_STATUS_CHAR:
        if (ota_notification.process_bar_ntf_enable) {
            notify_enable = true;
        }
        break;
    case CMD_CHAR:
        if (ota_notification.command_ntf_enable) {
            notify_enable = true;
        }
        break;
    case CUS_CHAR:
        if (ota_notification.customer_ntf_enable) {
            notify_enable = true;
        }
        break;
    case INVALID_CHAR:
        break;
    }

    if (notify_enable) {
        rc = ble_gattc_notify_custom(conn_handle, attr_handle, txom);
        if (rc == 0) {
            ESP_LOGD(TAG, "Notification sent, attr_handle = %d", attr_handle);
        } else {
            ESP_LOGE(TAG, "Error in sending notification, rc = %d", rc);
        }
        return rc;
    }

    /* If notifications are disabled return ESP_FAIL */
    ESP_LOGI(TAG, "Notify is disabled");
    return ESP_FAIL;
}

static void ble_ota_start_write_chr(struct os_mbuf *om)
{
    uint8_t cmd_ack[CMD_ACK_LENGTH] = {0x03, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00
                                      };
    uint16_t crc16;

    esp_ble_ota_char_t ota_char = find_ota_char_and_desr_by_handle(attribute_handle);
    if ((om->om_data[0] == 0x01) && (om->om_data[1] == 0x00)) {
        start_ota = true;

        ota_total_len = (om->om_data[2]) + (om->om_data[3] * 256) +
                        (om->om_data[4] * 256 * 256) + (om->om_data[5] * 256 * 256 * 256);

        ESP_LOGI(TAG, "recv ota start cmd, fw_length = %" PRIu32 "", ota_total_len);

        if (fw_buf == NULL) {
            fw_buf = (uint8_t *)malloc(ota_block_size * sizeof(uint8_t));
            if (fw_buf == NULL) 
            {
                ESP_LOGE(TAG, "%s -  malloc fail", __func__);
            }
        } else {
            memset(fw_buf, 0x0, ota_block_size);
        }
        cur_sector = 0;
        cur_packet = 0;

        cmd_ack[2] = 0x01;
        cmd_ack[3] = 0x00;
        crc16 = crc16_ccitt(cmd_ack, 18);
        cmd_ack[18] = crc16 & 0xff;
        cmd_ack[19] = (crc16 & 0xff00) >> 8;
        esp_ble_ota_notification_data(connection_handle, attribute_handle, cmd_ack, ota_char);
            
    } else if ((om->om_data[0] == 0x02) && (om->om_data[1] == 0x00)) {
        printf("\nCMD_CHAR -> 0 : %d, 1 : %d", om->om_data[0],
               om->om_data[1]);

        xSemaphoreTake(notify_sem, portMAX_DELAY);

        start_ota = false;
        ota_total_len = 0;

        xSemaphoreGive(notify_sem);

        ESP_LOGD(TAG, "recv ota stop cmd");
        cmd_ack[2] = 0x02;
        cmd_ack[3] = 0x00;
        crc16 = crc16_ccitt(cmd_ack, 18);
        cmd_ack[18] = crc16 & 0xff;
        cmd_ack[19] = (crc16 & 0xff00) >> 8;
        esp_ble_ota_notification_data(connection_handle, attribute_handle, cmd_ack, ota_char);
        free(fw_buf);
        fw_buf = NULL;
    }
}

static void esp_ble_ota_write_chr(struct os_mbuf *om)
{
    esp_ble_ota_char_t ota_char = find_ota_char_and_desr_by_handle(attribute_handle);

    uint8_t cmd_ack[CMD_ACK_LENGTH] = {0x03, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00
                                      };
    uint16_t crc16;

    if ((om->om_data[0] + (om->om_data[1] * 256)) != cur_sector) {
        // sector error
        if ((om->om_data[0] == 0xff) && (om->om_data[1] == 0xff)) {
            // last sector
            ESP_LOGD(TAG, "Last sector");
        } else {
            // sector error
            ESP_LOGE(TAG, "%s - sector index error, cur: %" PRIu32 ", recv: %d", __func__,
                     cur_sector, (om->om_data[0] + (om->om_data[1] * 256)));
            cmd_ack[0] = om->om_data[0];
            cmd_ack[1] = om->om_data[1];
            cmd_ack[2] = 0x02; //sector index error
            cmd_ack[3] = 0x00;
            cmd_ack[4] = cur_sector & 0xff;
            cmd_ack[5] = (cur_sector & 0xff00) >> 8;
            crc16 = crc16_ccitt(cmd_ack, 18);
            cmd_ack[18] = crc16 & 0xff;
            cmd_ack[19] = (crc16 & 0xff00) >> 8;
            esp_ble_ota_notification_data(connection_handle, attribute_handle, cmd_ack, ota_char);
        }
    }

    if (om->om_data[2] != cur_packet) { // packet seq error
        if (om->om_data[2] == 0xff) { // last packet
            ESP_LOGD(TAG, "last packet");
            goto write_ota_data;
        } else { // packet seq error
            ESP_LOGE(TAG, "%s - packet index error, cur: %" PRIu32 ", recv: %d", __func__,
                     cur_packet, om->om_data[2]);
        }
    }

write_ota_data:
    os_mbuf_copydata(om, 3, os_mbuf_len(om) - 3, fw_buf + fw_buf_offset);
    fw_buf_offset += os_mbuf_len(om) - 3;

    ESP_LOGD(TAG, "DEBUG: Sector:%" PRIu32 ", total length:%" PRIu32 ", length:%d", cur_sector,
             fw_buf_offset, os_mbuf_len(om) - 3);

    if (om->om_data[2] == 0xff) {
        cur_packet = 0;
        cur_sector++;
        ESP_LOGD(TAG, "DEBUG: recv %" PRIu32 " sector", cur_sector);
        goto sector_end;
    } else {
        ESP_LOGD(TAG, "DEBUG: wait next packet");
        cur_packet++;
    }
    return;

sector_end:
    if (fw_buf_offset < ota_block_size) {
        write_to_ringbuf(fw_buf, fw_buf_offset);
    } else {
        write_to_ringbuf(fw_buf, 4096);
    }

    fw_buf_offset = 0;
    memset(fw_buf, 0x0, ota_block_size);

    cmd_ack[0] = om->om_data[0];
    cmd_ack[1] = om->om_data[1];
    cmd_ack[2] = 0x00; //success
    cmd_ack[3] = 0x00;
    crc16 = crc16_ccitt(cmd_ack, 18);
    cmd_ack[18] = crc16 & 0xff;
    cmd_ack[19] = (crc16 & 0xff00) >> 8;
    counter = true;
    esp_ble_ota_notification_data(connection_handle, attribute_handle, cmd_ack, ota_char);
}

static uint16_t crc16_ccitt(const unsigned char *buf, int len)
{
    uint16_t crc16 = 0;
    int32_t i;

    while (len--) {
        crc16 ^= *buf++ << 8;

        for (i = 0; i < 8; i++) {
            if (crc16 & 0x8000) {
                crc16 = (crc16 << 1) ^ 0x1021;
            } else {
                crc16 = crc16 << 1;
            }
        }
    }

    return crc16;
}

static int ble_ota_gatt_handler(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    esp_ble_ota_char_t ota_char;

    attribute_handle = attr_handle;

    switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            ota_char = find_ota_char_and_desr_by_handle(attr_handle);
            ESP_LOGI(TAG, "client read, ota_char: %d", ota_char);
            break;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:

            ota_char = find_ota_char_and_desr_by_handle(attr_handle);
            ESP_LOGD(TAG, "client write; len = %d", os_mbuf_len(ctxt->om));

            if (ota_char == RECV_FW_CHAR) {
                if (start_ota) {
                    esp_ble_ota_write_chr(ctxt->om);

                } else {
                    ESP_LOGE(TAG, "%s -  don't receive the start cmd", __func__);
                }
            } else if (ota_char == CMD_CHAR) {
                ble_ota_start_write_chr(ctxt->om);
            }
            break;

        default:
            return BLE_ATT_ERR_UNLIKELY;
    }
    return 0;
}


/* Define new custom service */
static const struct ble_gatt_svc_def new_ble_svc_gatt_defs[] = {
    {
        /*** Service: SPP */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_THYMIO_UUID16),
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* commands characteristic (from device to Thymio) */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_CMD_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, {
                /* sensors stream characteristic (from Thymio to device) */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_SENSORS_STREAM_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_sensors_stream_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
            }, {
                /* python characteristic */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_PYTHON_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_python_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
            }, {
                /* audio characteristic */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_AUDIO_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_audio_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
            }, {
                /* file system characteristic */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_FILE_SYSTEM_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_fs_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
            }, {
                /* device info characteristic */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_DEVICE_INFO_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_dev_info_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_INDICATE,
            }, {
                /* python stdout characteristic */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_PYTHON_STDOUT_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_python_stdout_val_handle,
                .flags = BLE_GATT_CHR_F_INDICATE,
            }, {
                0, /* No more characteristics */
            }
        },
    },
    {
        /* OTA Service Declaration */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_OTA_SERVICE_UUID),
        .characteristics = (struct ble_gatt_chr_def[])
        {
            {
                /* Receive Firmware Characteristic */
                .uuid = BLE_UUID16_DECLARE(RECV_FW_UUID),
                .access_cb = ble_ota_gatt_handler,
                .val_handle = &receive_fw_val,
                .flags = BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_WRITE,
            }, {
                /* OTA Characteristic */
                .uuid = BLE_UUID16_DECLARE(OTA_BAR_UUID),
                .access_cb = ble_ota_gatt_handler,
                .val_handle = &ota_status_val,
                .flags = BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_READ,
            }, {
                /* Command Characteristic */
                .uuid = BLE_UUID16_DECLARE(COMMAND_UUID),
                .access_cb = ble_ota_gatt_handler,
                .val_handle = &command_val,
                .flags = BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_WRITE,
            }, {
                /* Customer characteristic */
                .uuid = BLE_UUID16_DECLARE(CUSTOMER_UUID),
                .access_cb = ble_ota_gatt_handler,
                .val_handle = &custom_val,
                .flags = BLE_GATT_CHR_F_INDICATE | BLE_GATT_CHR_F_WRITE,
            }, {
                0, /* No more characteristics in this service */
            }
        },
    },    
    {
        0, /* No more services. */
    },
};

static void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    //char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        //MODLOG_DFLT(DEBUG, "registered service %s with handle=%d\n",
        //            ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
        //            ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        //MODLOG_DFLT(DEBUG, "registering characteristic %s with "
        //            "def_handle=%d val_handle=%d\n",
        //            ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
        //            ctxt->chr.def_handle,
        //            ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        //MODLOG_DFLT(DEBUG, "registering descriptor %s with handle=%d\n",
        //            ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
        //            ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}
 
int gatt_svr_init(void)
{
    int rc = 0;
    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(new_ble_svc_gatt_defs);

    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(new_ble_svc_gatt_defs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

static void bt_rx_tx_task(void *pvParameters) 
{
    int rc = 0;
    while(1)
    {
        if(bt_cmd_received) // Handle commands coming from the device
        {
            bt_cmd_received = false;
            switch(bt_rx_data[0])
            {
                case CMD_WRITE_MOST_ACTUATORS:
                    if(bt_cmd_len == CMD_WRITE_MOST_ACTUATORS_LEN) // Check correct size is received
                    {
                        memcpy(bt_rx_data_temp, bt_rx_data, bt_cmd_len);
                        Leds_SetCircleBrightness(
                            (bt_rx_data_temp[1]&0x0F), 
                            (bt_rx_data_temp[1]&0xF0)>>4,
                            (bt_rx_data_temp[2]&0x0F),
                            (bt_rx_data_temp[2]&0xF0)>>4,
                            (bt_rx_data_temp[3]&0x0F),
                            (bt_rx_data_temp[3]&0xF0)>>4,
                            (bt_rx_data_temp[4]&0x0F),
                            (bt_rx_data_temp[4]&0xF0)>>4);
                        
                        Leds_SetLegoFrontBrightness(
                            (bt_rx_data_temp[5]&0x0F), 
                            (bt_rx_data_temp[5]&0xF0)>>4,
                            (bt_rx_data_temp[6]&0x0F),
                            (bt_rx_data_temp[6]&0xF0)>>4,
                            (bt_rx_data_temp[7]&0x0F),
                            (bt_rx_data_temp[7]&0xF0)>>4,
                            (bt_rx_data_temp[8]&0x0F),
                            (bt_rx_data_temp[8]&0xF0)>>4);

                        Leds_SetLegoBackBrightness(
                            (bt_rx_data_temp[9]&0x0F), 
                            (bt_rx_data_temp[9]&0xF0)>>4,
                            (bt_rx_data_temp[10]&0x0F),
                            (bt_rx_data_temp[10]&0xF0)>>4,
                            (bt_rx_data_temp[11]&0x0F),
                            (bt_rx_data_temp[11]&0xF0)>>4,
                            (bt_rx_data_temp[12]&0x0F),
                            (bt_rx_data_temp[12]&0xF0)>>4);

                        Leds_SetFrontLeftBrightness((bt_rx_data_temp[13]&0x0F), (bt_rx_data_temp[13]&0xF0)>>4, (bt_rx_data_temp[14]&0x0F));
                        Leds_SetFrontRightBrightness((bt_rx_data_temp[15]&0x0F), (bt_rx_data_temp[15]&0xF0)>>4, (bt_rx_data_temp[16]&0x0F));
                        Leds_SetBackLeftBrightness((bt_rx_data_temp[17]&0x0F), (bt_rx_data_temp[17]&0xF0)>>4, (bt_rx_data_temp[18]&0x0F));
                        Leds_SetBackRightBrightness((bt_rx_data_temp[19]&0x0F), (bt_rx_data_temp[19]&0xF0)>>4, (bt_rx_data_temp[20]&0x0F));

                        SetMotorTargets(bt_rx_data_temp[21]|(bt_rx_data_temp[22]<<8), bt_rx_data_temp[23]|(bt_rx_data_temp[24]<<8));

                        // Play sound based on bt_rx_data_temp[25]
                        if((bt_rx_data_temp[25] > 0) && (bt_rx_data_temp[15] <= 16))
                        {
                            Codec_PlayOnboardSound(bt_rx_data_temp[25] - 1);
                        }
                    }
                    break;

                case CMD_WRITE_OTHERS_ACTUATORS:
                    if(bt_cmd_len == CMD_WRITE_OTHERS_ACTUATORS_LEN) // Check correct size is received
                    {
                        memcpy(bt_rx_data_temp, bt_rx_data, bt_cmd_len);
                        
                        Leds_SetColorSensorBrightness((bt_rx_data_temp[1]&0x0F), (bt_rx_data_temp[1]&0xF0)>>4, (bt_rx_data_temp[2]&0x0F));
                        Leds_SetDebugBrightness((bt_rx_data_temp[3]&0x0F), (bt_rx_data_temp[3]&0xF0)>>4, (bt_rx_data_temp[4]&0x0F));
                        
                        //Behavior_Disable(B_LEDS_BUTTON); // Otherwise the buttons leds cannot be controlled
                        Leds_SetSingleBrightness(E_Led_Button_Forward, bt_rx_data_temp[5]&0x0F);
                        Leds_SetSingleBrightness(E_Led_Button_Right, (bt_rx_data_temp[5]&0xF0)>>4);
                        Leds_SetSingleBrightness(E_Led_Button_Backward, bt_rx_data_temp[6]&0x0F);
                        Leds_SetSingleBrightness(E_Led_Button_Left, (bt_rx_data_temp[6]&0xF0)>>4);

                        //Behavior_Disable(B_LED_RC5); // Otherwise the RC5 led cannot be controlled
                        Leds_SetSingleBrightness(E_Led_RC5, bt_rx_data_temp[7]&0x0F);

                        if((bt_rx_data_temp[7]&0x10) == 0x10)
                        {
                            Behavior_Disable(B_LED_MIC);
                            Behavior_Enable(B_LED_MIC_STATE); 
                        }
                        else
                        {
                            Behavior_Disable(B_LED_MIC);
                            Behavior_Disable(B_LED_MIC_STATE); 
                        }

                    }                
                    break;

                default:
                    break;
            }
        }

        if(bt_most_sensors_stream_en)
        {
            memset(bt_tx_data, 0x00, MAX_BT_TX_BUFF);
            bt_tx_data[0] = STREAM_NOTIFY_MOST_SENSORS;
            // Send update to the device
            T_HSV hsv_temp = ColorSensor_GetHsv();
            bt_tx_data[1] = hsv_temp.Hue&0xFF;
            bt_tx_data[2] = hsv_temp.Hue>>8;
            bt_tx_data[3] = hsv_temp.Saturation;
            bt_tx_data[4] = hsv_temp.Value;

            uint16_t prox_temp = GetGroundValue(0);
            bt_tx_data[5] = prox_temp&0xFF;
            bt_tx_data[6] = prox_temp>>8;
            prox_temp = GetGroundValue(1);
            bt_tx_data[7] = prox_temp&0xFF;
            bt_tx_data[8] = prox_temp>>8;

            T_Axis acc_temp, gyro_temp;
            acc_temp = Accelerometer_GetAcceleration();
            bt_tx_data[9] = acc_temp.X&0xFF;
            bt_tx_data[10] = acc_temp.X>>8;
            bt_tx_data[11] = acc_temp.Y&0xFF;
            bt_tx_data[12] = acc_temp.Y>>8;
            bt_tx_data[13] = acc_temp.Z&0xFF;
            bt_tx_data[14] = acc_temp.Z>>8;
            gyro_temp = Gyroscope_GetAngularVelocity();
            bt_tx_data[15] = gyro_temp.X&0xFF;
            bt_tx_data[16] = gyro_temp.X>>8;
            bt_tx_data[17] = gyro_temp.Y&0xFF;
            bt_tx_data[18] = gyro_temp.Y>>8;
            bt_tx_data[19] = gyro_temp.Z&0xFF;
            bt_tx_data[20] = gyro_temp.Z>>8;

            uint8_t* status_temp = Buttons_GetStatus();
            bt_tx_data[21] = 0;
            if(status_temp[0] == 1)
            {
                bt_tx_data[21] |= 0x01;
            }
            if(status_temp[1] == 1)
            {
                bt_tx_data[21] |= 0x02;
            }
            if(status_temp[2] == 1)
            {
                bt_tx_data[21] |= 0x04;
            }
            if(status_temp[3] == 1)
            {
                bt_tx_data[21] |= 0x08;
            }
            if(status_temp[4] == 1)
            {
                bt_tx_data[21] |= 0x10;
            }

            int16_t vol = STM32_GetMicrophoneIntensity();
            bt_tx_data[22] = vol&0xFF;
            bt_tx_data[23] = vol>>8;

            prox_temp = GetProximityValue(0);
            bt_tx_data[24] = prox_temp&0xFF;
            bt_tx_data[25] = prox_temp>>8;
            prox_temp = GetProximityValue(1);
            bt_tx_data[26] = prox_temp&0xFF;
            bt_tx_data[27] = prox_temp>>8;        
            prox_temp = GetProximityValue(2);
            bt_tx_data[28] = prox_temp&0xFF;
            bt_tx_data[29] = prox_temp>>8;   
            
            prox_temp = GetProximityValue(3);
            bt_tx_data[30] = prox_temp&0xFF;
            bt_tx_data[31] = prox_temp>>8;    
            prox_temp = GetProximityValue(4);
            bt_tx_data[32] = prox_temp&0xFF;
            bt_tx_data[33] = prox_temp>>8;    
            prox_temp = GetProximityValue(5);
            bt_tx_data[34] = prox_temp&0xFF;
            bt_tx_data[35] = prox_temp>>8;
            prox_temp = GetProximityValue(6);
            bt_tx_data[36] = prox_temp&0xFF;
            bt_tx_data[37] = prox_temp>>8;            

            int16_t toggle = -1;
            bt_tx_data[38] = RC5_GetCommand(&toggle);

            struct os_mbuf *txom;
            txom = ble_hs_mbuf_from_flat(bt_tx_data, sizeof(bt_tx_data));
            rc = ble_gattc_notify_custom(connection_handle, ble_sensors_stream_val_handle, txom);
            if( rc == 0)
            {
                ESP_LOGI(TAG,"Most sensors notif sent successfully");
            }
            else 
            {
                ESP_LOGI(TAG,"Error in sending most sensors notif");
            }
        }

        if(bt_others_sensors_stream_en)
        {
            memset(bt_tx_data, 0x00, MAX_BT_TX_BUFF);
            bt_tx_data[0] = STREAM_NOTIFY_OTHERS_SENSORS;
            // Send update to the device
            T_RawColor raw_temp = ColorSensor_GetRaw();
            bt_tx_data[1] = raw_temp.Red & 0xFF;
            bt_tx_data[2] = raw_temp.Red >> 8;
            bt_tx_data[3] = raw_temp.Green & 0xFF;
            bt_tx_data[4] = raw_temp.Green >> 8;
            bt_tx_data[5] = raw_temp.Blue & 0xFF;
            bt_tx_data[6] = raw_temp.Blue >> 8;
            bt_tx_data[7] = raw_temp.Clear & 0xFF;
            bt_tx_data[8] = raw_temp.Clear >> 8;

            bt_tx_data[9] = ColorSensor_GetColor();

            uint16_t prox_temp = GetGroundAmbient(0);
            bt_tx_data[10] = prox_temp&0xFF;
            bt_tx_data[11] = prox_temp>>8;
            prox_temp = GetGroundAmbient(1);
            bt_tx_data[12] = prox_temp&0xFF;
            bt_tx_data[13] = prox_temp>>8;

            prox_temp = GetGroundReflected(0);
            bt_tx_data[14] = prox_temp&0xFF;
            bt_tx_data[15] = prox_temp>>8;
            prox_temp = GetGroundReflected(1);
            bt_tx_data[16] = prox_temp&0xFF;
            bt_tx_data[17] = prox_temp>>8;

            int16_t temp_val = Gyroscope_GetAngleZ_deg();
            bt_tx_data[18] = temp_val&0xFF;
            bt_tx_data[19] = temp_val>>8;

            bt_tx_data[20] = 0;
            if(Gpio_IsTapDetected())
            {
                bt_tx_data[20] |= 0x01;
            }
            if(Gpio_IsFreeFallDetected())
            {
                bt_tx_data[20] |= 0x02;
            }
            if(IS_EVENT(EVENT_MIC))
            {
                bt_tx_data[20] |= 0x04;
            }

            temp_val = GetLeftSpeed();
            bt_tx_data[21] = temp_val&0xFF;
            bt_tx_data[22] = temp_val>>8;

            temp_val = GetRightSpeed();
            bt_tx_data[23] = temp_val&0xFF;
            bt_tx_data[24] = temp_val>>8;

            temp_val = STM32_GetLeftMotorPwm();
            bt_tx_data[25] = temp_val&0xFF;
            bt_tx_data[26] = temp_val>>8;

            temp_val = STM32_GetRightMotorPwm();
            bt_tx_data[27] = temp_val&0xFF;
            bt_tx_data[28] = temp_val>>8;            

            temp_val = STM32_GetBatteryVoltage();
            bt_tx_data[29] = temp_val&0xFF;
            bt_tx_data[30] = temp_val>>8;            

            struct os_mbuf *txom;
            txom = ble_hs_mbuf_from_flat(bt_tx_data, STREAM_NOTIFY_OTHERS_SENSORS_LEN);
            rc = ble_gattc_notify_custom(connection_handle, ble_sensors_stream_val_handle, txom);
            if( rc == 0)
            {
                ESP_LOGI(TAG,"Others sensors notif sent successfully");
            }
            else 
            {
                ESP_LOGI(TAG,"Error in sending others sensors notif");
            }            
        }
     
        if(mp_receiving_script)
        {
            mp_script_timeout++;
            if(mp_script_timeout == 1500) // 50 hz * 30 seconds = 1500
            {
                mp_script_timeout = 0;
                mp_receiving_script = false;
                ble_indicate_python_load(FILE_LOAD_NOT_COMPLETE);
                Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
            }
        }

        if(receiving_audio)
        {
            audio_timeout++;
            if(audio_timeout == 1500) // 50 hz * 30 seconds = 1500
            {
                audio_timeout = 0;
                receiving_audio = false;
                ble_indicate_audio_load(FILE_LOAD_NOT_COMPLETE);
            }
        } 
        
        if(audio_playing)
        {
            if(Codec_IsSoundFinished())
            {
                audio_playing = false;
                ble_indicate_audio_exec(AUDIO_EXEC_OK);
            }
        }

        if(audio_recording)
        {
            if(Codec_IsRecordFinished())
            {
                audio_recording = false;
                audio_ready = true;
                ble_indicate_audio_rec(AUDIO_REC_OK);
            }
        }

        if(fs_receiving_file)
        {
            fs_timeout++;
            if(fs_timeout == 1500) // 50 hz * 30 seconds = 1500
            {
                fs_timeout = 0;
                fs_receiving_file = false;
                ble_indicate_fs(FS_IND_LOAD_RES, FS_LOAD_NOT_COMPLETE);
                Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
            }
        }

        vTaskDelay(20/portTICK_PERIOD_MS); // 50 hz update
    }
    vTaskDelete(NULL);
}

bool ble_ota_ringbuf_init(uint32_t ringbuf_size)
{
    s_ringbuf = xRingbufferCreate(ringbuf_size, RINGBUF_TYPE_BYTEBUF);
    if (s_ringbuf == NULL) {
        return false;
    }

    return true;
}

size_t write_to_ringbuf(const uint8_t *data, size_t size)
{
    BaseType_t done = xRingbufferSend(s_ringbuf, (void *)data, size, (TickType_t)portMAX_DELAY);
    if (done) {
        return size;
    } else {
        return 0;
    }
}

void ota_task(void *arg)
{
    esp_partition_t *partition_ptr = NULL;
    esp_partition_t partition;
    const esp_partition_t *next_partition = NULL;
    static uint8_t ota_task_state = 0;

    uint32_t recv_len = 0;
    uint8_t *data = NULL;
    size_t item_size = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;
    ESP_LOGI(TAG, "ota_task start");

    start_time = esp_timer_get_time();
    notify_sem = xSemaphoreCreateCounting(100, 0);
    xSemaphoreGive(notify_sem);

    if (!ble_ota_ringbuf_init(OTA_RINGBUF_SIZE)) {
        ESP_LOGE(TAG, "%s init ringbuf fail", __func__);
        return;
    }
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "Semaphore and ringbuf init time %lld us", (end_time-start_time));

    start_time = esp_timer_get_time();
    partition_ptr = (esp_partition_t *)esp_ota_get_boot_partition();
    if (partition_ptr == NULL) {
        ESP_LOGE(TAG, "boot partition NULL!\r\n");
        goto OTA_ERROR;
    }
    if (partition_ptr->type != ESP_PARTITION_TYPE_APP) {
        ESP_LOGE(TAG, "esp_current_partition->type != ESP_PARTITION_TYPE_APP\r\n");
        goto OTA_ERROR;
    }
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "esp_ota_get_boot_partition time %lld us", (end_time-start_time));

    start_time = esp_timer_get_time();
    if (partition_ptr->subtype == ESP_PARTITION_SUBTYPE_APP_FACTORY) {
        partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
    } else {
        next_partition = esp_ota_get_next_update_partition(partition_ptr); // Get info from "OTA data" partition
        if (next_partition) {
            partition.subtype = next_partition->subtype;
        } else {
            partition.subtype = ESP_PARTITION_SUBTYPE_APP_OTA_0;
        }
    }
    //printf("next partition subtype = %d\n", partition.subtype);
    partition.type = ESP_PARTITION_TYPE_APP;
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "esp_ota_get_next_update_partition time %lld us", (end_time-start_time));    

    start_time = esp_timer_get_time();
    // Verify that the partition returned by "esp_ota_get_next_update_partition" actually exist and is valid...needed?
    partition_ptr = (esp_partition_t *)esp_partition_find_first(partition.type, partition.subtype, NULL);
    if (partition_ptr == NULL) {
        ESP_LOGE(TAG, "partition NULL!\r\n");
        goto OTA_ERROR;
    }
    memcpy(&partition, partition_ptr, sizeof(esp_partition_t));
    end_time = esp_timer_get_time();
    ESP_LOGI(TAG, "esp_partition_find_first time %lld us", (end_time-start_time));

    while (1)
    {
        switch(ota_task_state)
        {
            case 0: // Prepare the OTA partition once the "start ota" command is received
                // We do not prepare the partition at init because it takes about 5 seconds, this mean 
                // that the robot would take 5 seconds to turn on that is way too much!
                // By setting a larger supervision timeout (8 seconds) it can be done here without
                // losing connection.
                if(start_ota)
                {
                    // Gives time to the gatt request to terminate before start preparing the partition otherwise a BLE error is returned to the connected device.
                    vTaskDelay(500/portTICK_PERIOD_MS); 
                    start_time = esp_timer_get_time();
                    if (esp_ota_begin(&partition, OTA_SIZE_UNKNOWN, &out_handle) != ESP_OK) { // This function takes 4-5 seconds because it erase all the partition.
                        ESP_LOGE(TAG, "esp_ota_begin failed!\r\n");
                        goto OTA_ERROR;
                    }
                    end_time = esp_timer_get_time();
                    ESP_LOGI(TAG, "esp_ota_begin time %lld us", (end_time-start_time));
                    ESP_LOGI(TAG, "wait for data from ringbuf!");
                    recv_len = 0;
                    ota_task_state = 1;
                }
                vTaskDelay(100/portTICK_PERIOD_MS);
                break;

            case 1: // deal with all receive packet
                //data = (uint8_t *)xRingbufferReceive(s_ringbuf, &item_size, (TickType_t)portMAX_DELAY);

                data = (uint8_t *)xRingbufferReceive(s_ringbuf, &item_size, pdMS_TO_TICKS(50));

                if(data != NULL) 
                {
                    xSemaphoreTake(notify_sem, portMAX_DELAY);

                    ESP_LOGI(TAG, "recv: %u, recv_total:%"PRIu32"\n", item_size, recv_len + item_size);

                    if (item_size != 0) {
                        if (esp_ota_write(out_handle, (const void *)data, item_size) != ESP_OK) {
                            ESP_LOGE(TAG, "esp_ota_write failed!\r\n");
                            esp_ota_abort(out_handle);
                            start_ota = false;
                            ota_task_state = 0;
                        }

                        recv_len += item_size;
                        vRingbufferReturnItem(s_ringbuf, (void *)data);

                        if (recv_len >= ota_total_len) {
                            xSemaphoreGive(notify_sem);

                            if (esp_ota_end(out_handle) != ESP_OK) {
                                ESP_LOGE(TAG, "esp_ota_end failed!\r\n");
                                esp_ota_abort(out_handle);
                                start_ota = false;
                                ota_task_state = 0;
                            }

                            if (esp_ota_set_boot_partition(&partition) != ESP_OK) {
                                ESP_LOGE(TAG, "esp_ota_set_boot_partition failed!\r\n");
                                start_ota = false;
                                esp_ota_abort(out_handle);
                                ota_task_state = 0;
                            }

                            vSemaphoreDelete(notify_sem);
                            esp_restart();
                        }
                    }
                    xSemaphoreGive(notify_sem);
                }

                if(!start_ota) // Device disconnected
                {
                    esp_ota_abort(out_handle);
                    start_ota = false;
                    ota_task_state = 0;
                    break;
                }       
                break;
        }

    }

OTA_ERROR:
    ESP_LOGE(TAG, "OTA failed");
    vTaskDelete(NULL);
}

void ble_indicate_python_exec(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {PYTHON_IND_EXEC_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_python_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"PY exec indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"PY exec error in sending indication");
    }
    if(value == PYTHON_EXEC_OK)
    {
        Codec_PlayOnboardSound(TONE_TYPE_CODEEXECSUCESS);
    }
    else
    {
        Codec_PlayOnboardSound(TONE_TYPE_CODEERROR);
    }
}

void ble_indicate_python_load(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {PYTHON_IND_LOAD_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_python_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"PY load indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"PY load error in sending indication");
    }
}

void ble_indicate_python_save(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {PYTHON_IND_SAVE_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_python_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"PY save indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"PY save error in sending indication");
    }
}

void ble_indicate_audio_exec(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {AUDIO_IND_EXEC_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_audio_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"Audio exec indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"Audio exec error in sending indication");
    }
}

void ble_indicate_audio_load(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {AUDIO_IND_LOAD_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_audio_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"Audio load indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"Audio load error in sending indication");
    }    
}

void ble_indicate_audio_rec(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {AUDIO_IND_REC_RES, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_audio_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"Audio rec indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"Audio rec error in sending indication");
    }   
}

void ble_indicate_fs(uint8_t type, uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {type, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_fs_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"FS indication sent successfully (%d,%d)", type, value);
    }
    else 
    {
        ESP_LOGI(TAG,"FS error in sending indication (%d,%d)", type, value);
    }
}

void ble_indicate_fs_list_err(void)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[1] = {FS_IND_LIST_RES};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_indicate_custom(connection_handle, ble_fs_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"FS list error indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"FS list error in sending indication");
    }
}

void ble_indicate_fs_list(uint8_t *data, uint16_t len)
{
    int rc = 0;
    uint16_t bytes_sent = 0;
    uint16_t seq_id = 0;
    uint8_t *current_data_ptr = data;
    uint16_t remaining_len = len;
    uint8_t *ind_buf_data = NULL;

    // Check the integrity of the data once all the script is received.
    calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)data, len);
    calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
    ESP_LOGI(TAG, "fs list calc crc=%x", calculated_crc);

    // --- Maximum Data Payload Calculation ---
    // Max Payload for First Packet: 500 - 9 = 491 bytes
    const uint16_t MAX_PAYLOAD_FIRST = FS_MAX_TOTAL_CHUNK_SIZE - FS_LIST_FIRST_PACKET_HEADER_LEN;
    // Max Payload for Subsequent Packets: 500 - 2 = 498 bytes
    const uint16_t MAX_PAYLOAD_SUBSEQUENT = FS_MAX_TOTAL_CHUNK_SIZE - FS_SUBSEQUENT_PACKET_HEADER_LEN;

    while (remaining_len > 0)
    {
        uint16_t chunk_payload_len;
        struct os_mbuf *txom;

        if (seq_id == 0)
        {
            // --- First Packet (seq_id = 0) ---

            // Determine the payload length for this chunk
            chunk_payload_len = (remaining_len > MAX_PAYLOAD_FIRST) ? MAX_PAYLOAD_FIRST : remaining_len;

            uint16_t total_chunk_len = FS_LIST_FIRST_PACKET_HEADER_LEN + chunk_payload_len;
            ind_buf_data = malloc(total_chunk_len); // ind_buf_data alloc #1
            if(!ind_buf_data)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for first chunk data buffer");
                ble_indicate_fs_list_err(); // Tell the client there was an error, this is ok here since the transfering of the json is not started yet (no confusion between FS_IND_LIST_RES and sequence id)
                mp_list_files_free_buffer();
                return;
            }
            ESP_LOGI(TAG, "Preparing first chunk: Total Chunk Len %d, Payload Len %d", total_chunk_len, chunk_payload_len);
            // Allocate mbuf for the first chunk: header (11) + data (up to 489)
            txom = ble_hs_mbuf_from_flat(ind_buf_data, total_chunk_len);
            if (!txom)
            {
                ESP_LOGE(TAG, "Failed to allocate mbuf for first chunk");
                ble_indicate_fs_list_err(); // Tell the client there was an error, this is ok here since the transfering of the json is not started yet (no confusion between FS_IND_LIST_RES and sequence id)
                mp_list_files_free_buffer();
                free(ind_buf_data); // free ind_buf_data alloc #1
                ind_buf_data = NULL;
                return;
            }

            // 1. ID = 0x04 (1 byte) -> Offset 0
            uint8_t id = FS_IND_LIST_RES;
            os_mbuf_copyinto(txom, 0, &id, 1);

            // 2. size (2 bytes) - Total length of the original data (len) -> Offset 1
            os_mbuf_copyinto(txom, 1, &len, 2);

            // 3. CRC32 (4 bytes) -> Offset 3
            os_mbuf_copyinto(txom, 3, &calculated_crc, 4);

            // 4. seq id (2 bytes) - must be 0 for the first packet -> Offset 7
            os_mbuf_copyinto(txom, 7, &seq_id, 2);

            // 5. Data payload -> Offset 9 (header_len)
            os_mbuf_copyinto(txom, FS_LIST_FIRST_PACKET_HEADER_LEN, current_data_ptr, chunk_payload_len);
        }
        else
        {
            // --- Subsequent Packets (seq_id > 0) ---

            // Determine the payload length for this chunk
            chunk_payload_len = (remaining_len > MAX_PAYLOAD_SUBSEQUENT) ? MAX_PAYLOAD_SUBSEQUENT : remaining_len;

            uint16_t total_chunk_len = FS_SUBSEQUENT_PACKET_HEADER_LEN + chunk_payload_len;
            ind_buf_data = malloc(total_chunk_len); // ind_buf_data alloc #2
            if(!ind_buf_data)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for chunk %d", seq_id);
                //ble_indicate_fs_list_err(); // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
                mp_list_files_free_buffer();
                return;
            }            
            ESP_LOGI(TAG, "Preparing chunk %d: Total Chunk Len %d, Payload Len %d", seq_id, total_chunk_len, chunk_payload_len);
            // Allocate mbuf for the subsequent chunk: header (2) + data (up to 498)
            txom = ble_hs_mbuf_from_flat(ind_buf_data, total_chunk_len);
            if (!txom)
            {
                ESP_LOGE(TAG, "Failed to allocate mbuf for chunk %d", seq_id);
                //ble_indicate_fs_list_err(); // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
                mp_list_files_free_buffer();
                free(ind_buf_data); // free ind_buf_data alloc #2
                ind_buf_data = NULL;
                return;
            }

            // 1. seq id (2 bytes) - incremented by one -> Offset 0
            os_mbuf_copyinto(txom, 0, &seq_id, 2);

            // 2. Data payload -> Offset 2 (header_len)
            os_mbuf_copyinto(txom, FS_SUBSEQUENT_PACKET_HEADER_LEN, current_data_ptr, chunk_payload_len);
        }

        // Send the indication. NimBLE takes ownership of txom on success (rc == 0).
        rc = ble_gattc_indicate_custom(connection_handle, ble_fs_val_handle, txom);
        
        free(ind_buf_data); // free ind_buf_data alloc #1 or alloc #2
        ind_buf_data = NULL;

        if (rc == 0)
        {
            ESP_LOGI(TAG, "FS list indication sent successfully: Seq ID %d, Total Len %d, Payload %d",
                     seq_id, os_mbuf_len(txom), chunk_payload_len);

            // Update state for the next chunk
            current_data_ptr += chunk_payload_len;
            remaining_len -= chunk_payload_len;
            bytes_sent += chunk_payload_len;
            seq_id++; // Increment sequence ID for the next packet
        }
        else
        {
            ESP_LOGE(TAG, "FS list error in sending indication: Seq ID %d, rc=%d. Freeing mbuf.", seq_id, rc);
            // Must free the mbuf chain manually on failure, as NimBLE did not take ownership.
            os_mbuf_free_chain(txom);
            //ble_indicate_fs_list_err();   // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
            mp_list_files_free_buffer();
            return; // Stop sending on first failure
        }
    }

    if (rc == 0)
    {
        ESP_LOGI(TAG, "FS list indication fully sent: %d total data bytes in %d packets", len, seq_id);
    }
    mp_list_files_free_buffer();
}

void ble_indicate_dev_info(uint8_t *data, uint16_t len)
{
    int rc = 0;
    struct os_mbuf *txom;
    txom = ble_hs_mbuf_from_flat(data, len);
    rc = ble_gattc_indicate_custom(connection_handle, ble_dev_info_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"Device info indication sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"Device info error in sending indication");
    }
}

void ble_indicate_download(uint8_t *data, uint32_t len)
{
    int rc = 0;
    uint16_t bytes_sent = 0;
    uint16_t seq_id = 0;
    uint8_t *current_data_ptr = data;
    uint32_t remaining_len = len;
    uint8_t *ind_buf_data = NULL;
    EventBits_t bits = 0;
    //uint8_t send_trials = 0;
    // Check the integrity of the data once all the script is received.
    calculated_crc = esp_rom_crc32_be(initial_crc, (uint8_t*)data, len);
    calculated_crc = ~calculated_crc; // The final CRC value needs to be bitwise NOT-ed to get the standard CRC32 result.
    ESP_LOGI(TAG, "fs download calc crc=%x", calculated_crc);

    // --- Maximum Data Payload Calculation ---
    // Max Payload for First Packet: 500 - 11 = 489 bytes
    const uint16_t MAX_PAYLOAD_FIRST = FS_MAX_TOTAL_CHUNK_SIZE - FS_DOWNLOAD_FIRST_PACKET_HEADER_LEN;
    // Max Payload for Subsequent Packets: 500 - 2 = 498 bytes
    const uint16_t MAX_PAYLOAD_SUBSEQUENT = FS_MAX_TOTAL_CHUNK_SIZE - FS_SUBSEQUENT_PACKET_HEADER_LEN;

    while (remaining_len > 0)
    {
        uint16_t chunk_payload_len;
        struct os_mbuf *txom;

        if (seq_id == 0)
        {
            // --- First Packet (seq_id = 0) ---

            // Determine the payload length for this chunk
            chunk_payload_len = (remaining_len > MAX_PAYLOAD_FIRST) ? MAX_PAYLOAD_FIRST : remaining_len;

            uint16_t total_chunk_len = FS_DOWNLOAD_FIRST_PACKET_HEADER_LEN + chunk_payload_len;
            ind_buf_data = malloc(total_chunk_len); // ind_buf_data alloc #1
            if(!ind_buf_data)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for first chunk data buffer");
                ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_ERROR); // Tell the client there was an error, this is ok here since the transfering of the file is not started yet (no confusion between FS_IND_DOWNLOAD_RES and sequence id)
                mp_read_file_free_buffer();
                return;
            }
            ESP_LOGI(TAG, "Preparing first chunk: Total Chunk Len %d, Payload Len %d", total_chunk_len, chunk_payload_len);
            // Allocate mbuf for the first chunk: header (11) + data (up to 489)
            txom = ble_hs_mbuf_from_flat(ind_buf_data, total_chunk_len);
            if (!txom)
            {
                ESP_LOGE(TAG, "Failed to allocate mbuf for first chunk");
                ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_ERROR);  // Tell the client there was an error, this is ok here since the transfering of the file is not started yet (no confusion between FS_IND_DOWNLOAD_RES and sequence id)
                mp_read_file_free_buffer();
                free(ind_buf_data); // free ind_buf_data alloc #1
                ind_buf_data = NULL;
                return;
            }

            // 1. ID (1 byte) -> Offset 0
            uint8_t id = FS_IND_DOWNLOAD_DATA;
            os_mbuf_copyinto(txom, 0, &id, 1);

            // 2. size (4 bytes) - Total length of the data (len) -> Offset 1
            os_mbuf_copyinto(txom, 1, &len, 4);

            // 3. CRC32 (4 bytes) -> Offset 5
            os_mbuf_copyinto(txom, 5, &calculated_crc, 4);

            // 4. seq id (2 bytes) - must be 0 for the first packet -> Offset 9
            os_mbuf_copyinto(txom, 9, &seq_id, 2);

            // 5. Data payload -> Offset 11 (header_len)
            os_mbuf_copyinto(txom, FS_DOWNLOAD_FIRST_PACKET_HEADER_LEN, current_data_ptr, chunk_payload_len);
        }
        else
        {
            // --- Subsequent Packets (seq_id > 0) ---

            // Determine the payload length for this chunk
            chunk_payload_len = (remaining_len > MAX_PAYLOAD_SUBSEQUENT) ? MAX_PAYLOAD_SUBSEQUENT : remaining_len;

            uint16_t total_chunk_len = FS_SUBSEQUENT_PACKET_HEADER_LEN + chunk_payload_len;
            ind_buf_data = malloc(total_chunk_len); // ind_buf_data alloc #2
            if(!ind_buf_data)
            {
                ESP_LOGE(TAG, "Failed to allocate memory for chunk %d", seq_id);
                //ble_indicate_fs_list_err(); // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
                mp_read_file_free_buffer();
                return;
            }            
            ESP_LOGI(TAG, "Preparing chunk %d: Total Chunk Len %d, Payload Len %d", seq_id, total_chunk_len, chunk_payload_len);
            // Allocate mbuf for the subsequent chunk: header (2) + data (up to 498)
            txom = ble_hs_mbuf_from_flat(ind_buf_data, total_chunk_len);
            if (!txom)
            {
                ESP_LOGE(TAG, "Failed to allocate mbuf for chunk %d", seq_id);
                //ble_indicate_fs_list_err(); // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
                mp_read_file_free_buffer();
                free(ind_buf_data); // free ind_buf_data alloc #2
                ind_buf_data = NULL;
                return;
            }

            // 1. seq id (2 bytes) - incremented by one -> Offset 0
            os_mbuf_copyinto(txom, 0, &seq_id, 2);

            // 2. Data payload -> Offset 2 (header_len)
            os_mbuf_copyinto(txom, FS_SUBSEQUENT_PACKET_HEADER_LEN, current_data_ptr, chunk_payload_len);
        }

        xEventGroupClearBits(ind_ack_event_group, INDICATION_ACK_BIT|INDICATION_ERR_BIT);
        // Send the indication. NimBLE takes ownership of txom on success (rc == 0).
        rc = ble_gattc_indicate_custom(connection_handle, ble_fs_val_handle, txom);
        bits = xEventGroupWaitBits(ind_ack_event_group, INDICATION_ACK_BIT|INDICATION_ERR_BIT, pdTRUE, pdFALSE, pdMS_TO_TICKS(5000)); // Wait for the indication ack or timeout after 5 seconds   
        free(ind_buf_data); // free ind_buf_data alloc #1 or alloc #2
        ind_buf_data = NULL;

        if (bits & INDICATION_ERR_BIT) 
        {
            vTaskDelay(1000/portTICK_PERIOD_MS); // Small delay to avoid busy loop
            /*
            send_trials++;
            if(send_trials >= 3)
            {
                rc = -1; // Force error after 3 trials
            }
            else
            {
                continue; // Retry sending the same packet
            }
            */
        }
        if (rc == 0)
        {
            //ESP_LOGI(TAG, "FS download indication sent successfully: Seq ID %d, Total Len %d, Payload %d",
            //         seq_id, os_mbuf_len(txom), chunk_payload_len);

            // Update state for the next chunk
            current_data_ptr += chunk_payload_len;
            remaining_len -= chunk_payload_len;
            bytes_sent += chunk_payload_len;
            seq_id++; // Increment sequence ID for the next packet
            //send_trials = 0; // Reset send trials for the next packet
        }
        else
        {
            ESP_LOGE(TAG, "FS download error in sending indication: Seq ID %d, rc=%d. Freeing mbuf.", seq_id, rc);
            // Must free the mbuf chain manually on failure, as NimBLE did not take ownership.
            os_mbuf_free_chain(txom);
            //ble_indicate_fs_list_err();   // Do not indicate error to the client here, it could be confused with sequence ids. The client will notice the missing packets by itself with a timeout.
            mp_read_file_free_buffer();
            return; // Stop sending on first failure
        }
    }

    if (rc == 0)
    {
        ESP_LOGI(TAG, "FS download indication fully sent: %d total data bytes in %d packets", len, seq_id);
    }
    mp_read_file_free_buffer();
}


/**
 * @brief Fragments and sends a data buffer over BLE indication.
 *
 * @param data Pointer to the data to send.
 * @param len Length of the data to send.
 * @param tx_buf A temporary buffer used for holding fragments. Must be at least STDOUT_MAX_CHUNK_SIZE bytes.
 */
static void send_ble_data_fragmented(uint8_t *data, size_t len, uint8_t *tx_buf)
{
    if (!is_connect || len == 0) {
        return;
    }

    size_t remaining = len;
    size_t offset = 0;

    while (remaining > 0 && is_connect) {
        size_t chunk_size = (remaining > STDOUT_MAX_CHUNK_SIZE) ? STDOUT_MAX_CHUNK_SIZE : remaining;

        // Copy the chunk into our tx_buf (fragment buffer)
        memcpy(tx_buf, data + offset, chunk_size);
        
        struct os_mbuf *txom = ble_hs_mbuf_from_flat(tx_buf, chunk_size);
        if (!txom) {
            ESP_LOGE(TAG, "Failed to create mbuf for stdout");
            break; // Stop trying to send this item
        }

        // Send as indication
        int rc = ble_gattc_indicate_custom(connection_handle, ble_python_stdout_val_handle, txom);
        
        if (rc == 0) {
            // Success, NimBLE takes ownership of txom
            ESP_LOGD(TAG, "Sent %d bytes of stdout", chunk_size);
        } else {
            // Failure, we must free txom
            os_mbuf_free_chain(txom);
            ESP_LOGE(TAG, "Error sending stdout indication: %d", rc);
            // If we're disconnected, is_connect will be false and we'll break
            // Otherwise, we'll just drop this packet
            break; 
        }

        remaining -= chunk_size;
        offset += chunk_size;
        
        // Small delay to avoid flooding.
        vTaskDelay(20 / portTICK_PERIOD_MS); // This is especially needed for cases including large and often prints
    }
    ESP_LOGI(TAG, "Sent total %d bytes of stdout", len);
}

/**
 * @brief FreeRTOS task to empty the stdout ring buffer and send over BLE.
 *
 * This task waits for data to appear in s_stdout_ringbuf, retrieves it, and sends it as a BLE notification.
 */

static void ble_stdout_task(void *pvParameters) 
{
    uint8_t *data = NULL;
    size_t item_size = 0;
    
    // This buffer is for accumulating data from the ring buffer. 
    // From various test it was noticed that often many small size indications are sent to the PC.
    // Since there is a limited number of buffer in the low level pool, then some data are often lost.
    // To avoid data loss, a temporary buffer is used: send indication only when either the buffer is filled 
    // with at least 100 bytes or no new data are received within 1 second.
    // Data flow: Python output => ringbuffer => temporary buffer => BLE
    uint8_t *accum_buf = malloc(STDOUT_ACCUM_BUF_SIZE);
    if (!accum_buf) {
        ESP_LOGE(TAG, "Failed to alloc stdout accum_buf!");
        vTaskDelete(NULL);
        return;
    }

    // This buffer is for holding fragments during BLE sending
    uint8_t *fragment_buf = malloc(STDOUT_MAX_CHUNK_SIZE);
    if (!fragment_buf) {
        ESP_LOGE(TAG, "Failed to alloc stdout fragment_buf!");
        free(accum_buf);
        vTaskDelete(NULL);
        return;
    }

    size_t accum_len = 0;
    bool send_now = false;

    while(1) {
        
        // If we are not connected, flush the buffer and wait
        if (!is_connect) {
            accum_len = 0; // Flush buffer
            vTaskDelay(500 / portTICK_PERIOD_MS);
            continue;
        }

        // Wait for data to arrive in the ring buffer
        data = (uint8_t *)xRingbufferReceive(s_stdout_ringbuf, &item_size, pdMS_TO_TICKS(STDOUT_SEND_TIMEOUT_MS));

        send_now = false;

        if (data != NULL) {
            // We got new data.

            // Check if this item *by itself* is too big for the buffer
            if (item_size > STDOUT_ACCUM_BUF_SIZE) {
                ESP_LOGW(TAG, "Single stdout item (%d) > accum_buf (%d). Sending previous and this one.", item_size, STDOUT_ACCUM_BUF_SIZE);
                // Send what we have *now*
                send_ble_data_fragmented(accum_buf, accum_len, fragment_buf);
                // Send the new, large item directly
                send_ble_data_fragmented(data, item_size, fragment_buf);
                // Reset buffer
                accum_len = 0;
                vRingbufferReturnItem(s_stdout_ringbuf, (void *)data);
                continue; // Skip to next loop iteration
            }

            // Check if adding this item will *overflow* the buffer
            if (accum_len + item_size > STDOUT_ACCUM_BUF_SIZE) {
                // Buffer will overflow. Send what we have *first*.
                send_now = true;
            } else {
                // It fits. Add it to the buffer.
                memcpy(accum_buf + accum_len, data, item_size);
                accum_len += item_size;
                vRingbufferReturnItem(s_stdout_ringbuf, (void *)data);
                data = NULL; // Mark as processed
            }

        } else {
            // data is NULL. This means our 1-second timer has expired.
            if (accum_len > 0) { // Send the accumulated data if any
                send_now = true;
            }
        }

        // Check send conditions
        if (!send_now && accum_len >= STDOUT_SEND_THRESHOLD_BYTES) {
            // We hit the 100-byte threshold
            send_now = true;
        }
        
        // Send if required
        if (send_now && is_connect) {
            send_ble_data_fragmented(accum_buf, accum_len, fragment_buf);
            accum_len = 0; // Reset buffer
        }

        // Handle the item that caused the overflow (the one that doesn't fit within the accumulation buffer) if any
        if (data != NULL) {
            // data still holds the item that didn't fit.
            // Start the new buffer with it.
            memcpy(accum_buf, data, item_size);
            accum_len = item_size;
            vRingbufferReturnItem(s_stdout_ringbuf, (void *)data);
        }

        vTaskDelay(10 / portTICK_PERIOD_MS); // This is needed for yielding to other tasks (including WDT) for cases with heavy prints (fast loops what print continuously)
    }
    
    // Should never be reached
    free(accum_buf);
    free(fragment_buf);
    vTaskDelete(NULL);
}

/**
 * @brief Initializes the BLE stdout service.
 * * This creates the ring buffer and spawns the task that
 * sends data from the buffer over BLE.
 */
void ble_spp_stdout_init(void)
{
    // Create the ring buffer for stdout
    s_stdout_ringbuf = xRingbufferCreate(STDOUT_RINGBUF_SIZE, RINGBUF_TYPE_NOSPLIT);
    if (s_stdout_ringbuf == NULL) {
        ESP_LOGE(TAG, "Failed to create stdout ring buffer!");
        return; // Or assert
    }

    // Spawn the task that sends stdout over BLE
    xTaskCreate(&ble_stdout_task, "ble_stdout_task", 5120, NULL, 1, NULL); // Give low priority to BLE stdout task to avoid interference with other tasks
    
    ESP_LOGI(TAG, "BLE Stdout Service Initialized");
}

size_t ble_spp_stdout_write(const void *data, size_t size)
{
    if (s_stdout_ringbuf == NULL) {
        return 0; // Service not initialized
    }
    //ESP_LOGI(TAG, "DUP TERM WRITE");
    //ESP_LOG_BUFFER_CHAR(TAG, data, size);
    // Non-blocking (small timeout) write to the ring buffer
    BaseType_t rc = xRingbufferSend(s_stdout_ringbuf, data, size, pdMS_TO_TICKS(10));
    
    if (rc != pdTRUE) {
        // Buffer was full, we dropped the data
        return 0; 
    }
    
    return size; // Success
}

void ble_spp_init(void)
{
    int rc;
    char ble_name[32];
    uint8_t mac_addr[6] = {0};
    
    uint32_t blk3_rdata2;
    uint32_t blk3_rdata6;
    uint32_t blk3_rdata7;
    uint8_t efuse_data[12];
    char lot[3] = {0};
    uint16_t pcb_id = 0;

    ind_ack_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_nimble_hci_and_controller_init());

    nimble_port_init();

    /* Initialize the NimBLE host configuration. */
    ble_hs_cfg.reset_cb = ble_spp_server_on_reset;
    ble_hs_cfg.sync_cb = ble_spp_server_on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /*
    ble_hs_cfg.sm_io_cap = CONFIG_EXAMPLE_IO_TYPE;
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_bonding = 1;
#endif
#ifdef CONFIG_EXAMPLE_MITM
    ble_hs_cfg.sm_mitm = 1;
#endif
#ifdef CONFIG_EXAMPLE_USE_SC
    ble_hs_cfg.sm_sc = 1;
#else
    ble_hs_cfg.sm_sc = 0;
#endif
#ifdef CONFIG_EXAMPLE_BONDING
    ble_hs_cfg.sm_our_key_dist = 1;
    ble_hs_cfg.sm_their_key_dist = 1;
#endif
*/

    rc = gatt_svr_init();
    ESP_LOGI("BLE_SPP", "Init failed with return code: %d", rc);
    assert(rc == 0);

    /* Set the default device name. */
    // The robot ID is stored in the first 96 bits (12 bytes) of EFUSE BLK3.
    //
    // Each entry is now 4 bytes:
    //   - 2 bytes: lot letters (ASCII), e.g. 'B''A'
    //   - 2 bytes: PCB ID number (binary uint16_t), e.g. 1
    //
    // Total layout:
    //   Entry 0 -> bytes  0..3
    //   Entry 1 -> bytes  4..7
    //   Entry 2 -> bytes  8..11
    //
    // An invalid entry is identified by:
    //   letters = 0xFFFF AND pcb_id = 0xFFFF
    //
    // Old entries should therefore be overwritten with all bits set to 1.
    //
    // Example valid ID:
    //   "BA0001"    
    // Read first 96 bits from BLK3
    blk3_rdata0 = REG_READ(EFUSE_BLK3_RDATA0_REG);
    blk3_rdata1 = REG_READ(EFUSE_BLK3_RDATA1_REG);
    blk3_rdata2 = REG_READ(EFUSE_BLK3_RDATA2_REG);
    // Convert words into byte array (little endian)
    memcpy(&efuse_data[0], &blk3_rdata0, 4);
    memcpy(&efuse_data[4], &blk3_rdata1, 4);
    memcpy(&efuse_data[8], &blk3_rdata2, 4);
    // Search for first valid entry
    for(int i = 0; i < 3; i++) {
        uint8_t *entry = &efuse_data[i * 4];
        uint16_t lot_raw = entry[0] | (entry[1] << 8);
        uint16_t id_raw  = entry[2] | (entry[3] << 8);
        // Valid entry found
        if((lot_raw != 0xFFFF) && (id_raw != 0xFFFF)) {
            lot[0] = entry[0];
            lot[1] = entry[1];
            lot[2] = '\0';
            pcb_id = id_raw;
            break;
        }
    }
    snprintf(ble_name, sizeof(ble_name), "THYMIO-%s%04u", lot, pcb_id);
    
    /*
    rc = esp_read_mac(mac_addr, ESP_MAC_BT); // Get the Bluetooth MAC address
    if (rc == ESP_OK) {
        // Format the name string. The last two bytes of the MAC are used here.
        snprintf(ble_name, sizeof(ble_name), "THYMIO-%02X%02X", mac_addr[4], mac_addr[5]);
    } else {
        // Handle the error if reading the MAC fails.
        // Set a default name as a fallback.
        strcpy(ble_name, "THYMIO-UNKNOWN");
    }
    */
    rc = ble_svc_gap_device_name_set(ble_name);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(ble_spp_server_host_task);

    xTaskCreate(&bt_rx_tx_task, "bt_rx_tx_task", 3072, NULL, 5, NULL);
    xTaskCreate(&ota_task, "ota_task", OTA_TASK_SIZE, NULL, 5, NULL);

    ble_spp_stdout_init();
}
