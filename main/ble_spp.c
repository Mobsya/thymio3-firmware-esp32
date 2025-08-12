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
/* BLE */
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "console/console.h"
#include "services/gap/ble_svc_gap.h"
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
#include "mp_component.h"

static const char *TAG = "THYMIO_BLUETOOTH";

#define MAX_BT_RX_BUFF (CMD_SET_MOST_ACTUATORS_LEN)
#define MAX_BT_TX_BUFF (RSP_MOST_SENSORS_LEN)

uint8_t bt_rx_data[MAX_BT_RX_BUFF]; // Received commands from the device (e.g. from phone)
uint8_t bt_rx_data_temp[MAX_BT_RX_BUFF]; // Double buffer for parsing the data while receiving new data without corruption
uint8_t bt_tx_data[MAX_BT_TX_BUFF]; // Data sent to the device (e.g. to phone)
bool bt_cmd_received = false;
uint8_t bt_cmd_len = 0;
bool bt_sensors_stream_en = false;

static int ble_spp_server_gap_event(struct ble_gap_event *event, void *arg);
static uint8_t own_addr_type;
static bool is_connect = false;
uint16_t connection_handle;
static uint16_t ble_sensors_stream_val_handle;
static uint16_t ble_python_val_handle;
char mp_script[MAX_MP_SCRIPT_LEN];
uint16_t mp_script_tot_len = 0;
uint16_t mp_script_curr_len = 0;
bool mp_receiving_script = false;

void ble_store_config_init(void);

/**
 * Logs information about a connection to the console.
 */
static void
ble_spp_server_print_conn_desc(struct ble_gap_conn_desc *desc)
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
        BLE_UUID16_INIT(BLE_SVC_SPP_UUID16)
    };
    fields.num_uuids16 = 1;
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
        }
        MODLOG_DFLT(INFO, "\n");
        if (event->connect.status != 0) {
            /* Connection failed; resume advertising. */
            ble_spp_server_advertise();
        }
        enter_micropython_mode();
        return 0;

    case BLE_GAP_EVENT_DISCONNECT:
        MODLOG_DFLT(INFO, "disconnect; reason=%d ", event->disconnect.reason);
        ble_spp_server_print_conn_desc(&event->disconnect.conn);
        MODLOG_DFLT(INFO, "\n");

        /* Connection terminated; resume advertising. */
        ble_spp_server_advertise();
        turnOffAllSensors();
        exit_micropython_mode();
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

    case BLE_GAP_EVENT_MTU:
        MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d cid=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.channel_id,
                    event->mtu.value);
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
                ESP_LOGI(TAG, "CMD buf len = %d (%d)", ctxt->om->om_len, ctxt->om->om_pkthdr_len);
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
                ESP_LOGI(TAG, "PY buf len = %d (%d)", ctxt->om->om_len, ctxt->om->om_pkthdr_len);
                ESP_LOG_BUFFER_HEX(TAG, ctxt->om->om_data, ctxt->om->om_len);
                if(mp_receiving_script)
                {
                    // copy remaining chunk of data
                    memcpy(mp_script + mp_script_curr_len, &ctxt->om->om_data[0], ctxt->om->om_len);
                    mp_script_curr_len += ctxt->om->om_len;
                    if(mp_script_curr_len == mp_script_tot_len)
                    {
                        mp_receiving_script = false;
                    }                    
                }
                else
                {
                    if(ctxt->om->om_data[0] == CMD_LOAD_SCRIPT)
                    {
                        mp_script_tot_len = (ctxt->om->om_data[1] << 8) | ctxt->om->om_data[2];
                        memset(mp_script, 0x0, MAX_MP_SCRIPT_LEN);
                        mp_script_curr_len = 0;
                        mp_receiving_script = true;
                        // copy first chunk of data
                        memcpy(mp_script + mp_script_curr_len, &ctxt->om->om_data[3], ctxt->om->om_len-3);
                        mp_script_curr_len += ctxt->om->om_len - 3;
                        if(mp_script_curr_len == mp_script_tot_len)
                        {
                            mp_receiving_script = false;
                        }

                    } else if(ctxt->om->om_data[0] == CMD_EXEC_SCRIPT)
                    {
                        mp_exec_script_from_ram(mp_script);

                    } else if(ctxt->om->om_data[0] == CMD_STOP_SCRIPT)
                    {
                        mp_stop_script();
                    }
                    break;         
                }

            case BLE_GATT_ACCESS_OP_READ_CHR:
                //MODLOG_DFLT(INFO, "Callback for read");
                break;

            default:
                //MODLOG_DFLT(INFO, "\nDefault Callback");
                break;
        }
    } 
    return 0;    

}

/* Define new custom service */
static const struct ble_gatt_svc_def new_ble_svc_gatt_defs[] = {
    {
        /*** Service: SPP */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(BLE_SVC_SPP_UUID16),
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
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                /* sensors stream characteristic (from Thymio to device) */
                .uuid = BLE_UUID16_DECLARE(BLE_SVC_PYTHON_CHR_UUID16),
                .access_cb = ble_svc_gatt_handler,
                .val_handle = &ble_python_val_handle,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
            }, {
                0, /* No more characteristics */
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
                case CMD_SET_MOST_ACTUATORS:
                    if(bt_cmd_len == CMD_SET_MOST_ACTUATORS_LEN) // Check correct size is received
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
                    }
                    break;

                case CMD_SET_OTHERS_ACTUATORS:
                    if(bt_cmd_len == CMD_SET_OTHERS_ACTUATORS_LEN) // Check correct size is received
                    {
                        memcpy(bt_rx_data_temp, bt_rx_data, bt_cmd_len);
                    }                
                    break;

                case CMD_SETUP_NOTIF:
                    if(bt_cmd_len == CMD_SETUP_NOTIF_LEN) // Check correct size is received
                    {
                        if((bt_rx_data[1] & 0x01) == 0x01) // Enable sensors stream
                        {
                            bt_sensors_stream_en = true;
                        }
                        else // Disable sensors stream
                        {
                            bt_sensors_stream_en = false;
                        }
                    }                 
                    break;

                default:
                    break;
            }
        }

        if(bt_sensors_stream_en)
        {
            bt_tx_data[0] = RSP_MOST_SENSORS;
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

            //uint8_t* status_temp = Buttons_GetStatus()
            // bt_tx_data[21] = ...

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
                ESP_LOGI(TAG,"Notification sent successfully");
            }
            else 
            {
                ESP_LOGI(TAG,"Error in sending notification");
            }
        }
     
        vTaskDelay(20/portTICK_PERIOD_MS); // 50 hz update
    }
    vTaskDelete(NULL);
}

void ble_notify_python_end(uint8_t value)
{
    int rc = 0;
    struct os_mbuf *txom;
    uint8_t temp[2] = {RSP_SCRIPT_FINISH, value};
    txom = ble_hs_mbuf_from_flat(temp, sizeof(temp));
    rc = ble_gattc_notify_custom(connection_handle, ble_python_val_handle, txom);
    if( rc == 0)
    {
        ESP_LOGI(TAG,"Notification sent successfully");
    }
    else 
    {
        ESP_LOGI(TAG,"Error in sending notification");
    }
}

void ble_spp_init(void)
{
    int rc;
    char ble_name[32];
    uint8_t mac_addr[6] = {0};

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
    assert(rc == 0);

    /* Set the default device name. */
    rc = esp_read_mac(mac_addr, ESP_MAC_BT); // Get the Bluetooth MAC address
    if (rc == ESP_OK) {
        // Format the name string. The last two bytes of the MAC are used here.
        snprintf(ble_name, sizeof(ble_name), "THYMIO-%02X%02X", mac_addr[4], mac_addr[5]);
    } else {
        // Handle the error if reading the MAC fails.
        // Set a default name as a fallback.
        strcpy(ble_name, "THYMIO-UNKNOWN");
    }
    rc = ble_svc_gap_device_name_set(ble_name);
    assert(rc == 0);

    /* XXX Need to have template for store */
    ble_store_config_init();

    nimble_port_freertos_init(ble_spp_server_host_task);

    xTaskCreate(&bt_rx_tx_task, "bt_rx_tx_task", 4096, NULL, 5, NULL);
}
