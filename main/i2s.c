//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    i2s.c
//! \brief   This module provides the useful functions to use the I2S peripheral
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_partition.h"

#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/i2s.h"

#include "i2s.h"

#include "board.h"
#include "dsp.h"
#include "fifo.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//! I2S number
#define I2S_NUM              0

//! I2S data bits
#define I2S_SAMPLE_BITS     16

//! I2S sample rate
#define I2S_SAMPLE_RATE   16000

//! I2S data format
#define I2S_FORMAT   (I2S_CHANNEL_FMT_ONLY_RIGHT)

//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))

//I2S read buffer length
#define I2S_READ_LEN (16 * 1024)

#define ADC_UNIT_NUM      ADC_UNIT_1

#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0

#define PARTITION_NAME   "sound"

// Sector size of flash
#define FLASH_SECTOR_SIZE         (0x1000)

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE         (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE          (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)

#define RX_BUFFER_SIZE            400 //I2S_READ_LEN

#define NUMBER_OF_SAMPLES        200u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  // Task for reading.
  TaskHandle_t i2s_task;
  // Buffer for reads.
  char buffer[NUMBER_OF_SAMPLES];
} main_data_t;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "i2s";

const esp_partition_t* data_partition = NULL;

static int flash_wr_size = 0;

static uint8_t* flash_read_buff = NULL;
static uint8_t* i2s_write_buff = NULL;

static uint16_t RxBuffer[RX_BUFFER_SIZE];  // RECV_QUEUE_SIZE

T_FifoWords* ProcessFifoRx = NULL;

float input[NUMBER_OF_SAMPLES];
float outputRe[NUMBER_OF_SAMPLES / 2];
float outputIm[NUMBER_OF_SAMPLES / 2];
float outputMag[NUMBER_OF_SAMPLES / 2];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void EraseFlash(void);

static void Scale12BitsTo8Bits(uint8_t* d_buff, uint8_t* s_buff, uint32_t len);

static void PrintBuffer(uint8_t* buf, int length);

static void RunReadingTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void I2S_Init(void)
{
  i2s_config_t i2s_config =
  {
    .mode                 = (I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_SAMPLE_BITS,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .channel_format       = I2S_FORMAT,
    .intr_alloc_flags     = 0,
    .dma_buf_count        = 2,
    .dma_buf_len          = 1024,
    .use_apll             = true
  };

  i2s_pin_config_t i2s_pin_config =
  {
    .bck_io_num   = I2S_SCLK_PIN,
    .ws_io_num    = I2S_LCLK_PIN,
    .data_out_num = I2S_DSIN_PIN,
    .data_in_num  = I2S_DOUT_PIN
  };

  //adc1_config_width(ADC_WIDTH_BIT_12);
  //adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);  // MICROPHONE_PIN

  // Install and start I2S driver
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &i2s_pin_config);

  // Init DAC pad
  //i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);

  // Init ADC pad
  //i2s_set_adc_mode(ADC_UNIT_NUM, ADC_CHANNEL_NUM);

  //i2s_adc_enable(I2S_NUM);

  ProcessFifoRx = Fifo16bits_Create(RxBuffer, RX_BUFFER_SIZE);

  ESP_LOGI(Tag, "I2S is initialized");
}

//_____________________________________________________________________________

void I2S_EnableDAC(void)
{
  //dac_i2s_enable();
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);
}

//_____________________________________________________________________________

void I2S_DisableDAC(void)
{
  i2s_set_dac_mode(I2S_DAC_CHANNEL_DISABLE);
  //dac_i2s_disable();
}

//_____________________________________________________________________________

void I2S_EnableADC(void)
{
  i2s_adc_enable(I2S_NUM);
}

//_____________________________________________________________________________

void I2S_Record(void)
{
  int i2s_read_len = I2S_READ_LEN;

  size_t bytes_read;

  data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                   ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);

  if (data_partition != NULL)
  {
    ESP_LOGI(Tag, "partiton addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size,
             data_partition->label);
  }
  else
  {
    ESP_LOGE(Tag, "Partition error: can't find partition name: %s\n", PARTITION_NAME);
    vTaskDelete(NULL);
  }

  // Erase flash
  EraseFlash();

  flash_wr_size = 0;

  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

  i2s_adc_enable(I2S_NUM);

  while (flash_wr_size < FLASH_RECORD_SIZE)
  {
    //read data from I2S bus, in this case, from ADC.
    i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    PrintBuffer((uint8_t*) i2s_read_buff, 64);
    //save original data from I2S(ADC) into flash.
    esp_partition_write(data_partition, flash_wr_size, i2s_read_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
  }

  i2s_adc_disable(I2S_NUM);
  free(i2s_read_buff);
  i2s_read_buff = NULL;
  free(flash_write_buff);
  flash_write_buff = NULL;

  flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
}

//_____________________________________________________________________________

void I2S_StartReading(void)
{
  main_data_t* data = malloc(sizeof(main_data_t));

  data->i2s_task = NULL;
  memset(data->buffer, 0, NUMBER_OF_SAMPLES);

  ESP_LOGI(Tag, "Enabling I2S ADC");

  //ESP_ERROR_CHECK(i2s_adc_enable(I2S_NUM));

  ESP_LOGI(Tag, "starting I2S read task");

  xTaskCreatePinnedToCore(
    RunReadingTask,     // Function to implement the task
    "I2S read",         // Name of the task
    4096,               // Stack size in words
    data,               // Task input parameter
    2,                  // Priority of the task
    & (data->i2s_task), // Task handle
    0);                 // Core where the task should run
}

//_____________________________________________________________________________

static void RunReadingTask(void* arg)
{
  main_data_t* data = arg;

  while (true)
  {
    size_t data_remaining = NUMBER_OF_SAMPLES;

    ESP_ERROR_CHECK(i2s_adc_enable(I2S_NUM));

    while (data_remaining > 0)
    {
      size_t data_recd = 0;

      ESP_ERROR_CHECK(i2s_read(I2S_NUM, data->buffer, data_remaining, &data_recd, portMAX_DELAY));

      data_remaining -= data_recd;
    }

    //if (bytes_read == sizeof(i2s_read_buff))
    {
      ESP_LOGE(Tag, "COUCOU, %d", data->buffer[0]);
      //ESP_LOGE(Tag, "COUCOU, %d, %d", count, i2s_read_buff[0]);
      i2s_adc_disable(I2S_NUM);
    }

    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void I2S_AcquireMicrophoneValues(void)
{
  static uint16_t count = 0u;

  uint16_t i2s_read_buff[NUMBER_OF_SAMPLES];
  //uint16_t* i2s_read_buff = (uint16_t*)calloc(NUMBER_OF_SAMPLES, sizeof(uint16_t));

  size_t bytes_read;

  //i2s_adc_enable(I2S_NUM);

  i2s_read(I2S_NUM, i2s_read_buff, 400, &bytes_read, portMAX_DELAY);
  //i2s_read(I2S_NUM, i2s_read_buff, sizeof(i2s_read_buff), &bytes_read, portMAX_DELAY);  // portMAX_DELAY

  //Fifo16bits_Write(ProcessFifoRx, i2s_read_buff, bytes_read / 2);

  if (bytes_read == sizeof(i2s_read_buff))
  {
    //ESP_LOGE(Tag, "COUCOU, %d", bytes_read);
    ESP_LOGE(Tag, "COUCOU, %d, %d", count, i2s_read_buff[0]);
    //i2s_adc_disable(I2S_NUM);
  }

  //free(i2s_read_buff);
  //i2s_read_buff = NULL;

  count++;
}

//_____________________________________________________________________________

void I2S_Process(void)
{
  uint16_t input[NUMBER_OF_SAMPLES];
  float value[NUMBER_OF_SAMPLES];
  T_MaxUnsigned maxOutput;

  if (Fifo16bits_GetNumberOfElements(ProcessFifoRx) >= NUMBER_OF_SAMPLES)
  {
    Fifo16bits_Read(ProcessFifoRx, input, NUMBER_OF_SAMPLES);

    //DSP_GetMaxUnsignedValue(input, NUMBER_OF_SAMPLES, &maxOutput);

    //for (uint16_t index = 0; index < NUMBER_OF_SAMPLES; index++)
    {
      ESP_LOGI(Tag, "Amp = %d", input[0]);
      //value[index] = ((input[index] * 1.1) / 4095) * 3.6;
    }

    //DSP_GetMaxValue(&value[0], NUMBER_OF_SAMPLES, &maxOutput);

    //ESP_LOGI(Tag, "Amp = %d, index = %d", maxOutput.Value, maxOutput.Index);

#if 0
    DSP_CalculateDFT(value, outputRe, outputIm, NUMBER_OF_SAMPLES);
    DSP_CalculateDFTOutputMag(outputRe, outputIm, outputMag, (NUMBER_OF_SAMPLES / 2));

    DSP_GetMaxValue(&outputMag[1], ((NUMBER_OF_SAMPLES / 2) - 1), &maxOutput);

    if (maxOutput.Value > 5.0)
    {
      ESP_LOGI(Tag, "Amp = %f, freq = %d [Hz]", maxOutput.Value, (maxOutput.Index + 1) * 10);
    }
#endif
  }
}

#if 0
void I2S_Process(void)
{
  int i2s_read_len = I2S_READ_LEN;
  static int counter = 0;

  size_t bytes_read;

  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));

  //i2s_adc_enable(I2S_NUM);

  while (counter < FLASH_RECORD_SIZE)
  {
    //read data from I2S bus, in this case, from ADC.
    i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
    //i2s_read(I2S_NUM, (void*) RxBuffer, 100, &bytes_read, 0);
    //PrintBuffer((uint8_t*) i2s_read_buff, 64);
    //counter += i2s_read_len;
    //ESP_LOGI(Tag, "Counter = %d", bytes_read);
  }

  //if (Fifo_IsFull(ProcessFifoRx))
  {
    //Fifo_Reset(ProcessFifoRx);
  }



  i2s_adc_disable(I2S_NUM);
  free(i2s_read_buff);
  i2s_read_buff = NULL;

  //flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
}
#endif

//_____________________________________________________________________________

void I2S_Write(void)
{
  uint16_t table[4] = {0x24AA, 0x3912, 0x48CC, 0x72FA};
  size_t i2s_bytes_write = 0;

  i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);
  i2s_write(I2S_NUM, table, 8, &i2s_bytes_write, portMAX_DELAY);
}

//_____________________________________________________________________________

void I2S_Read(void)
{
#if 0
  size_t bytes_read;
  int i2s_read_len = I2S_READ_LEN;
  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));

  i2s_adc_enable(I2S_NUM);
  //int16_t sensors[4];

  //i2s_read(i2s_port_t i2s_num, void *dest, size_t size, size_t *bytes_read, TickType_t ticks_to_wait);
  i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);
#endif
}

//_____________________________________________________________________________

void I2S_ReadFromFlash(void)
{
  size_t bytes_written;

  for (int rd_offset = 0; rd_offset < flash_wr_size; rd_offset += FLASH_SECTOR_SIZE)
  {
    // Read I2S(ADC) original data from flash
    esp_partition_read(data_partition, rd_offset, flash_read_buff, FLASH_SECTOR_SIZE);

    // Process data and scale to 8bit for I2S DAC.
    Scale12BitsTo8Bits(i2s_write_buff, flash_read_buff, FLASH_SECTOR_SIZE);

    // Send data
    i2s_write(I2S_NUM, i2s_write_buff, FLASH_SECTOR_SIZE, &bytes_written, portMAX_DELAY);
    printf("playing: %d %%\n", rd_offset * 100 / flash_wr_size);
  }
}

//_____________________________________________________________________________

static void EraseFlash(void)
{
  ESP_LOGI(Tag, "Erasing flash");

  const esp_partition_t* data_partition = NULL;

  data_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA,
                   ESP_PARTITION_SUBTYPE_DATA_FAT, PARTITION_NAME);

  if (data_partition != NULL)
  {
    ESP_LOGI(Tag, "Partition addr: 0x%08x; size: %d; label: %s\n", data_partition->address, data_partition->size,
             data_partition->label);
  }

  ESP_LOGI(Tag, "Erase size: %d Bytes\n", FLASH_ERASE_SIZE);
  ESP_ERROR_CHECK(esp_partition_erase_range(data_partition, 0, FLASH_ERASE_SIZE));
}

//_____________________________________________________________________________

static void Scale12BitsTo8Bits(uint8_t* d_buff, uint8_t* s_buff, uint32_t len)
{
  uint32_t j = 0;
  uint32_t dac_value = 0;

#if (I2S_SAMPLE_BITS == 16)
  for (int i = 0; i < len; i += 2)
  {
    dac_value = ((((uint16_t)(s_buff[i + 1] & 0xf) << 8) | ((s_buff[i + 0]))));
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 4096;
  }
#else
  for (int i = 0; i < len; i += 4)
  {
    dac_value = ((((uint16_t)(s_buff[i + 3] & 0xf) << 8) | ((s_buff[i + 2]))));
    d_buff[j++] = 0;
    d_buff[j++] = 0;
    d_buff[j++] = 0;
    d_buff[j++] = dac_value * 256 / 4096;
  }
#endif
}

//_____________________________________________________________________________

static void PrintBuffer(uint8_t* buf, int length)
{
  printf("======\n");

  for (int i = 0; i < length; i++)
  {
    printf("%02x ", buf[i]);

    if ((i + 1) % 8 == 0)
    {
      printf("\n");
    }
  }
  printf("======\n");
}
