//_____________________________________________________________________________
//
// Copyright (C) 2018                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    sound.c
//! \brief   This module provides the useful functions to generate the sound
//!
//! \author  Vincent Gonet
//!
//! \version $Id: sound.c 18076 2017-04-20 12:28:12Z v.gonet $
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

#include "soc/rtc_io_reg.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"

#include "driver/adc.h"
#include "driver/dac.h"
#include "driver/i2s.h"

#include "sound.h"

#include "audio_file.h"
#include "board.h"
#include "gpio.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

// Enable record sound and save in flash
#define RECORD_IN_FLASH_EN        (1)

// Enable replay recorded sound in flash
#define REPLAY_FROM_FLASH_EN      (1)

#define SINE_TABLE_SIZE    256

//i2s number
#define I2S_NUM (0)

//i2s data bits
#define I2S_SAMPLE_BITS (16)

//I2S read buffer length
#define I2S_READ_LEN (16 * 1024)

//i2s sample rate
#define I2S_SAMPLE_RATE (16000)

//I2S data format
#define I2S_FORMAT   (I2S_CHANNEL_FMT_ALL_RIGHT)  //(I2S_CHANNEL_FMT_ONLY_RIGHT)

//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))

#define ADC_UNIT_NUM      ADC_UNIT_1

#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0

#define PARTITION_NAME   "sound"

//flash record size, for recording 5 seconds' data
#define FLASH_RECORD_SIZE         (I2S_CHANNEL_NUM * I2S_SAMPLE_RATE * I2S_SAMPLE_BITS / 8 * 5)
#define FLASH_ERASE_SIZE          (FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE == 0) ? FLASH_RECORD_SIZE : FLASH_RECORD_SIZE + (FLASH_SECTOR_SIZE - FLASH_RECORD_SIZE % FLASH_SECTOR_SIZE)

//sector size of flash
#define FLASH_SECTOR_SIZE         (0x1000)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sound";

static const T_GpioPinConfig PinConfig = {GPIO0_PIN,
                                          E_GpioMode_Output,
                                          E_GpioResistor_None,
                                          E_GpioLevel_Low,
                                          E_GpioInterrupt_Disable
                                         };

int clk_8m_div = 4;      // RTC 8M clock divider (division is by clk_8m_div+1, i.e. 0 means 8MHz frequency)
int frequency_step = 8;  // Frequency step for CW generator
int scale = 1;           // 50% of the full scale
int offset;              // leave it default / 0 = no any offset
int invert = 2; // invert MSB to get sine waveform

const esp_partition_t* data_partition = NULL;

int i2s_read_len = I2S_READ_LEN;
int flash_wr_size = 0;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void EraseFlash(void);

static int ScaleDACData(uint8_t* dest, uint8_t* src, uint32_t len);

static void Scale12BitsTo8Bits(uint8_t* dest, uint8_t* src, uint32_t len);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sound_Init(void)
{
  int16_t i2s_num = I2S_NUM;

  i2s_config_t i2s_config =
  {
    .mode                 = (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_SAMPLE_BITS,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .channel_format       = I2S_FORMAT,
	.intr_alloc_flags     = 0,
    .dma_buf_count        = 2,
    .dma_buf_len          = 1024
  };

  // Install and start I2S driver
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

  // Init DAC pad
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);

  ESP_LOGI(Tag, "Sound is initialized");
}

#if 0
void Sound_Init(void)
{
  int16_t i2s_num = I2S_NUM;

  i2s_config_t i2s_config =
  {
    .mode                 = (I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN),
    .sample_rate          = I2S_SAMPLE_RATE,
    .bits_per_sample      = I2S_SAMPLE_BITS,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .channel_format       = I2S_FORMAT,
	.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 2,
    .dma_buf_len          = 1024
  };

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);  // MICROPHONE_PIN

  // Install and start I2S driver
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

  // Init DAC pad
  //i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);

  // Init ADC pad
  i2s_set_adc_mode(ADC_UNIT_NUM, ADC_CHANNEL_NUM);

  //i2s_adc_enable(I2S_NUM);

  ESP_LOGI(Tag, "Sound is initialized");
}
#endif

#if 0
void Sound_Init(void)
{
  // Enable tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  // Enable / connect tone generator
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
  // Invert MSB, otherwise part of waveform will have inverted
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);

  dac_output_enable(DAC_CHANNEL_1);
}
#endif

//_____________________________________________________________________________

void Sound_Task(void)
{
  size_t bytes_written;

  uint8_t* flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

  //while (1)
  {
    // Read flash and replay the sound via DAC
#if REPLAY_FROM_FLASH_EN
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
#endif

    //vTaskDelay(100 / portTICK_PERIOD_MS);
    //example_reset_play_mode();
    //i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_SAMPLE_BITS, I2S_CHANNEL_NUM);
  }

  free(flash_read_buff);
  free(i2s_write_buff);
  vTaskDelete(NULL);
}

#if 0
void Sound_Task(int16_t note)
{
  Sound_SetFrequency(clk_8m_div, note);

  /* Tune parameters of channel 2 only
   * to see and compare changes against channel 1
   */
  Sound_ScaleOutput(scale);
  Sound_OffsetOutput(offset);
  Sound_InvertOutput(invert);

  float frequency = RTC_FAST_CLK_FREQ_APPROX / (1 + clk_8m_div) * (float) note / 65536;
  printf("clk_8m_div: %d, frequency step: %d, frequency: %.0f Hz\n", clk_8m_div, note, frequency);
  //printf("DAC2 scale: %d, offset %d, invert: %d\n", scale, offset, invert);
}
#endif

//_____________________________________________________________________________

void Sound_Record(void)
{
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

  i2s_read_len = I2S_READ_LEN;
  flash_wr_size = 0;
  size_t bytes_read;

  // Record audio from ADC and save in flash
#if RECORD_IN_FLASH_EN
  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  i2s_adc_enable(I2S_NUM);

  while (flash_wr_size < FLASH_RECORD_SIZE)
  {
    // Read data from I2S bus, in this case, from ADC.
    i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

    //Save original data from I2S(ADC) into flash.
    esp_partition_write(data_partition, flash_wr_size, i2s_read_buff, i2s_read_len);
    flash_wr_size += i2s_read_len;
    ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
  }

  i2s_adc_disable(I2S_NUM);
  free(i2s_read_buff);
  i2s_read_buff = NULL;
  free(flash_write_buff);
  flash_write_buff = NULL;
#endif
}

//_____________________________________________________________________________

void Sound_PlayFile(void* pvParameter)
{
  size_t bytes_written;
  uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

  while (1)
  {
    int offset = 0;
    int tot_size = sizeof(audio_table);

    // Set I2S clock for playing the audio file
    i2s_set_clk(I2S_NUM, 16000, I2S_SAMPLE_BITS, 1);

    while (offset < tot_size)
    {
      int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
      int i2s_wr_len = ScaleDACData(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);
      i2s_write(I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
      offset += play_len;
      //example_disp_buf((uint8_t*) i2s_write_buff, 32);
    }

    //vTaskDelay(100 / portTICK_PERIOD_MS);
    vTaskDelete(NULL);

    // Reset I2S clock and mode
    i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_SAMPLE_BITS, I2S_CHANNEL_NUM);
  }

  free(i2s_write_buff);
  vTaskDelete(NULL);
}

//_____________________________________________________________________________

void Sound_SetFrequency(int16_t clk_8m_div, int16_t frequency_step)
{
  REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, clk_8m_div);
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, frequency_step, SENS_SW_FSTEP_S);
}

//_____________________________________________________________________________

void Sound_ScaleOutput(int16_t scale)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
}

//_____________________________________________________________________________

void Sound_OffsetOutput(int16_t offset)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_DC1, offset, SENS_DAC_DC1_S);
}

//_____________________________________________________________________________

void Sound_InvertOutput(int16_t invert)
{
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, invert, SENS_DAC_INV1_S);
}

//_____________________________________________________________________________

static void EraseFlash(void)
{
#if RECORD_IN_FLASH_EN
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
#else
  ESP_LOGI(Tag, "Skip flash erasing");
#endif
}

//_____________________________________________________________________________

static int ScaleDACData(uint8_t* dest, uint8_t* src, uint32_t len)
{
  uint32_t j = 0;

#if (I2S_SAMPLE_BITS == 16)
  for (int i = 0; i < len; i++)
  {
    dest[j++] = 0;
    dest[j++] = src[i];
  }

  return (len * 2);
#else
  for (int i = 0; i < len; i++)
  {
    dest[j++] = 0;
    dest[j++] = 0;
    dest[j++] = 0;
    dest[j++] = src[i];
  }

  return (len * 4);
#endif
}

//_____________________________________________________________________________

static void Scale12BitsTo8Bits(uint8_t* dest, uint8_t* src, uint32_t len)
{
  uint32_t j = 0;
  uint32_t dac_value = 0;

#if (I2S_SAMPLE_BITS == 16)
  for (int i = 0; i < len; i += 2)
  {
    dac_value = ((((uint16_t)(src[i + 1] & 0xf) << 8) | ((src[i + 0]))));
    dest[j++] = 0;
    dest[j++] = dac_value * 256 / 4096;
  }
#else
  for (int i = 0; i < len; i += 4)
  {
    dac_value = ((((uint16_t)(src[i + 3] & 0xf) << 8) | ((src[i + 2]))));
    dest[j++] = 0;
    dest[j++] = 0;
    dest[j++] = 0;
    dest[j++] = dac_value * 256 / 4096;
  }
#endif
}

//_____________________________________________________________________________

void Sound_StartRecord(void)
{
  i2s_adc_enable(I2S_NUM);
}

