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

#include <math.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/dac.h"

#include "driver/i2s.h"

#include <rom/ets_sys.h>

#include "sound.h"

#include "board.h"
#include "gpio.h"

#include "leds.h"

#include "audio_file.h"

#include "timer_hw.h"


//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

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
#define I2S_FORMAT (I2S_CHANNEL_FMT_RIGHT_LEFT)

//I2S channel number
#define I2S_CHANNEL_NUM ((I2S_FORMAT < I2S_CHANNEL_FMT_ONLY_RIGHT) ? (2) : (1))

#define ADC_UNIT_NUM      ADC_UNIT_1

#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_SoundState_Idle,
  E_SOundState_Start,
  E_SOundState_Stop
} T_SoundState;

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
                                          E_GpioInterrupt_Disable};

static uint8_t SineTable[SINE_TABLE_SIZE];
static float SoundTable[SINE_TABLE_SIZE];

static bool SoundIsInProgress = false;

static T_SoundState State = E_SoundState_Idle;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void SetI2SClock(void);

static void ResetI2SClock(void);

static uint32_t ScaleADCToDAC(uint8_t* dest, uint8_t* source, uint32_t length);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sound_InitSine(T_Volume volume)
{
  float conversionFactor = (2.0f * M_PI) / SINE_TABLE_SIZE;
  float angle_rad = 0.0f;

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    angle_rad = (pos * conversionFactor);
    SineTable[pos] = ((sin(angle_rad) * volume) + volume);
  }

  dac_output_enable(DAC_CHANNEL_1);
}

#if 0
void Sound_Init(void)
{
#if 0
  float conversionFactor = (2.0f * M_PI) / SINE_TABLE_SIZE;
  float angle_rad = 0.0f;

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    angle_rad = (pos * conversionFactor);
    SineTable[pos] = ((sin(angle_rad) * 127) + 128);
  }
#endif

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    SoundTable[pos] = 0;
  }

  Sound_Add(1, 10);
  //Sound_Add(5, 50);

  dac_output_enable(DAC_CHANNEL_1);
}
#endif

//#if 0
void Sound_Init(void)
{
  int i2s_num = I2S_NUM;

  i2s_config_t i2s_config =
  {
    .mode = I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN | I2S_MODE_ADC_BUILT_IN,
    .sample_rate =  I2S_SAMPLE_RATE,
    .bits_per_sample = I2S_SAMPLE_BITS,
	.communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .channel_format = I2S_FORMAT,
	.intr_alloc_flags = 0,
	.dma_buf_count = 2,
	.dma_buf_len = 1024
  };

  // Install and start the I2S driver
  i2s_driver_install(i2s_num, &i2s_config, 0, NULL);

  // Inititialize the DAC pad
  i2s_set_dac_mode(I2S_DAC_CHANNEL_RIGHT_EN);

  // Inititialize the ADC pad
  i2s_set_adc_mode(ADC_UNIT_NUM, ADC_CHANNEL_NUM);

  //Gpio_ConfigurePin(&PinConfig);

  //Sound_Enable();
}
//#endif
//_____________________________________________________________________________

#if 0
void Sound_Task(void)
{
  Sound_Generate();
}
#endif

//#if 0
void Sound_Task(void)
{
  int i2s_read_len = I2S_READ_LEN;
  uint8_t* flash_read_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  uint8_t* i2s_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));
  size_t bytes_written;

  while (1)
  {
    //4. Play an example audio file(file format: 8bit/16khz/single channel)
    int offset = 0;
    int tot_size = sizeof(audio_table);

    SetI2SClock();

    while (offset < tot_size)
    {
      int play_len = ((tot_size - offset) > (4 * 1024)) ? (4 * 1024) : (tot_size - offset);
      int i2s_wr_len = ScaleADCToDAC(i2s_write_buff, (uint8_t*)(audio_table + offset), play_len);

      i2s_write(I2S_NUM, i2s_write_buff, i2s_wr_len, &bytes_written, portMAX_DELAY);
      offset += play_len;
      //example_disp_buf((uint8_t*) i2s_write_buff, 32);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
    ResetI2SClock();
  }

  free(flash_read_buff);
  free(i2s_write_buff);
  vTaskDelete(NULL);
}
//#endif

//_____________________________________________________________________________

void Sound_Generate(void)
{
  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    dac_output_voltage(DAC_CHANNEL_1, (uint8_t)SoundTable[pos]);
    //ets_delay_us(5);
    //ESP_LOGI(Tag, "table[%d] = %d", pos, SineTable[pos]);
    //vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

void Sound_Add(float frequency, float amplitude)
{
  float angle_rad = 0.0f;
  float conversionFactor = (2 * M_PI) / SINE_TABLE_SIZE;

  static uint8_t count = 0u;

  for (int16_t pos = 0; pos < SINE_TABLE_SIZE; pos++)
  {
    angle_rad = ((float)pos * conversionFactor);
    SoundTable[pos] += ((sin(angle_rad * frequency) * amplitude) + (amplitude + 1.0f));
#if 0
    if (count == 1)
    {
      ESP_LOGI(Tag, "table[%d] = %f", pos, SoundTable[pos]);
      vTaskDelay(10 / portTICK_PERIOD_MS);
    }
#endif
  }

  count++;
}

//_____________________________________________________________________________

void Sound_Enable(void)
{
  // TODO GPIO0 shall be set in output on ESP32 and input on STM32 and next we can drive this pin
  Gpio_SetPinLevel(GPIO0_PIN, E_GpioLevel_High);
}

//_____________________________________________________________________________

void Sound_PlayNote(uint16_t note, uint32_t duration_us, T_Volume volume)
{
  if (!SoundIsInProgress)
  {
    Sound_InitSine(volume);

    SoundIsInProgress = true;

    //Sound_Record();

    if (note == 1)
    {
      TimerHw_StartTimer(80, duration_us);
    }
  }
}

//_____________________________________________________________________________



//_____________________________________________________________________________

void Sound_Record(void)
{
  int i2s_read_len = I2S_READ_LEN;
  //int flash_wr_size = 0;
  size_t bytes_read;

  char* i2s_read_buff = (char*) calloc(i2s_read_len, sizeof(char));
  uint8_t* flash_write_buff = (uint8_t*) calloc(i2s_read_len, sizeof(char));

  i2s_adc_enable(I2S_NUM);

  //while (flash_wr_size < FLASH_RECORD_SIZE)
  {
    //read data from I2S bus, in this case, from ADC.
    i2s_read(I2S_NUM, (void*) i2s_read_buff, i2s_read_len, &bytes_read, portMAX_DELAY);

    ESP_LOGI(Tag, "COUCOU");
    //example_disp_buf((uint8_t*) i2s_read_buff, 64);
    //save original data from I2S(ADC) into flash.
    //esp_partition_write(data_partition, flash_wr_size, i2s_read_buff, i2s_read_len);
    //flash_wr_size += i2s_read_len;
    //ets_printf("Sound recording %u%%\n", flash_wr_size * 100 / FLASH_RECORD_SIZE);
  }

  i2s_adc_disable(I2S_NUM);
  free(i2s_read_buff);
  i2s_read_buff = NULL;
  free(flash_write_buff);
  flash_write_buff = NULL;
}

//_____________________________________________________________________________

static void SetI2SClock(void)
{
  i2s_set_clk(I2S_NUM, 16000, I2S_SAMPLE_BITS, 1);
}

//_____________________________________________________________________________

static void ResetI2SClock(void)
{
  i2s_set_clk(I2S_NUM, I2S_SAMPLE_RATE, I2S_SAMPLE_BITS, I2S_CHANNEL_NUM);
}

//_____________________________________________________________________________

static uint32_t ScaleADCToDAC(uint8_t* dest, uint8_t* source, uint32_t length)
{
  uint32_t j = 0u;

#if (I2S_SAMPLE_BITS == 16)
  for (uint32_t i = 0u; i < length; i++)
  {
    dest[j++] = 0u;
    dest[j++] = source[i];
  }

  return (length * 2);
#else
  for (uint32_t i = 0u; i < length; i++)
  {
    dest[j++] = 0u;
    dest[j++] = 0u;
    dest[j++] = 0u;
    dest[j++] = source[i];
  }

  return (length * 4);
#endif
}

//_____________________________________________________________________________
#if 0
void periodic_timer_callback(void* arg)
{
  static uint16_t pos = 0u;

  Gpio_TogglePinLevel(IR_PULSE_FRONT_PIN);
  dac_output_voltage(DAC_CHANNEL_1, (uint8_t)SineTable[pos]);
  pos++;
  if (pos == SINE_TABLE_SIZE)
  {
	pos = 0u;
  }
  // Select the value
  // Send the value to DAC
}
#endif
//_____________________________________________________________________________

void oneshot_timer_callback(void* arg)
{
    esp_timer_handle_t periodic_timer_handle = (esp_timer_handle_t) arg;
    /* To start the timer which is running, need to stop it first */
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer_handle));
    //ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_handle, 1000000));

    SoundIsInProgress = false;
}
