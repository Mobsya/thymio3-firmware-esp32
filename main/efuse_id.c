//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    efuse_id.h
//! \brief   This module provides the useful functions to read and program the
//!          Thymio3's ID in the ESP32 eFuse 
//!
//! \author  Daniel Burnier
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------
#include "esp_log.h"
#include "esp_efuse.h"
#include "efuse_id.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define ENTRY_SIZE 32 // Each entry is 32 bits (4 bytes) in EFUSE BLK3
#define INDENT "  ->  " // Indentation for log messages

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum {
    DATA0 = 0,
    DATA1,
    DATA2,
    DATA3,
    DATA4,
    DATA5,
    DATA6,
    DATA7,
    ENTRIES_NUMBER,
} efuse_wdata_t;

typedef struct {
    uint32_t data;
    efuse_wdata_t offset;    // 0..7 (DATA0..DATA7)
} efuse_entry_t;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *TAG = "EFUSE_ID";

static efuse_entry_t efuse_entries[ENTRIES_NUMBER] = {
    {NO_ID,      DATA0}, // DATA0 - INVALID_ID for esp32 but enabled here for test purpose
    {INVALID_ID, DATA1}, // DATA1 - INVALID_ID for esp32
    {NO_ID,      DATA2}, // DATA2 - 1st entry
    {INVALID_ID, DATA3}, // DATA3 - INVALID_ID for esp32
    {INVALID_ID, DATA4}, // DATA4 - INVALID_ID for esp32
    {INVALID_ID, DATA5}, // DATA5 - INVALID_ID for esp32
    {NO_ID,      DATA6}, // DATA6 - 2nd entry
    {NO_ID,      DATA7}, // DATA7 - 3rd entry
};



//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

uint32_t getCurrentEfuseID(void){
    uint32_t currentEfuseID = INVALID_ID;
    for (uint8_t i = 0; i < ENTRIES_NUMBER; i++) {
        if (efuse_entries[i].data == INVALID_ID) {
            continue; // Skip invalid entries
        }
        currentEfuseID = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
        if (currentEfuseID != INVALID_ID) {
            return currentEfuseID; // Return the first no invalid entry found
        }
    }
    return currentEfuseID; // Return INVALID_ID if no valid entry found
}

//_____________________________________________________________________________

int8_t setCurrentEfuseID(uint32_t new_id) {
    uint32_t efuse_data = INVALID_ID;
    
    if ((new_id == INVALID_ID) || (new_id == NO_ID)) {
        ESP_LOGE(TAG, "Invalid new ID requested to be programmed in EFUSE BLK3: 0x%08X\n", new_id);
        return -1; // Invalid ID
    }
    
    // Search for first valid entry
    for(int i = 0; i < ENTRIES_NUMBER; i++) {
        if(efuse_entries[i].data == INVALID_ID) {
            continue; // Skip invalid entries
        }
        // Read the content of the current entry in EFUSE BLK3
        uint32_t efuse_data = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
        ESP_LOGI(TAG, "Content read in EFUSE BLK3 DATA%d : 0x%08X\n", efuse_entries[i].offset, efuse_data);
        if(efuse_data == INVALID_ID) {
            ESP_LOGI(TAG, INDENT "Actual slot content INVALID_ID\n");
            continue; // Try next entry
        }
        // Check if the new ID is different from the current one
        if(efuse_data != new_id) {
            ESP_LOGI(TAG, INDENT "Current ID 0x%08X is different from new ID 0x%08X, will program it\n", efuse_data, new_id);
            // Change only the bits that are 0 and must be changed to 1. Bits that are already 1 will remain 1 (cannot be changed back to 0).
            uint32_t data = new_id & ~efuse_data;
            if (efuse_data & ~new_id) {
                ESP_LOGE(TAG, "This entry in EFUSE BLK3 DATA%d can't be programmed: 0x%08X -> 0x%08X not allowed\n", efuse_entries[i].offset, efuse_data, new_id);
                return -2; // Entry is already used and cannot be written
            } else { // Need to program some bits 0 -> 1
                ESP_ERROR_CHECK(esp_efuse_batch_write_begin());
                ESP_LOGI(TAG, "Program or change ID in EFUSE BLK3 DATA%d: 0x%08X OR 0x%08X -> 0x%08X\n", efuse_entries[i].offset, efuse_data, data, new_id);
                ESP_ERROR_CHECK(esp_efuse_write_block(EFUSE_BLK3, &data, i * ENTRY_SIZE, ENTRY_SIZE));
                // Actually burn the fuses
                ESP_ERROR_CHECK(esp_efuse_batch_write_commit());
                // Recheck the same entry after writing
                efuse_data = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
                if(efuse_data != new_id) {
                    ESP_LOGE(TAG, "Failed to program the ID in EFUSE BLK3 DATA%d: read 0x%08X instead writen 0x%08X", efuse_entries[i].offset, efuse_data, new_id);
                    return -5; // Failed to program the entry
                } else {
                    ESP_LOGI(TAG, "Successfully programmed the ID in EFUSE BLK3 DATA%d", efuse_entries[i].offset);
                    return 0; // Successfully programmed the entry
                }
            }
        } else {
            ESP_LOGI(TAG, INDENT "Current ID 0x%08X is the same as new ID 0x%08X, no need to program it\n", efuse_data, new_id);
            return 0; // No need to program the entry
        }
        if(efuse_data == INVALID_ID) {
            ESP_LOGW(TAG, "Invalid ID found in EFUSE BLK3 DATA%d: 0x%08X", efuse_entries[i].offset, efuse_data);
            continue; // Try next entry
        }
    }

    if(efuse_data == INVALID_ID) {
        ESP_LOGE(TAG, "There is not anymore available entry in EFUSE BLK3 DATA");
        return -3; // No more free entry to write this new id
    } else if(efuse_data != NO_ID) {
        ESP_LOGW(TAG, "This entry is already used and can't be written with new ID");
        return -2; // Entry is already used and cannot be written
    }
    return -10; // Unexpected case
}

//_____________________________________________________________________________

int8_t killCurrentEfuseID(void) {
    if (getAvailableEfuseEntries() >= 2) { // At least one entry must be available to write a new id after killing the current entry
        ESP_LOGI(TAG, "There is at least one free entry to write a new ID after killing the current entry");
    } else {
        ESP_LOGE(TAG, "There is no more free entry to write a new ID after killing the current entry");
        return -1; // No more free entry to write a new id
    }

    for (uint8_t i = 0; i < ENTRIES_NUMBER; i++) {
        if (efuse_entries[i].data == INVALID_ID) {
            continue;
        } else {
            uint32_t efuse_data = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
            if (efuse_data != INVALID_ID) {
                // Kill the entry by writing INVALID_ID
                uint32_t data = INVALID_ID & ~efuse_data;
                ESP_ERROR_CHECK(esp_efuse_batch_write_begin());
                ESP_LOGI(TAG, "Kill the entry in EFUSE BLK3 DATA%d\n", efuse_entries[i].offset);
                ESP_ERROR_CHECK(esp_efuse_write_block(EFUSE_BLK3, &data, i * ENTRY_SIZE, ENTRY_SIZE));
                // Actually burn the fuses
                ESP_ERROR_CHECK(esp_efuse_batch_write_commit());
                // Recheck the same entry after writing
                efuse_data = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
                if (efuse_data != INVALID_ID) {
                    ESP_LOGE(TAG, "Failed to kill the ID in EFUSE BLK3 DATA%d: read 0x%08X instead writen 0x%08X\n", efuse_entries[i].offset, efuse_data, INVALID_ID);
                    return -1; // Failed to kill the entry
                } else {
                    ESP_LOGI(TAG, "Successfully killed the ID in EFUSE BLK3 DATA%d\n", efuse_entries[i].offset);
                    return 0; // Successfully killed the entry
                }
            }
        }
    }
    return -1; // No valid entry found to kill
}

//_____________________________________________________________________________

uint8_t getAvailableEfuseEntries(void) {
    uint8_t usable_entries = 0;
    for (uint8_t i = 0; i < ENTRIES_NUMBER; i++) {
        uint32_t efuse_data = esp_efuse_read_reg(EFUSE_BLK3, efuse_entries[i].offset);
        if ((efuse_entries[i].data != INVALID_ID) && (efuse_data != INVALID_ID)) {
            usable_entries++; // Entry is usable
        }
    }
    return usable_entries;
}
    
//_____________________________________________________________________________
