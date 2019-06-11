//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ota_update.c
//! \brief   This module provides the useful functions to perform the over-the-air update
//
//! \author  Vincent Gonet
//
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_spi_flash.h>

#include "ota_update.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUFFER_SIZE    4096u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "ops_update";

// SPI flash address for next write operation.
static uint32_t FlashCurrentAddress;

static const esp_partition_t* OtaPartition;

static esp_ota_handle_t OtaHandle;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static const esp_partition_t* FindNextBootPartition(void);

static void Dump128bytes(uint32_t addr, uint8_t* p);

static void Dump16bytes(uint32_t addr, uint8_t* p);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void OtaUpdate_Init(void)
{
  spi_flash_init();
}

//_____________________________________________________________________________

bool OtaUpdate_IsInProgress(void)
{
  return OtaPartition ? 1 : 0;
}

//_____________________________________________________________________________

T_Ota_Status OtaUpdate_Start(void)
{
  T_Ota_Status status = E_Ota_Status_StartFailed;
  esp_err_t result;

  OtaPartition = FindNextBootPartition();

  if (OtaPartition)
  {
    FlashCurrentAddress = OtaPartition->address;
    ESP_LOGI(Tag, "Set start address for flash writes to 0x%08x", FlashCurrentAddress);

    // TODO
    // This operation would trigger the watchdog of the currently running task if we fed it with the full partition size.
    // To avoid the issue, we erase only a small part here and afterwards erase every page before writing to it.
    result = esp_ota_begin(OtaPartition, BUFFER_SIZE, &OtaHandle);
    ESP_LOGI(Tag, "Result from esp_ota_begin: %d %d", result, OtaHandle);

    if (result == ESP_OK)
    {
      status = E_Ota_Status_Ok;
    }
    else
    {
      ESP_LOGE(Tag, "Failed to start, error %d", result);
    }
  }
  else
  {
    status = E_Ota_Status_PartitionNotFound;
  }

  return status;
}

//_____________________________________________________________________________

T_Ota_Status OtaUpdate_WriteHexData(const char* hexData, int len)
{
  uint8_t buffer[BUFFER_SIZE];
  uint16_t index;
  T_Ota_Status status = E_Ota_Status_WriteFailed;
  esp_err_t result;

  for (index = 0u; index < BUFFER_SIZE; index++)
  {
    buffer[index] = (index < len) ? hexData[index] : 0xff;
  }

  // Erase flash pages at 4k boundaries.
  if (FlashCurrentAddress % 0x1000 == 0)
  {
    int flashSectorToErase = FlashCurrentAddress / 0x1000;
    ESP_LOGI(Tag, "Erasing flash sector %d", flashSectorToErase);
    spi_flash_erase_sector(flashSectorToErase);
  }

  // Write data into flash memory.
  ESP_LOGI(Tag, "Writing flash at 0x%08x...", FlashCurrentAddress);
  // esp_err_t result = spi_flash_write(FlashCurrentAddress, buffer, 4096);
  result = esp_ota_write(OtaHandle, buffer, len);

  if (result == ESP_OK)
  {
    FlashCurrentAddress += len;
    status =  E_Ota_Status_Ok;
  }
  else
  {
    ESP_LOGE(Tag, "Failed to write flash at address 0x%08x, error %d", FlashCurrentAddress, result);
  }

  return status;
}

//_____________________________________________________________________________

T_Ota_Status OtaUpdate_Finish(void)
{
  T_Ota_Status status = E_Ota_Status_PartitionNotActivated;
  esp_err_t result;

  if (OtaPartition)
  {
    result = esp_ota_end(OtaHandle);

    if (result == ESP_OK)
    {
      result = esp_ota_set_boot_partition(OtaPartition);

      if (result == ESP_OK)
      {
        ESP_LOGI(Tag, "Boot partition activated: %s", OtaPartition->label);
        status = E_Ota_Status_Ok;
      }
      else
      {
        ESP_LOGE(Tag, "Failed to activate boot partition %s, error %d", OtaPartition->label, result);
        OtaPartition = NULL;
      }
    }
    else
    {
      ESP_LOGE(Tag, "Failed to end, error %d", result);
      status = E_Ota_Status_EndFailed;
    }
  }
  else
  {
    status = E_Ota_Status_PartitionNotFound;
  }

  return status;
}

//_____________________________________________________________________________

void OtaUpdate_DumpInformation(void)
{
  uint8_t firstBuffer[BUFFER_SIZE];
  uint8_t secondBuffer[BUFFER_SIZE];
  esp_err_t result;

  ESP_LOGI(Tag, "OTA Dump Information");

  size_t chipSize = spi_flash_get_chip_size();
  ESP_LOGI(Tag, "flash chip size = %d", chipSize);

  ESP_LOGI(Tag, "Reading flash at 0x00000000....");
  result = spi_flash_read(0, firstBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0, firstBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00001000....");
  result = spi_flash_read(0x1000, firstBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x1000, firstBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00004000....");
  result = spi_flash_read(0x4000, firstBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x4000, firstBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x0000D000....");
  result = spi_flash_read(0xD000, firstBuffer, BUFFER_SIZE);
  Dump128bytes(0xD000, firstBuffer);
  result = spi_flash_read(0xE000, firstBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0xE000, firstBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00010000....");
  result = spi_flash_read(0x10000, firstBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x10000, firstBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00020000....");
  result = spi_flash_read(0x20000, secondBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x20000, secondBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00110000....");
  result = spi_flash_read(0x110000, secondBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x110000, secondBuffer);

  ESP_LOGI(Tag, "Reading flash at 0x00210000....");
  result = spi_flash_read(0x210000, secondBuffer, BUFFER_SIZE);
  ESP_LOGI(Tag, "Result = %d", result);
  Dump128bytes(0x210000, secondBuffer);
}

//_____________________________________________________________________________

static const esp_partition_t* FindNextBootPartition(void)
{
  // Factory -> OTA_0
  // OTA_0   -> OTA_1
  // OTA_1   -> OTA_0

  const esp_partition_t* currentBootPartition = esp_ota_get_boot_partition();
  const esp_partition_t* nextBootPartition = NULL;

  if (!strcmp("factory", currentBootPartition->label))
  {
    nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_0");
  }

  if (!strcmp("ota_0", currentBootPartition->label))
  {
    nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_1");
  }

  if (!strcmp("ota_1", currentBootPartition->label))
  {
    nextBootPartition = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, "ota_0");
  }

  if (nextBootPartition)
  {
    ESP_LOGI(Tag, "Found next boot partition: %02x %02x 0x%08x %s",
             nextBootPartition->type, nextBootPartition->subtype, nextBootPartition->address, nextBootPartition->label);
  }
  else
  {
    ESP_LOGE(Tag, "Failed to determine next boot partition from current boot partition: %s",
             currentBootPartition ? currentBootPartition->label : "NULL");
  }

  return nextBootPartition;
}

//_____________________________________________________________________________

static void Dump128bytes(uint32_t addr, uint8_t* p)
{
  uint8_t index;
  uint32_t addr2;

  for (index = 0u; index < 8u; index++)
  {
    addr2 = addr + 16u * index;
    Dump16bytes(addr2, &p[16u * index]);
  }
}

//_____________________________________________________________________________

static void Dump16bytes(uint32_t addr, uint8_t* p)
{
  ESP_LOGI(Tag, "%08X : %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
           addr, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]);
}
