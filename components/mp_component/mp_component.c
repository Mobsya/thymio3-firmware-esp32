/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "mp_component.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/objstr.h"
#include "py/builtin.h"
#include "py/stackctrl.h"
#include "py/nlr.h"
#include "py/compile.h"
#include "py/persistentcode.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "py/mpstate.h"
#include "py/qstr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_task.h"
#include "soc/cpu.h"
#include "esp_log.h"

//#include "py/obj.h"
//MP_REGISTER_ROOT_POINTER(mp_obj_t native_code_pointers);

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/spiram.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/spiram.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/spiram.h"
#endif

#include "shared/readline/readline.h"
#include "shared/runtime/pyexec.h"
#include "uart.h"
#include "usb.h"
#include "usb_serial_jtag.h"
#include "modmachine.h"
//#include "modnetwork.h"
#include "mpthreadport.h"
#include "../main/leds.h"
#include "../main/behavior.h"
#include "../main/buttons.h"
#include "../main/mode.h"
#include "../main/common.h"
#include "../main/accelerometer.h"
#include "../main/ble_spp.h"
#include "../main/utility.h"

#if MICROPY_BLUETOOTH_NIMBLE
#include "extmod/modbluetooth.h"
#endif

// MicroPython runs as a task under FreeRTOS
#define MP_TASK_PRIORITY        (ESP_TASK_PRIO_MIN + 1)
#define MP_TASK_STACK_SIZE      (16 * 1024)

// Set the margin for detecting stack overflow, depending on the CPU architecture.
#if CONFIG_IDF_TARGET_ESP32C3
#define MP_TASK_STACK_LIMIT_MARGIN (2048)
#else
#define MP_TASK_STACK_LIMIT_MARGIN (1024)
#endif

#define JSON_BUFFER_SIZE 10240  // Considering 100 bytes per file for their description (name, size) we can list about 100 files.

static int8_t mp_component_state = -1;
static EventGroupHandle_t mp_component_event_group;
uint8_t scriptPresent[7] = {0};
static uint8_t main_counter = 0;
uint8_t* buttonState;
char* ram_file_data;
uint8_t ram_script_id;
char file_name[30];
size_t ram_file_len;
nlr_buf_t nlr;
static char *json_buf = NULL;
char json_mem_info[256] = {0};
uint16_t json_mem_info_len = 0;
int32_t flash_free_bytes = -1;
int32_t ram_free_bytes = -1;
char *read_file_data = NULL;
size_t read_file_len = 0;


const int EVT_EXEC_MODE = BIT0;

// Funzione helper per l'esecuzione dello script MicroPython
void run_micropython_script(char* script_content) {
    //if (mp_globals == NULL) {
    //    mp_globals = mp_globals_new();
    //}

    mp_handle_pending(false); // Do not raise exception

    // Creazione del lexer a partire dalla stringa C
    mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_string_gt_, script_content, strlen(script_content), 0);
    if (lex == NULL) {
        printf("Errore: Impossibile creare il lexer\n");
        return;
    }

    // Inizializzazione del parse tree
    mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);

    // Esecuzione del codice
    qstr source_name = qstr_from_str("prova");
    mp_obj_t script_result = mp_compile(&parse_tree, source_name, false);
    mp_call_function_0(script_result);

    // Libera la memoria del lexer
    //mp_lexer_free(lex); // Already done by mp_parse!!
}


int vprintf_null(const char *format, va_list ap) {
    // do nothing: this is used as a log target during raw repl mode
    return 0;
}

void mp_exec_script_task(void *pvParameter) {
    char* script_data = (char*) pvParameter;
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        run_micropython_script(script_data);
        nlr_pop();
    } else {
        // Un'eccezione è stata sollevata (es. KeyboardInterrupt)
        printf("Script interrotto.\n");
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
    vTaskDelete(NULL);
}

/**
 * @brief The 'write' function for our MicroPython dupterm stream.
 *
 * This is called by MP's print().
 */
static mp_uint_t ble_stream_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    
    // Call the public BLE service function
    size_t bytes_written = ble_spp_stdout_write(buf, size);

    if (bytes_written < size) {
        // We dropped some data (buffer was full)
        // We don't signal an error, just report what we did
    }

    return bytes_written;
}

STATIC mp_uint_t ble_stream_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    *errcode = MP_EAGAIN;
    return 0;
}

STATIC mp_uint_t mp_stream_ioctl_dummy(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    // Set the error code to indicate that the operation is not supported
    *errcode = MP_EINVAL; // Invalid argument/operation not supported
    return 0;
}

STATIC const mp_stream_p_t ble_stream_p = {
    .read = ble_stream_read,
    .write = ble_stream_write,
    .is_text = true,
    .ioctl = mp_stream_ioctl_dummy, // Try adding this if it's missing
};

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_ble_stdout_stream,
    MP_QSTR_BLE_Stream,
    MP_TYPE_FLAG_NONE,
    protocol, &ble_stream_p
);

static const mp_obj_base_t mp_stdout_ble_stream_obj = {
    .type = &mp_type_ble_stdout_stream
};

/*
STATIC const mp_obj_type_t mp_ble_stream_type = {
    { &mp_type_type },
    .name = MP_QSTR_ble_stream,
    .protocol = &ble_stream_p,
};

STATIC mp_ble_stream_obj_t ble_stream_obj = { { &mp_ble_stream_type } };
*/

void mp_task(void *pvParameter) {
    nlr_buf_t nlr;
    volatile uint32_t sp = (uint32_t)get_sp();
    #if MICROPY_PY_THREAD
    mp_thread_init(pxTaskGetStackStart(NULL), MP_TASK_STACK_SIZE / sizeof(uintptr_t));
    #endif
    #if CONFIG_USB_ENABLED
    usb_init();
    #elif CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG
    usb_serial_jtag_init();
    #endif
    #if MICROPY_HW_ENABLE_UART_REPL
    uart_stdout_init();
    #endif
    machine_init();

    size_t mp_task_heap_size;
    void *mp_task_heap = NULL;

    #if CONFIG_SPIRAM_USE_MALLOC
    // SPIRAM is issued using MALLOC, fallback to normal allocation rules
    mp_task_heap = NULL;
    #elif CONFIG_ESP32_SPIRAM_SUPPORT
    // Try to use the entire external SPIRAM directly for the heap
    mp_task_heap = (void *)SOC_EXTRAM_DATA_LOW;
    switch (esp_spiram_get_chip_size()) {
        case ESP_SPIRAM_SIZE_16MBITS:
            mp_task_heap_size = 2 * 1024 * 1024;
            break;
        case ESP_SPIRAM_SIZE_32MBITS:
        case ESP_SPIRAM_SIZE_64MBITS:
            mp_task_heap_size = 4 * 1024 * 1024;
            break;
        default:
            // No SPIRAM, fallback to normal allocation
            mp_task_heap = NULL;
            break;
    }
    #elif CONFIG_ESP32S2_SPIRAM_SUPPORT || CONFIG_ESP32S3_SPIRAM_SUPPORT
    // Try to use the entire external SPIRAM directly for the heap
    size_t esp_spiram_size = esp_spiram_get_size();
    if (esp_spiram_size > 0) {
        mp_task_heap = (void *)SOC_EXTRAM_DATA_HIGH - esp_spiram_size;
        mp_task_heap_size = esp_spiram_size;
    }
    #endif

    if (mp_task_heap == NULL) {
        // Allocate the uPy heap using malloc and get the largest available region,
        // limiting to 1/2 total available memory to leave memory for the OS.
        #if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 1, 0)
        size_t heap_total = heap_caps_get_total_size(MALLOC_CAP_8BIT);
        #else
        multi_heap_info_t info;
        heap_caps_get_info(&info, MALLOC_CAP_8BIT);
        size_t heap_total = info.total_free_bytes + info.total_allocated_bytes;
        #endif
        mp_task_heap_size = MIN(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), heap_total / 2);
        mp_task_heap = malloc(mp_task_heap_size);
        //printf("uPy allocated heap size = %d", mp_task_heap_size);
    }
    
soft_reset:
    // initialise the stack pointer for the main thread
    mp_stack_set_top((void *)sp);
    mp_stack_set_limit(MP_TASK_STACK_SIZE - MP_TASK_STACK_LIMIT_MARGIN);
    gc_init(mp_task_heap, mp_task_heap + mp_task_heap_size);
    mp_init();
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    readline_init0();

    MP_STATE_PORT(native_code_pointers) = MP_OBJ_NULL;

    // initialise peripherals
    //machine_pins_init();
    #if MICROPY_PY_MACHINE_I2S
    machine_i2s_init0();
    #endif

    //mp_os_dupterm(MP_OBJ_FROM_PTR(&mp_stdout_ble_stream_obj), 0);
    //mp_uos_dupterm(MP_OBJ_FROM_PTR(&mp_stdout_ble_stream_obj), 0);
    //mp_stream_dupterm(MP_OBJ_FROM_PTR(&mp_stdout_ble_stream_obj), 0);
    //MP_STATE_VM(dupterm_objs[1]) = MP_OBJ_FROM_PTR(&mp_stdout_ble_stream_obj);

    if (nlr_push(&nlr) == 0) {
        // Load the uos module and get the 'dupterm' function object
        mp_obj_t uos_module = mp_import_name(MP_QSTR_uos, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
        mp_obj_t dupterm_func = mp_load_attr(uos_module, MP_QSTR_dupterm);

        // Call uos.dupterm(stream_object)
        mp_call_function_1(dupterm_func, MP_OBJ_FROM_PTR(&mp_stdout_ble_stream_obj));  
        nlr_pop();
    } else {
        // Exception raised (es. KeyboardInterrupt)
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }

    // run boot-up scripts
    pyexec_frozen_module("_boot.py", false);
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        if(mp_import_stat("main.py") == MP_IMPORT_STAT_FILE) { // If main.py is present then show the user a LEDs "KITT effect".
            Leds_SetBodyBrightness(0,0,0);
            Behavior_Enable(B_LEDS_LEGO_KITT);
            main_counter = 0;
            Accelerometer_ClearTapStatus();  // Clear any tap made before if any
            while(1) {
                if(main_counter >= 30) { // If the user do not press the center button within 3 seconds, then start the main.py 
                    Behavior_Disable(B_LEDS_LEGO_KITT);
                    Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
                    Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);         
                    int ret = pyexec_file_if_exists("main.py");
                    if (ret & PYEXEC_FORCED_EXIT) {
                        goto soft_reset_exit;
                    }
                }
                if (Accelerometer_IsTapDetected()) { // If the user make a tap then avoid starting the main.py script and enable the behaviors menu
                    Behavior_Disable(B_LEDS_LEGO_KITT);
                    Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
                    Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
                    exit_micropython_mode();    // Enable the behaviors menu
                    break;                    
                }
                vTaskDelay(100 / portTICK_PERIOD_MS);
                main_counter++;
            }
        } else { // If main.py not present then enable the behaviors menu
            Leds_SetLegoFrontBrightness(0, 0, 0, 0, 0, 0, 0, 0);
            Leds_SetLegoBackBrightness(0, 0, 0, 0, 0, 0, 0, 0);
            exit_micropython_mode();    // Enable the behaviors menu
        }
    }
    // Check presence of mainID.py scripts
    if(mp_import_stat("main1.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[0] = 0;
    } else {
        scriptPresent[0] = 1;
    }
    if(mp_import_stat("main2.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[1] = 0;
    } else {
        scriptPresent[1] = 1;
    }    
    if(mp_import_stat("main3.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[2] = 0;
    } else {
        scriptPresent[2] = 1;
    }    
    if(mp_import_stat("main4.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[3] = 0;
    } else {
        scriptPresent[3] = 1;
    }
    if(mp_import_stat("main5.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[4] = 0;
    } else {
        scriptPresent[4] = 1;
    }    
    if(mp_import_stat("main6.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[5] = 0;
    } else {
        scriptPresent[5] = 1;
    }    
    if(mp_import_stat("main7.py") != MP_IMPORT_STAT_FILE) {
        scriptPresent[6] = 0;
    } else {
        scriptPresent[6] = 1;
    }    

    for (;;) {

        // If REPL was chosen then skip the wait event. This is because the REPL can be switched from "raw" to "friendly" or viceversa  
        // (e.g. when using pyboard.py) and this imply that the code will exit the main loop and restart from "soft_reset".
        if(mp_component_state == -1) {
            xEventGroupWaitBits(mp_component_event_group, EVT_EXEC_MODE, true, false, portMAX_DELAY); // Wait for an event to be raised
            ESP_LOGE("mp_component", "script=%d", mp_component_state);
        }

        switch(mp_component_state) {
            case 0: // REPL
                enter_micropython_mode();
                while(1) {
                    if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
                        vprintf_like_t vprintf_log = esp_log_set_vprintf(vprintf_null);
                        if (pyexec_raw_repl() != 0) {
                            break;
                        }
                        esp_log_set_vprintf(vprintf_log);
                    } else {
                        if (pyexec_friendly_repl() != 0) {
                            break;
                        }
                    }
                }
                goto soft_reset_exit;
                break;
            case 1: // main1.py           
                pyexec_file("main1.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 2: // main2.py
                pyexec_file("main2.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 3: //main3.py
                pyexec_file("main3.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 4: //main4.py
                pyexec_file("main4.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 5: //main5.py
                pyexec_file("main5.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 6: //main6.py
                pyexec_file("main6.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 7: //main7.py
                pyexec_file("main7.py");
                mp_component_state = -1; // Execute the script only once
                break;
            case 8: //script from RAM
                // Exception handled by Micropython by using NLR method
                if (nlr_push(&nlr) == 0) {
                    run_micropython_script(ram_file_data);
                    nlr_pop();
                    ble_indicate_python_exec(PYTHON_EXEC_OK);
                } else {
                    // Exception raised (es. KeyboardInterrupt)
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ble_indicate_python_exec(PYTHON_EXEC_ERROR);
                }
                turnOffAllSensors();             
                mp_component_state = -1; // Execute the script only once
                break;
            case 9: // save script                
                memset(file_name, 0, sizeof(file_name));
                if(ram_script_id == 0)
                {
                    snprintf(file_name, sizeof(file_name), "main.py");
                }
                else
                {
                    snprintf(file_name, sizeof(file_name), "main%d.py", ram_script_id);
                }
                const char* data = ram_file_data;            
                if (nlr_push(&nlr) == 0) {
                    mp_obj_t file_obj = mp_builtin_open(2, (mp_obj_t[]){
                        mp_obj_new_str(file_name, strlen(file_name)),
                        mp_obj_new_str("wb", 2)
                    }, (mp_map_t *)&mp_const_empty_map);

                    const mp_stream_p_t *stream_p = mp_get_stream_raise(file_obj, MP_STREAM_OP_WRITE);
                    int err;
                    stream_p->write(file_obj, data, ram_file_len, &err);
                    mp_stream_close(file_obj);
                    nlr_pop();
                    ble_indicate_python_save(PYTHON_SAVE_OK);
                } else {
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ble_indicate_python_save(PYTHON_SAVE_ERROR);
                }
                mp_component_state = -1; // Execute the command only once
                break;
            case 10: // save file
                if (nlr_push(&nlr) == 0) {
                    // Check if there is enough space in the filesystem
                    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
                    mp_obj_t statvfs_fn = mp_load_attr(os_module, MP_QSTR_statvfs);
                    mp_obj_t res = mp_call_function_1(statvfs_fn, mp_obj_new_str("/", 1));
                    mp_obj_t *items;
                    mp_obj_get_array_fixed_n(res, 10, &items);
                    uint32_t block_size = mp_obj_get_int(items[0]);
                    uint32_t free_blocks = mp_obj_get_int(items[3]);
                    size_t free_bytes = block_size * free_blocks;
                    ESP_LOGI("mp_component", "Filesystem free space: %d bytes", free_bytes);
                    if(free_bytes < ram_file_len) {
                        ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_NO_SPACE);
                        ESP_LOGI("mp_component", "Not enough space to save the file (%d bytes needed)", ram_file_len);
                        nlr_pop();
                    } 
                    else
                    {
                        mp_obj_t file_obj = mp_builtin_open(2, (mp_obj_t[]){
                            mp_obj_new_str(file_name, strlen(file_name)),
                            mp_obj_new_str("wb", 2)
                        }, (mp_map_t *)&mp_const_empty_map);

                        const mp_stream_p_t *stream_p = mp_get_stream_raise(file_obj, MP_STREAM_OP_WRITE);
                        int err = 0;
                        mp_uint_t written = stream_p->write(file_obj, ram_file_data, ram_file_len, &err);
                        if (written < ram_file_len) {
                            ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_NO_SPACE);
                            ESP_LOGI("mp_component", "Not enough space to save the file (%d bytes written, %d bytes needed)", written, ram_file_len);   
                        }         
                        else if (err != 0) {
                            ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_ERROR);
                            ESP_LOGI("mp_component", "Error while writing the file (err=%d)", err);                        
                        }
                        else
                        {
                            ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_OK);
                        }
                        mp_stream_close(file_obj);
                        nlr_pop();
                    }
                } else {
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ble_indicate_fs(FS_IND_SAVE_RES, FS_SAVE_ERROR);
                }
                mp_component_state = -1; // Execute the command only once
                break;
            case 11: // delete file
                if (nlr_push(&nlr) == 0) {
                    // Import os module
                    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));

                    // Get listdir('/')
                    mp_obj_t listdir_fn = mp_load_attr(os_module, MP_QSTR_listdir);
                    mp_obj_t files_obj = mp_call_function_1(listdir_fn, mp_obj_new_str("/", 1));

                    // Iterate through returned list
                    size_t len;
                    mp_obj_t *items;
                    mp_obj_get_array(files_obj, &len, &items);

                    bool found = false;
                    for (size_t i = 0; i < len; i++) {
                        const char *fname = mp_obj_str_get_str(items[i]);
                        if (strcmp(fname, file_name) == 0) {
                            found = true;
                            break;
                        }
                    }

                    if (found) {
                        // Call os.remove("/filename")
                        char full_path[64];
                        snprintf(full_path, sizeof(full_path), "/%s", file_name);
                        mp_obj_t remove_fn = mp_load_attr(os_module, MP_QSTR_remove);
                        mp_call_function_1(remove_fn, mp_obj_new_str(full_path, strlen(full_path)));

                        ble_indicate_fs(FS_IND_DELETE_RES, FS_DELETE_OK);
                    } else {
                        ble_indicate_fs(FS_IND_DELETE_RES, FS_DELETE_NOT_FOUND);
                    }

                    nlr_pop();
                } else {
                    // Print Python exception (if any) and indicate error
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ble_indicate_fs(FS_IND_DELETE_RES, FS_DELETE_ERROR);
                }
                mp_component_state = -1; // Execute the command only once
                break;
            case 12: // list files
                json_buf = calloc(JSON_BUFFER_SIZE, sizeof(char));
                if(json_buf == NULL) {
                    ESP_LOGE("mp_component", "Failed to allocate memory for JSON buffer");
                    ble_indicate_fs_list_err();
                    mp_component_state = -1; // Execute the command only once                    
                    break;
                }
                if (nlr_push(&nlr) == 0) {
                    // Import os
                    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));

                    // Get os.listdir("/")
                    mp_obj_t listdir_fn = mp_load_attr(os_module, MP_QSTR_listdir);
                    mp_obj_t files_obj = mp_call_function_1(listdir_fn, mp_obj_new_str("/", 1));

                    // Start JSON array
                    strcat(json_buf, "[");

                    size_t len;
                    mp_obj_t *items;
                    mp_obj_get_array(files_obj, &len, &items);

                    // Get os.stat for file sizes
                    mp_obj_t stat_fn = mp_load_attr(os_module, MP_QSTR_stat);

                    for (size_t i = 0; i < len; i++) {
                        const char *fname = mp_obj_str_get_str(items[i]);

                        // Build full path "/filename"
                        char full_path[64];
                        snprintf(full_path, sizeof(full_path), "/%s", fname);

                        // Get file info using os.stat(full_path)
                        mp_obj_t stat_res = mp_call_function_1(stat_fn, mp_obj_new_str(full_path, strlen(full_path)));

                        // os.stat() returns a tuple, where element [6] = file size
                        mp_obj_t *stat_items;
                        mp_obj_get_array_fixed_n(stat_res, 10, &stat_items);
                        mp_int_t fsize = mp_obj_get_int(stat_items[6]);

                        // Append entry to JSON buffer
                        char entry[256];
                        snprintf(entry, sizeof(entry), "{\"name\":\"%s\",\"size\":%ld}", fname, (long)fsize);
                        strcat(json_buf, entry);

                        if (i < len - 1) strcat(json_buf, ",");
                    }

                    strcat(json_buf, "]");
                    nlr_pop();
                } else { // Exception raised
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ble_indicate_fs_list_err();
                    free(json_buf);
                    mp_component_state = -1; // Execute the command only once
                    break;
                }
                ESP_LOG_BUFFER_CHAR("mp_component", json_buf, strlen(json_buf));
                ble_indicate_fs_list((uint8_t *)json_buf, strlen(json_buf));     
                mp_component_state = -1; // Execute the command only once
                break;
            case 13: // mem info
                memset(json_mem_info, 0, sizeof(json_mem_info));

                if (nlr_push(&nlr) == 0) {
                    // --- Flash free space ---
                    // This is the flash storage in the "micropython space" used to save files
                    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
                    mp_obj_t statvfs_fn = mp_load_attr(os_module, MP_QSTR_statvfs);
                    mp_obj_t res = mp_call_function_1(statvfs_fn, mp_obj_new_str("/", 1));
                    mp_obj_t *items;
                    mp_obj_get_array_fixed_n(res, 10, &items);
                    uint32_t block_size = mp_obj_get_int(items[0]);
                    uint32_t free_blocks = mp_obj_get_int(items[3]);
                    flash_free_bytes = block_size * free_blocks;

                    // --- RAM free space (esp-idf C side) ---
                    // This is the working memory used with BLE so we report this instead of micropython side heap
                    ram_free_bytes = esp_get_free_heap_size();

                    // --- RAM free space (MicroPython heap) ---
                    /*
                    mp_obj_t gc_module = mp_import_name(MP_QSTR_gc, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));
                    mp_obj_t mem_free_fn = mp_load_attr(gc_module, MP_QSTR_mem_free);
                    mp_obj_t free_res = mp_call_function_0(mem_free_fn);
                    ram_free_bytes = mp_obj_get_int(free_res);
                    */  

                    nlr_pop();
                } else {
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                }                

                // --- Always return JSON, even if failed ---
                snprintf(&json_mem_info[3], sizeof(json_mem_info)-3,
                        "{\"flash_bytes_free\": %d, \"ram_bytes_free\": %d}",
                        flash_free_bytes, ram_free_bytes);

                json_mem_info_len = strlen(&json_mem_info[3]);                        
                json_mem_info[0] = DEV_INFO_IND_MEMORY_RES;
                json_mem_info[1] = (json_mem_info_len >> 8) & 0xFF;  // length high byte
                json_mem_info[2] = json_mem_info_len & 0xFF;         // length low byte
                json_mem_info_len += 3;

                ble_indicate_dev_info((uint8_t *)json_mem_info, json_mem_info_len);
                ESP_LOG_BUFFER_CHAR("mp_component", &json_mem_info[3], json_mem_info_len-3);
                mp_component_state = -1; // Execute the command only once
                break;
            case 14: // read file
                read_file_len = 0;
                read_file_data = NULL;

                if (nlr_push(&nlr) == 0) {

                    // Import os module
                    mp_obj_t os_module = mp_import_name(MP_QSTR_os, mp_const_none, MP_OBJ_NEW_SMALL_INT(0));

                    // Get listdir('/')
                    mp_obj_t listdir_fn = mp_load_attr(os_module, MP_QSTR_listdir);
                    mp_obj_t files_obj = mp_call_function_1(listdir_fn, mp_obj_new_str("/", 1));

                    // Iterate through returned list
                    size_t len;
                    mp_obj_t *items;
                    mp_obj_get_array(files_obj, &len, &items);

                    bool found = false;
                    for (size_t i = 0; i < len; i++) {
                        const char *fname = mp_obj_str_get_str(items[i]);
                        if (strcmp(fname, file_name) == 0) {
                            found = true;
                            break;
                        }
                    }            
                    
                    if (found) {
                        // Open file for reading
                        mp_obj_t file_obj = mp_builtin_open(2, (mp_obj_t[]){
                            mp_obj_new_str(file_name, strlen(file_name)),
                            mp_obj_new_str("rb", 2)
                        }, (mp_map_t *)&mp_const_empty_map);

                        const mp_stream_p_t *stream_p = mp_get_stream_raise(file_obj, MP_STREAM_OP_READ);
                        int err;
                        mp_uint_t bytes_read;

                        // Seek to end to get file length
                        mp_obj_t seek_meth = mp_load_attr(file_obj, MP_QSTR_seek);
                        mp_call_function_2(seek_meth, mp_obj_new_int(0), mp_obj_new_int(2)); // SEEK_END
                        mp_obj_t tell_meth = mp_load_attr(file_obj, MP_QSTR_tell);
                        mp_obj_t file_len_obj = mp_call_function_0(tell_meth);
                        size_t file_len = mp_obj_get_int(file_len_obj);
                        read_file_len = file_len;

                        // Rewind
                        mp_call_function_2(seek_meth, mp_obj_new_int(0), mp_obj_new_int(0)); // SEEK_SET

                        // Allocate buffer
                        read_file_data = malloc(file_len);
                        if (!read_file_data) {
                            ESP_LOGI("mp_component", "Out of memory allocating %d bytes", file_len);
                            mp_stream_close(file_obj);
                            ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_ERROR);
                            mp_component_state = -1; // Execute the command only once                             
                            nlr_pop();                      
                            break;                            
                        }

                        // Read the file
                        bytes_read = stream_p->read(file_obj, read_file_data, file_len, &err);
                        if (err != 0 || bytes_read != file_len) {
                            ESP_LOGI("mp_component", "Read error or incomplete read");
                            mp_stream_close(file_obj);
                            free(read_file_data);
                            read_file_data = NULL;
                            read_file_len = 0;
                            ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_ERROR);
                            mp_component_state = -1; // Execute the command only once                         
                            nlr_pop();
                            break;
                        }

                        mp_stream_close(file_obj);
                        
                    } else {
                        ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_NOT_FOUND);
                    }

                    nlr_pop();

                } else {
                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                    ESP_LOGI("mp_component", "Exception while reading file");
                    if(read_file_data) 
                    {
                        free(read_file_data);
                        read_file_data = NULL;
                    }
                    read_file_len = 0;
                    ble_indicate_fs(FS_IND_DOWNLOAD_RES, FS_DOWNLOAD_ERROR);
                    mp_component_state = -1; // Execute the command only once
                    break;
                }
                ble_indicate_download((uint8_t *)read_file_data, read_file_len);
                mp_component_state = -1; // Execute the command only once       
                break;
            case 15: // firmware info
                memset(json_mem_info, 0, sizeof(json_mem_info));           

                snprintf(&json_mem_info[3], sizeof(json_mem_info)-3,
                        "{\"esp32_ver\": %d, \"stm32_ver\": %d}",
                        Common_GetFirmwareVersion(), 0);

                json_mem_info_len = strlen(&json_mem_info[3]);                        
                json_mem_info[0] = DEV_INFO_IND_FIRMWARE_RES;
                json_mem_info[1] = (json_mem_info_len >> 8) & 0xFF;  // length high byte
                json_mem_info[2] = json_mem_info_len & 0xFF;         // length low byte
                json_mem_info_len += 3;

                ble_indicate_dev_info((uint8_t *)json_mem_info, json_mem_info_len);
                ESP_LOG_BUFFER_CHAR("mp_component", &json_mem_info[3], json_mem_info_len-3);
                mp_component_state = -1; // Execute the command only once
                break;                
            default:
                break; 
        }
        
    }

soft_reset_exit:

    #if MICROPY_BLUETOOTH_NIMBLE
    mp_bluetooth_deinit();
    #endif

    machine_timer_deinit_all();

    #if MICROPY_PY_THREAD
    mp_thread_deinit();
    #endif

    // Free any native code pointers that point to iRAM.
    if (MP_STATE_PORT(native_code_pointers) != MP_OBJ_NULL) {
        size_t len;
        mp_obj_t *items;
        mp_obj_list_get(MP_STATE_PORT(native_code_pointers), &len, &items);
        for (size_t i = 0; i < len; ++i) {
            heap_caps_free(MP_OBJ_TO_PTR(items[i]));
        }
    }

    gc_sweep_all();

    mp_hal_stdout_tx_str("MPY: soft reboot\r\n");

    // deinitialise peripherals
    machine_pwm_deinit_all();
    // TODO: machine_rmt_deinit_all();
    //machine_pins_deinit();
    machine_deinit();
    #if MICROPY_PY_USOCKET_EVENTS
    usocket_events_deinit();
    #endif

    mp_deinit();
    fflush(stdout);
    goto soft_reset;
}


void boardctrl_startup(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

//void app_main(void) {
void init_micropython(void) {
    // Hook for a board to run code at start up.
    // This defaults to initialising NVS.
    MICROPY_BOARD_STARTUP();

    mp_component_event_group = xEventGroupCreate();

    enter_micropython_mode();

    // Create and transfer control to the MicroPython task.
    //printf("task ret = %d\n", xTaskCreatePinnedToCore(mp_task, "mp_task", MP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, MP_TASK_PRIORITY, &mp_main_task_handle, MP_TASK_COREID));
    xTaskCreatePinnedToCore(mp_task, "mp_task", MP_TASK_STACK_SIZE / sizeof(StackType_t), NULL, MP_TASK_PRIORITY, &mp_main_task_handle, MP_TASK_COREID);
}

void nlr_jump_fail(void *val) {
    printf("NLR jump failed, val=%p\n", val);
    esp_restart();
}

// modussl_mbedtls uses this function but it's not enabled in ESP IDF
void mbedtls_debug_set_threshold(int threshold) {
    (void)threshold;
}

void *esp_native_code_commit(void *buf, size_t len, void *reloc) {
    len = (len + 3) & ~3;
    uint32_t *p = heap_caps_malloc(len, MALLOC_CAP_EXEC);
    if (p == NULL) {
        m_malloc_fail(len);
    }
    if (MP_STATE_PORT(native_code_pointers) == MP_OBJ_NULL) {
        MP_STATE_PORT(native_code_pointers) = mp_obj_new_list(0, NULL);
    }
    mp_obj_list_append(MP_STATE_PORT(native_code_pointers), MP_OBJ_TO_PTR(p));
    if (reloc) {
        mp_native_relocate(reloc, buf, (uintptr_t)p);
    }
    memcpy(p, buf, len);
    return p;
	//return NULL;
}

void exec_script(uint8_t id) {
    mp_component_state = id;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE); // Tell the main micropython loop to execute REPL or a user script.
}

uint8_t script_is_present(uint8_t id) {
    return scriptPresent[id-1];
}

void mp_exec_script_from_ram(char* script)
{
    if(mp_component_state == -1) // Run only one script at a time
    {
        ram_file_data = script;
        mp_component_state = 8;
        xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE); // Tell the main micropython loop to execute REPL or a user script.
    }
    else
    {
        ble_indicate_python_exec(PYTHON_EXEC_ALREADY_RUNNING);
    }
}

void mp_stop_script(void)
{
    mp_sched_keyboard_interrupt();
}

void mp_save_script(char* script, uint8_t id, uint16_t script_len)
{
    ram_file_data = script;
    ram_file_len = script_len;
    ram_script_id = id;
    mp_component_state = 9;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);

}

void mp_save_file(uint8_t* data, char* filename, uint32_t data_len)
{
    ram_file_data = (char*)data;
    ram_file_len = data_len;
    memset(file_name, 0, sizeof(file_name));
    snprintf(file_name, sizeof(file_name), "%s", filename);
    mp_component_state = 10;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

void mp_delete_file(char* filename)
{
    memset(file_name, 0, sizeof(file_name));
    snprintf(file_name, sizeof(file_name), "%s", filename);
    mp_component_state = 11;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

void mp_list_files(void)
{
    mp_component_state = 12;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

void mp_list_files_free_buffer(void)
{
    if(json_buf != NULL) {
        free(json_buf);
        json_buf = NULL;
    }
}


void mp_read_file(char* filename)
{
    memset(file_name, 0, sizeof(file_name));
    snprintf(file_name, sizeof(file_name), "%s", filename);
    mp_component_state = 14;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

void mp_read_file_free_buffer(void)
{
    if(read_file_data != NULL) {
        free(read_file_data);
        read_file_data = NULL;
    }
}

void mp_mem_info(void)
{
    mp_component_state = 13;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

void mp_firmware_info(void)
{
    mp_component_state = 15;
    xEventGroupSetBits(mp_component_event_group, EVT_EXEC_MODE);
}

MP_REGISTER_ROOT_POINTER(mp_obj_t native_code_pointers);
