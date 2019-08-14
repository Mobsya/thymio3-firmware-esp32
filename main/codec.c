//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    codec.c
//! \brief   This module provides the useful functions to use the audio codec
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "esp_log.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "audio_hal.h"

#include "driver/ledc.h"

#include "codec.h"

#include "board.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MCLK_FREQUENCY_Hz   20000000

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int Position;
  const uint8_t* Start;
  const uint8_t* End;
} T_File;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern const uint8_t adf_music_mp3_start[] asm("_binary_adf_music_mp3_start");
extern const uint8_t adf_music_mp3_end[] asm("_binary_adf_music_mp3_end");

extern const uint8_t chicken_mp3_start[] asm("_binary_chicken_mp3_start");
extern const uint8_t chicken_mp3_end[] asm("_binary_chicken_mp3_end");

extern const uint8_t dixie_horn_mp3_start[] asm("_binary_dixie_horn_mp3_start");
extern const uint8_t dixie_horn_mp3_end[] asm("_binary_dixie_horn_mp3_end");

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "codec";

static TaskHandle_t PlayerTask = NULL;

static audio_pipeline_handle_t pipeline;
static audio_element_handle_t i2s_stream_writer;
static audio_element_handle_t mp3_decoder;
static audio_event_iface_handle_t evt;

static T_File File;

static int16_t Number = -1;

static bool PlayerIsBusy = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunMP3PlayerTask(void* arg);

static void SelectFile(int16_t index);

static void StopMP3Player(void);

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Codec_Init(void)
{
  ledc_timer_config_t ledc_timer =
  {
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_num  = LEDC_TIMER_0,
	.bit_num    = 2,
	.freq_hz    = MCLK_FREQUENCY_Hz
  };

  ledc_timer_config(&ledc_timer);

  ledc_channel_config_t ledc_channel =
  {
    .channel    = LEDC_CHANNEL_0,
    .gpio_num   = I2S_MCLK_PIN,
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_sel  = LEDC_TIMER_0,
    .duty       = 2
  };

  ledc_channel_config(&ledc_channel);
}

//_____________________________________________________________________________

void Codec_StartMP3Player(int number)
{
  Number = number;

  if (PlayerIsBusy)
  {
    StopMP3Player();
  }

  PlayerIsBusy = true;

  //vTaskDelete(TxProxIRTask);

  xTaskCreatePinnedToCore(
    RunMP3PlayerTask,  // Function to implement the task
    "player",          // Name of the task
    4096,              // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    &PlayerTask,       // Task handle
    1);                // Core where the task should run
}

//_____________________________________________________________________________

static void RunMP3PlayerTask(void* arg)
{
  SelectFile(Number);

  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 1);  // 1 is selected to use the ES8374 codec
  audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

  ESP_LOGI(Tag, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  pipeline = audio_pipeline_init(&pipeline_cfg);
  mem_assert(pipeline);

  ESP_LOGI(Tag, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_decoder = mp3_decoder_init(&mp3_cfg);
  audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);

  ESP_LOGI(Tag, "[2.2] Create i2s stream to write data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT; //I2S_CHANNEL_FMT_RIGHT_LEFT; //I2S_CHANNEL_FMT_ALL_RIGHT;
  i2s_cfg.i2s_pin_config.bck_io_num = I2S_SCLK_PIN;
  i2s_cfg.i2s_pin_config.ws_io_num  = I2S_LCLK_PIN;
  i2s_cfg.i2s_pin_config.data_out_num = I2S_DSIN_PIN;
  i2s_cfg.i2s_pin_config.data_in_num = I2S_DOUT_PIN;
  i2s_cfg.i2s_config.sample_rate = 48000;
  i2s_stream_writer = i2s_stream_init(&i2s_cfg);

  ESP_LOGI(Tag, "[2.3] Register all elements to audio pipeline");
  audio_pipeline_register(pipeline, mp3_decoder, "mp3");
  audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

  ESP_LOGI(Tag, "[2.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(pipeline, (const char* []){"mp3", "i2s"}, 2);

  ESP_LOGI(Tag, "[ 3 ] Set up  event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  //audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);
  evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(pipeline, evt);

  ESP_LOGI(Tag, "[ 4 ] Start audio_pipeline");
  audio_pipeline_run(pipeline);

  while (1)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(evt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*) mp3_decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(mp3_decoder, &music_info);

      ESP_LOGI(Tag, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(i2s_stream_writer, &music_info);
      i2s_stream_set_clk(i2s_stream_writer, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }

    /* Stop when the last pipeline element (i2s_stream_writer in this case) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*) i2s_stream_writer
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (int) msg.data == AEL_STATUS_STATE_STOPPED)
    {
      break;
    }
  }

  ESP_LOGI(Tag, "[ 4 ] Stop audio_pipeline");
  StopMP3Player();
}

//_____________________________________________________________________________

static void SelectFile(int16_t index)
{
  //uint8_t idx = *index;

  ESP_LOGE(Tag, "Index = %d", index);

  File.Position = 0;

  switch (index)
  {
    case 0:
      File.Start = adf_music_mp3_start;
      File.End   = adf_music_mp3_end;
      break;

    case 1:
      File.Start = chicken_mp3_start;
      File.End   = chicken_mp3_end;
      break;

    case 2:
      File.Start = dixie_horn_mp3_start;
      File.End   = dixie_horn_mp3_end;
      break;

    default:
      ESP_LOGW(Tag, "Not supported index = %d", index);
      break;
  }

  File.Position = 0;
}

//_____________________________________________________________________________

static void StopMP3Player(void)
{
  audio_pipeline_terminate(pipeline);
  audio_pipeline_unregister(pipeline, mp3_decoder);
  audio_pipeline_unregister(pipeline, i2s_stream_writer);

  // Terminate the pipeline before removing the listener
  audio_pipeline_remove_listener(pipeline);

  // Make sure audio_pipeline_remove_listener is called before destroying event_iface
  audio_event_iface_destroy(evt);

  // Release all resources
  audio_pipeline_unregister(pipeline, i2s_stream_writer);
  audio_pipeline_unregister(pipeline, mp3_decoder);
  audio_pipeline_deinit(pipeline);
  audio_element_deinit(i2s_stream_writer);
  audio_element_deinit(mp3_decoder);

  PlayerIsBusy = false;
  vTaskDelete(PlayerTask);
}

//_____________________________________________________________________________

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx)
{
  int read_size = File.End - File.Start - File.Position;

  if (read_size == 0)
  {
    return AEL_IO_DONE;
  }
  else if (len < read_size)
  {
    read_size = len;
  }

  memcpy(buf, File.Start + File.Position, read_size);
  File.Position += read_size;

  return read_size;
}
