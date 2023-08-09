//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    main.c
//! \brief   This module provides the useful functions to run the application
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "sdkconfig.h"

#include "esp_log.h"
#include "esp32/spiram.h"

#include "aseba.h"
#include "aseba_esp32.h"
#include "behavior.h"
//#include "bluetooth.h"
//#include "ble.h"
#include "buttons.h"
#include "codec.h"
#include "comm.h"
#include "fifo.h"
#include "file_server.h"
#include "file_system.h"
#include "gpio.h"
#include "leds.h"
#include "mode.h"
#include "power.h"
#include "rc5.h"
#include "sensors.h"
#include "tcp_server.h"
#include "test.h"
#include "timer_sw.h"
//#include "tracking.h"
#include "uart.h"
#include "wifi.h"
#include "wifi_update.h"
#include "mp_component.h"

#include <stdlib.h>
#include <stdio.h>
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_system.h"

#include "vfs_fat_internal.h"
#include "diskio_impl.h"

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
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "main";

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

//uint8_t temp_buff[4096]={0};
bool printStats = false;

void stats(void*z)
{
	char * buf=malloc(1000);

	while (1)
	{
		//CPU usage & task list
		memset(buf, 0x0, 1000);
		vTaskGetRunTimeStats(buf);
		printf("%s\n",buf);
		vTaskList(buf);
		printf("%s",buf);

		//Timers
		esp_timer_dump(stdout);
		printf("\n");

		//Heap
		printf("heap_free_size %d\n",heap_caps_get_free_size(MALLOC_CAP_8BIT));

		vTaskDelay(15000 / portTICK_PERIOD_MS);
	}
}

void stats2(void*z)
{
	char * buf=malloc(1000);
	char * buf_addr = buf;
	volatile UBaseType_t uxArraySize;
	//TaskStatus_t *mytasks = pvPortMalloc( uxTaskGetNumberOfTasks() * sizeof( TaskStatus_t ) );
	TaskStatus_t mytasks[30];
	unsigned long mytasksLastRunTimeCounter[30];
	unsigned long mytasksMinCpu[30] = {100};
	unsigned long mytasksMaxCpu[30] = {0};
	uint16_t mytasksMinStack[30] = {60000};
	TaskStatus_t *mytaskTemp;
	uint8_t num_tasks = 0, i = 0, j = 0;
	unsigned long cpuUsageTemp = 0;
	uint32_t lastTotalRunTimeCounter = 0, totalRunTimeCounter = 0;
	uint32_t currHeapSize = 0, minHeapSize = 0;
	uint8_t totalMaxCpu0Usage = 0, sumCpu0Usage = 0, maxCpu0UsageTaskId = 0, taskMaxCpu0Usage = 0;
	uint8_t totalMaxCpu1Usage = 0, sumCpu1Usage = 0, maxCpu1UsageTaskId = 0, taskMaxCpu1Usage = 0;
	uint8_t totalMaxCpuXUsage = 0, sumCpuXUsage = 0, maxCpuXUsageTaskId = 0, taskMaxCpuXUsage = 0; // Not pinned to any core.
	//num_tasks = uxTaskGetSystemState(mytasks, uxArraySize, &lastTotalRunTimeCounter);
	//for(j=0; j<num_tasks; j++) {
	//	mytasksLastRunTimeCounter[j] = lastTotalRunTimeCounter;
	//	mytasksMinStack[j] = mytasks[j].usStackHighWaterMark;
	//}

	while (1)
	{
		uxArraySize = uxTaskGetNumberOfTasks();

	   /* Allocate a TaskStatus_t structure for each task.*/
		mytaskTemp = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

	   if(mytaskTemp != NULL)
	   {
		  /* Generate raw status information about each task. */
		  uxArraySize = uxTaskGetSystemState(mytaskTemp, uxArraySize, &totalRunTimeCounter);
		  //printf("uxArraySize = %d\n", uxArraySize);
		  sumCpu0Usage = 0;
		  sumCpu1Usage = 0;
		  sumCpuXUsage = 0;
		  for(i=0; i<uxArraySize; i++) {
			  for(j=0; j<num_tasks; j++) {
				  if(mytaskTemp[i].xTaskNumber == mytasks[j].xTaskNumber) { // Found the task in the list, update its stats
					  memcpy(&mytasks[j], &mytaskTemp[i], sizeof(TaskStatus_t));
					  //printf("%s) task timer = %u, last task timer = %lu, total counter = %u, total last counter = %u\n", mytasks[j].pcTaskName, mytasks[j].ulRunTimeCounter, mytasksLastRunTimeCounter[j], totalRunTimeCounter, lastTotalRunTimeCounter);
					  cpuUsageTemp = (mytasks[j].ulRunTimeCounter - mytasksLastRunTimeCounter[j])*100/(totalRunTimeCounter - lastTotalRunTimeCounter);
					  if(mytasksMinCpu[j] > cpuUsageTemp) {
						  mytasksMinCpu[j] = cpuUsageTemp;
					  }
					  if(mytasksMaxCpu[j] < cpuUsageTemp) {
						  mytasksMaxCpu[j] = cpuUsageTemp;
					  }
					  if(mytasksMinStack[j] > mytasks[j].usStackHighWaterMark) {
						  mytasksMinStack[j] = mytasks[j].usStackHighWaterMark;
					  }
					  if(strcmp("IDLE", mytasks[j].pcTaskName) != 0) { // IDLE tasks represent free cpu
						  if(mytasks[j].xCoreID == 0) {
							  if(taskMaxCpu0Usage < cpuUsageTemp) {
								  taskMaxCpu0Usage = cpuUsageTemp;
								  maxCpu0UsageTaskId = j;
							  }
							  sumCpu0Usage += cpuUsageTemp;
						  } else if(mytasks[j].xCoreID == 1) {
							  if(taskMaxCpu1Usage < cpuUsageTemp) {
								  taskMaxCpu1Usage = cpuUsageTemp;
								  maxCpu1UsageTaskId = j;
							  }
							  sumCpu1Usage += cpuUsageTemp;
						  } else {
							  if(taskMaxCpuXUsage < cpuUsageTemp) {
								  taskMaxCpuXUsage = cpuUsageTemp;
								  maxCpuXUsageTaskId = j;
							  }
							  sumCpuXUsage += cpuUsageTemp;
						  }
					  }
					  mytasksLastRunTimeCounter[j] = mytasks[j].ulRunTimeCounter;
					  break;
				  }
			  }
			  if(j==num_tasks) { // Task not found in the list, add it
				  memcpy(&mytasks[num_tasks], &mytaskTemp[i], sizeof(TaskStatus_t));
				  mytasksLastRunTimeCounter[num_tasks] = 0;
				  mytasksMinStack[num_tasks] = mytasks[num_tasks].usStackHighWaterMark;
				  num_tasks++;
			  }
		  }

		  lastTotalRunTimeCounter = totalRunTimeCounter;
		  if(totalMaxCpu0Usage < sumCpu0Usage) {
			  totalMaxCpu0Usage = sumCpu0Usage;
		  }
		  if(totalMaxCpu1Usage < sumCpu1Usage) {
			  totalMaxCpu1Usage = sumCpu1Usage;
		  }
		  if(totalMaxCpuXUsage < sumCpuXUsage) {
			  totalMaxCpuXUsage = sumCpuXUsage;
		  }
		  /* The array is no longer needed, free the memory it consumes. */
		  vPortFree(mytaskTemp);
	   }

		if(printStats) {
			printStats = false;
			buf = buf_addr;
			memset(buf, 0x0, 1000);
			for(j=0; j<num_tasks; j++) {
				sprintf(buf, "%s,%lu,%lu,%d,%d\n", mytasks[j].pcTaskName, mytasksMinCpu[j], mytasksMaxCpu[j], mytasksMinStack[j], mytasks[j].xCoreID);
				buf += strlen((char*)buf);
				//mytasksLastRunTimeCounter[j] = 0;
				mytasksMinCpu[j] = 100;
				mytasksMaxCpu[j] = 0;
				mytasksMinStack[j] = 60000;
			}
			printf("%s",buf_addr);
			printf("tot max core0 usage=%d (%d, %s)\n", totalMaxCpu0Usage, taskMaxCpu0Usage, mytasks[maxCpu0UsageTaskId].pcTaskName);
			printf("tot max core1 usage=%d (%d, %s)\n", totalMaxCpu1Usage, taskMaxCpu1Usage, mytasks[maxCpu1UsageTaskId].pcTaskName);
			printf("tot max coreX usage=%d (%d, %s)\n", totalMaxCpuXUsage, taskMaxCpuXUsage, mytasks[maxCpuXUsageTaskId].pcTaskName);
			totalMaxCpu0Usage = 0;
			taskMaxCpu0Usage = 0;
			maxCpu0UsageTaskId = 0;
			totalMaxCpu1Usage = 0;
			taskMaxCpu1Usage = 0;
			maxCpu1UsageTaskId = 0;
			totalMaxCpuXUsage = 0;
			taskMaxCpuXUsage = 0;
			maxCpuXUsageTaskId = 0;

			//Heap
			//printf("heap_free_size %d\n",heap_caps_get_free_size(MALLOC_CAP_8BIT));
			// Heap statistics
			// DRAM heap size: use heap_caps_get_total_size(MALLOC_CAP_8BIT) or heap_caps_get_total_size(MALLOC_CAP_8BIT|MALLOC_CAP_32BIT) because only DRAM can handle 8bit access
			// Total (DRAM + IRAM) heap size: use heap_caps_get_total_size(MALLOC_CAP_32BIT) because both handle 32bit access
			// IRAM heap size: use (heap_caps_get_total_size(MALLOC_CAP_32BIT) - heap_caps_get_total_size(MALLOC_CAP_8BIT))
		   	//printf("tot heap=%d, dram heap=%d, iram heap=%d\n", heap_caps_get_total_size(MALLOC_CAP_32BIT), heap_caps_get_total_size(MALLOC_CAP_8BIT), (heap_caps_get_total_size(MALLOC_CAP_32BIT) - heap_caps_get_total_size(MALLOC_CAP_8BIT)));
		   	//printf("tot free heap=%d, tot dram heap=%d, tot free iram heap=%d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT), (heap_caps_get_free_size(MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT)));
		   	currHeapSize = heap_caps_get_free_size(MALLOC_CAP_32BIT);
		   	minHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT);
		   	printf("free heap=%d, min free heap=%d\n\n", currHeapSize, minHeapSize);

		}

		vTaskDelay(500 / portTICK_PERIOD_MS);
	}
}

FRESULT scan_files (
    char* path        /* Start node to be scanned (also used as work area) */
)
{
    FRESULT res;
    FILINFO fno;
    FF_DIR dir;
    int i;
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
    static char lfn[_MAX_LFN + 1];
    fno.lfname = lfn;
    fno.lfsize = sizeof lfn;
#endif


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        i = strlen(path);
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fname[0] == '.') continue;             /* Ignore dot entry */
#if _USE_LFN
            fn = *fno.lfname ? fno.lfname : fno.fname;
#else
            fn = fno.fname;
#endif
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                sprintf(&path[i], "/%s", fn);
                res = scan_files(path);
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                //printf("%s/%s\n", path, fn);
                ESP_LOGD(Tag, "%s/%s\n", path, fn);
            }
        }
    }

    return res;
}


int app_main(void)
{
//*****************************************************************************
// Initialization
//*****************************************************************************

	esp_log_level_set("*", ESP_LOG_NONE);
	//esp_log_level_set("*", ESP_LOG_ERROR);
	//esp_log_level_set("*", ESP_LOG_INFO);
	//esp_log_level_set("*", ESP_LOG_DEBUG);
	//esp_log_level_set("*", ESP_LOG_VERBOSE);




  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "** Initializations **");
  ESP_LOGI(Tag, "*********************");

  ESP_LOGI(Tag, "heap (cont.)=%ul, heap (all)=%ul, min_heap=%ul", esp_get_free_heap_size(), esp_get_free_internal_heap_size(), esp_get_minimum_free_heap_size());

  TimerSw_Init();

  Gpio_Init();

  Aseba_Init();

  Leds_Init();

  RC5_Init();

  Sensors_Init();
  Comm_Init();

  Fifo8bits_Init();

  //Test_Run();

  Behavior_Init();

  Mode_Init(false);
  //Mode_InitVM();

  //WIFI_Init();
	TCPServer_Init();
  AsebaESP32_Init();

  //Bluetooth_Init();
  //BLE_Init();

  //Codec_SetVolume(100);
  //Codec_PlayMP3FileFromFlash(E_SoundIndex_Startup);

  // TODO Move to Behavior when entering into settings
  //ESP_ERROR_CHECK(FileServer_Start("/spiffs"));

  //Codec_SetVolume(80);
  //Codec_PlayMP3File(2);
  //Codec_RecordWAVFile(0);
  //Codec_PlayWAVFile(2);

//*****************************************************************************
// Start the tasks
//*****************************************************************************

  ESP_LOGI(Tag, "*********************");
  ESP_LOGI(Tag, "******* Tasks *******");
  ESP_LOGI(Tag, "*********************");

  //esp_log_level_set("*", ESP_LOG_ERROR);
  //esp_log_level_set("sequence", ESP_LOG_INFO);
  //esp_log_level_set("mode", ESP_LOG_INFO);

  ESP_LOGI(Tag, "OTA");

  //BLE_Start();
  //WIFI_Start();
  AsebaESP32_Start();

  //Test_StartDebugging();

  Behavior_Start();

  RC5_Start();

  Sensors_Start();
  Comm_Start();

  Leds_Start();

  init_micropython();


//     // Handle of the wear levelling library instance
//     static wl_handle_t s_wl_handle = WL_INVALID_HANDLE;

//     // Mount path for the partition
//     //const char *base_path = "/spiflash";
//     const char *base_path = "/sound";

//     ESP_LOGI(Tag, "Mounting FAT filesystem");
//     // To mount device we need name of device partition, define base_path
//     // and allow format partition in case if it is new one and was not formated before
//     const esp_vfs_fat_mount_config_t mount_config = {
//             .max_files = 4,
//             .format_if_mount_failed = false,
//             .allocation_unit_size = 4096
//     };
//     /*
//     esp_err_t err = esp_vfs_fat_spiflash_mount(base_path, "vfs", &mount_config, &s_wl_handle);
//     if (err != ESP_OK) {
//         ESP_LOGE(Tag, "Failed to mount FATFS (%s)", esp_err_to_name(err));
//         return;
//     }
//     */
    
//     esp_err_t err = esp_vfs_fat_rawflash_mount(base_path, "vfs", &mount_config);
//     if (err != ESP_OK) {
//         ESP_LOGE(Tag, "(esp_vfs_fat_rawflash_mount) Failed to mount FATFS (%s)", esp_err_to_name(err));
//         return 0;
//     }
    
    
// /*    
//     FIL fsrc, fdst;      // File objects
//     UINT br, bw;         // File read/write count
//     f_open(&fdst, "0:sound/prova.txt", FA_WRITE | FA_CREATE_ALWAYS);
//     f_write(&fdst, "Hello", 5, &bw);           // Write it to the destination file
//     f_close(&fdst);
// */


//     ESP_LOGI(Tag, "Opening file");
//     FILE *f = fopen("/sound/hello.txt", "w");
//     if (f == NULL) {
//         ESP_LOGE(Tag, "Failed to open file for writing");
//         return 0;
//     }
//     fprintf(f, "written using ESP-IDF %s\n", esp_get_idf_version());
//     fclose(f);
//     ESP_LOGI(Tag, "File written");

//     // Open file for reading
//     ESP_LOGI(Tag, "Reading file");
//     f = fopen("/sound/hello.txt", "r");
//     if (f == NULL) {
//         ESP_LOGE(Tag, "Failed to open file for reading");
//         return 0;
//     }
//     char line[128];
//     fgets(line, sizeof(line), f);
//     //printf("%s", line);
//     fclose(f);
//     // strip newline
//     char *pos = strchr(line, '\n');
//     if (pos) {
//         *pos = '\0';
//     }
//     ESP_LOGI(Tag, "Read from file: '%s'", line);


//     //listDir();
//     scan_files("0:sound");

//     // Unmount FATFS
//     ESP_LOGI(Tag, "Unmounting FAT filesystem");
//     //ESP_ERROR_CHECK( esp_vfs_fat_spiflash_unmount(base_path, s_wl_handle));
//     ESP_ERROR_CHECK( esp_vfs_fat_rawflash_unmount(base_path, "vfs"));

//     ESP_LOGI(Tag, "Done");


// /*
//     FF_DIR* *dp;  
//     FILINFO* fno;
//     f_opendir (dp, base_path);
//     if (dp != NULL)
//     {
//       while (f_readdir (dp, fno)) != NULL)
//         puts (ep->d_name);
            
//       (void) closedir (dp);
//       //return 0;
//     }
//     else
//     {
//       perror ("Couldn't open the directory");
//       //return -1;
//     }
// */



/*
  heap_caps_check_integrity_all(true);
  ESP_LOGI(Tag, "heap (cont.)=%u, heap (all)=%u, min_heap=%u", esp_get_free_heap_size(), esp_get_free_internal_heap_size(), esp_get_minimum_free_heap_size());
  int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  int min_free_32bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT);
  printf("||   Miniumum Free DRAM\t|   Minimum Free IRAM\t|| \n");
  printf("||\t%-6d\t\t|\t%-6d\t\t||\n", min_free_8bit_cap, (min_free_32bit_cap - min_free_8bit_cap));
*/
  //Codec_PlayMP3FileFromFlash(0);
  //Codec_PlayMP3FileFromFlash(E_SoundIndex_Startup);

  //xTaskCreatePinnedToCore(stats, "stats", 4096, NULL, 0, NULL, 0);
  //xTaskCreatePinnedToCore(stats2, "stats2", 4096, NULL, 0, NULL, 0);

  //listDir();

/*
  vTaskDelay(10000 / portTICK_PERIOD_MS);
 // FileSystem_CreateFile("512.txt");
  FileSystem_Write("/spiffs/512.txt", temp_buff, 512);
  ESP_LOGI(Tag, "written 512.txt");

//  FileSystem_CreateFile("1024.txt");
  FileSystem_Write("/spiffs/1024.txt", temp_buff, 1024);
  ESP_LOGI(Tag, "written 1024.txt");

//  FileSystem_CreateFile("2048.txt");
  FileSystem_Write("/spiffs/2048.txt", temp_buff, 2048);
  ESP_LOGI(Tag, "written 2048.txt");

//  FileSystem_CreateFile("4096.txt");
  FileSystem_Write("/spiffs/4096.txt", temp_buff, 4096);
  ESP_LOGI(Tag, "written 4096.txt");
*/

  //size_t psram_size = esp_spiram_get_size();
  //printf("PSRAM size: %d bytes\n", psram_size);

  return 0;
}





