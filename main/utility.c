//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    utility.c
//! \brief   This module provides general utility functions
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#include "utility.h"
#include "esp_timer.h"
#include "stm32_spi.h"
#include "leds.h"
#include "esp_system.h"
#include "esp32/spiram.h"
#include "esp_heap_caps.h"

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

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

int64_t getTimeUs(void) {
	// Time based on 240 MHz clock
	//return portGET_RUN_TIME_COUNTER_VALUE()
	// Time based on 1 us resolution
	return esp_timer_get_time();
}

void turnOffAllSensors(void) {
	Leds_SetBodyBrightness(0u, 0u, 0u);
	Leds_SetCircleBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
	Leds_SetLegoFrontBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
	Leds_SetLegoBackBrightness(0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u);
	SetMotorTargets(0, 0);
}

void printMemInfo(void) {
	uint32_t currHeapSize = 0, minHeapSize = 0;
	//Heap
	//printf("heap_free_size %d\n",heap_caps_get_free_size(MALLOC_CAP_8BIT));
	// Heap statistics
	// DRAM heap size: use heap_caps_get_total_size(MALLOC_CAP_8BIT) or heap_caps_get_total_size(MALLOC_CAP_8BIT|MALLOC_CAP_32BIT) because only DRAM can handle 8bit access
	// Total (DRAM + IRAM) heap size: use heap_caps_get_total_size(MALLOC_CAP_32BIT) because both handle 32bit access
	// IRAM heap size: use (heap_caps_get_total_size(MALLOC_CAP_32BIT) - heap_caps_get_total_size(MALLOC_CAP_8BIT))
	printf("tot heap=%d, dram heap=%d, iram heap=%d, free internal=%u\n", heap_caps_get_total_size(MALLOC_CAP_32BIT), heap_caps_get_total_size(MALLOC_CAP_8BIT), (heap_caps_get_total_size(MALLOC_CAP_32BIT) - heap_caps_get_total_size(MALLOC_CAP_8BIT)), esp_get_free_internal_heap_size());
	printf("tot free heap=%d, tot dram heap=%d, tot free iram heap=%d\n", heap_caps_get_free_size(MALLOC_CAP_32BIT), heap_caps_get_free_size(MALLOC_CAP_8BIT), (heap_caps_get_free_size(MALLOC_CAP_32BIT) - heap_caps_get_free_size(MALLOC_CAP_8BIT)));
	currHeapSize = heap_caps_get_free_size(MALLOC_CAP_32BIT);
	minHeapSize = heap_caps_get_minimum_free_size(MALLOC_CAP_32BIT);
	printf("free heap=%d, min free heap=%d\n\n", currHeapSize, minHeapSize);

/*
  heap_caps_check_integrity_all(true);
  ESP_LOGI(Tag, "heap (cont.)=%u, heap (all)=%u, min_heap=%u", esp_get_free_heap_size(), esp_get_free_internal_heap_size(), esp_get_minimum_free_heap_size());
  int min_free_8bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
  int min_free_32bit_cap = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_32BIT);
  printf("||   Miniumum Free DRAM\t|   Minimum Free IRAM\t|| \n");
  printf("||\t%-6d\t\t|\t%-6d\t\t||\n", min_free_8bit_cap, (min_free_32bit_cap - min_free_8bit_cap));
*/	

  //size_t psram_size = esp_spiram_get_size();
  //printf("PSRAM size: %d bytes\n", psram_size);
}
