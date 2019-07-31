//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    mp3.c
//! \brief   This module provides the useful functions to use the MP3
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

#include "esp_log.h"
//#include "esp_peripherals.h"

//#include "amrnb_encoder.h"
//#include "amr_decoder.h"
#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
//#include "filter_resample.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "audio_hal.h"
//#include "spiffs_stream.h"

#include "mp3.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#if 0
#define RECORD_RATE         48000
#define RECORD_CHANNEL          1  // Mono
#define RECORD_BITS            16

#define SAVE_FILE_RATE       8000
#define SAVE_FILE_CHANNEL       1  // Mono
#define SAVE_FILE_BITS         16

#define PLAYBACK_RATE       48000
#define PLAYBACK_CHANNEL        1
#define PLAYBACK_BITS          16

#define ADC_UNIT_NUM      ADC_UNIT_1
#define ADC_CHANNEL_NUM   ADC1_CHANNEL_0
#endif

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

static const char* Tag = "mp3";

static TaskHandle_t PlayerTask = NULL;
static TaskHandle_t RecorderTask = NULL;

static audio_pipeline_handle_t pipeline;
static audio_element_handle_t i2s_stream_writer;
static audio_element_handle_t mp3_decoder;
static audio_event_iface_handle_t evt;

static T_File File;

static int16_t Number = -1;

static bool PlayerIsBusy = false;

//static esp_periph_set_handle_t set;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void RunMP3PlayerTask(void* arg);

static void RunMP3RecorderTask(void* arg);

static void SelectFile(int16_t index);

static void StopMP3Player(void);

//static audio_element_handle_t CreateI2SStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

//static audio_element_handle_t CreateFilter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type);

//static audio_element_handle_t CreateAMREncoder(void);

//static audio_element_handle_t CreateSPIFFSStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void MP3_Init(void)
{
  ESP_LOGI(Tag, "MP3 is initialized");
}

//_____________________________________________________________________________

void MP3_StartPlayer(int number)
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
//#if 0
void MP3_StartRecorder(void)
{
  xTaskCreatePinnedToCore(
    RunMP3RecorderTask,  // Function to implement the task
    "mp3",               // Name of the task
    4096,                // Stack size in words
    NULL,                // Task input parameter
    1,                   // Priority of the task
    &RecorderTask,       // Task handle
    1);                  // Core where the task should run
}
//#endif
//_____________________________________________________________________________

static void RunMP3PlayerTask(void* arg)
{
  SelectFile(Number);

  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 0);
  audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

  ESP_LOGI(Tag, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  pipeline = audio_pipeline_init(&pipeline_cfg);
  mem_assert(pipeline);

  ESP_LOGI(Tag, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_decoder = mp3_decoder_init(&mp3_cfg);
  audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);

  ESP_LOGI(Tag, "[2.2] Create i2s stream to write data to ESP32 internal DAC");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;
  i2s_cfg.i2s_config.sample_rate = 48000;
  i2s_stream_writer = i2s_stream_init(&i2s_cfg);

  ESP_LOGI(Tag, "[2.3] Register all elements to audio pipeline");
  audio_pipeline_register(pipeline, mp3_decoder, "mp3");
  audio_pipeline_register(pipeline, i2s_stream_writer, "i2s");

  ESP_LOGI(Tag, "[2.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[ESP32 DAC]");
  audio_pipeline_link(pipeline, (const char* [])
  {"mp3", "i2s"
  }, 2);

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

static void RunMP3RecorderTask(void* arg)
{
#if 0
  audio_pipeline_handle_t pipeline_rec = NULL;
  //audio_pipeline_handle_t pipeline_play = NULL;
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();

  // Recorder
  ESP_LOGI(Tag, "[1.1] Initialize recorder pipeline");
  pipeline_rec = audio_pipeline_init(&pipeline_cfg);
  //pipeline_play = audio_pipeline_init(&pipeline_cfg);

  ESP_LOGI(Tag, "[1.2] Create audio elements for recorder pipeline");
  audio_element_handle_t i2s_reader = CreateI2SStream(RECORD_RATE, RECORD_BITS, RECORD_CHANNEL, AUDIO_STREAM_READER);
  audio_element_handle_t filter_downsample = CreateFilter(RECORD_RATE, RECORD_CHANNEL, SAVE_FILE_RATE, SAVE_FILE_CHANNEL,
      AUDIO_CODEC_TYPE_ENCODER);
  audio_element_handle_t amr_encoder = CreateAMREncoder();
  audio_element_handle_t spiffs_writer = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL,
                                         AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[1.3] Register all elements to record pipeline");
  audio_pipeline_register(pipeline_rec, i2s_reader, "i2s_reader");
  audio_pipeline_register(pipeline_rec, filter_downsample, "filter_downsample");
  audio_pipeline_register(pipeline_rec, amr_encoder, "amr_encoder");
  audio_pipeline_register(pipeline_rec, spiffs_writer, "file_writer");

  ESP_LOGI(Tag, "[1.4] Link it together [ESP32 ADC]-->i2s_stream-->filter-->amr_encoder-->spiffs_stream-->[flash]");
  audio_pipeline_link(pipeline_rec, (const char* [])
  {"i2s_reader", "filter_downsample", "amr_encoder", "file_writer"
  }, 4);

  ESP_LOGI(Tag, "Setup file path to save recorded audio");
  i2s_stream_set_clk(i2s_reader, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);
  audio_element_set_uri(spiffs_writer, "/spiffs/rec.amr");
  audio_pipeline_run(pipeline_rec);

  ESP_LOGI(Tag, "[ 3 ] Set up event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  audio_event_iface_handle_t evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "COUCOU");

  audio_event_iface_set_listener(esp_periph_set_get_event_iface(set), evt);

  audio_pipeline_run(pipeline_rec);

#if 0
  // Playback
  ESP_LOGI(Tag, "[2.2] Create audio elements for playback pipeline");
  audio_element_handle_t spiffs_reader = create_spiffs_stream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL,
                                         AUDIO_STREAM_READER);
  audio_element_handle_t amr_decoder = create_amr_decoder();
  audio_element_handle_t filter_upsample = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, PLAYBACK_RATE,
      PLAYBACK_CHANNEL, AUDIO_CODEC_TYPE_DECODER);
  audio_element_handle_t i2s_writer = CreateI2SStream(PLAYBACK_RATE, PLAYBACK_BITS, PLAYBACK_CHANNEL,
                                      AUDIO_STREAM_WRITER);

  ESP_LOGI(TAG, "[2.3] Register audio elements to playback pipeline");
  audio_pipeline_register(pipeline_play, spiffs_reader_el,     "file_reader");
  audio_pipeline_register(pipeline_play, amr_decoder_el,       "amr_decoder");
  audio_pipeline_register(pipeline_play, filter_upsample_el,   "filter_upsample");
  audio_pipeline_register(pipeline_play, i2s_writer_el, "i2s_writer");
#endif

  while (1)
  {

  }
#endif
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
  audio_pipeline_deinit(pipeline);
  audio_element_deinit(i2s_stream_writer);
  audio_element_deinit(mp3_decoder);

  //ProxIR_Start();

  PlayerIsBusy = false;
  vTaskDelete(PlayerTask);
}

//_____________________________________________________________________________
#if 0
static audio_element_handle_t CreateI2SStream(int sample_rates, int bits, int channels, audio_stream_type_t type)
{
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_INTERNAL_ADC_CFG_CUSTOM();
  i2s_cfg.type = type;
  audio_element_handle_t i2s_stream = i2s_stream_init(&i2s_cfg);
  mem_assert(i2s_stream);

  // Init ADC pad
  i2s_set_adc_mode(ADC_UNIT_NUM, ADC_CHANNEL_NUM);

  audio_element_info_t i2s_info = {0};
  audio_element_getinfo(i2s_stream, &i2s_info);
  i2s_info.bits = bits;
  i2s_info.channels = channels;
  i2s_info.sample_rates = sample_rates;
  audio_element_setinfo(i2s_stream, &i2s_info);

  return i2s_stream;
}

//_____________________________________________________________________________

static audio_element_handle_t CreateFilter(int source_rate, int source_channel, int dest_rate, int dest_channel,
    audio_codec_type_t type)
{
  rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
  rsp_cfg.src_rate = source_rate;
  rsp_cfg.src_ch = source_channel;
  rsp_cfg.dest_rate = dest_rate;
  rsp_cfg.dest_ch = dest_channel;
  rsp_cfg.type = type;

  return rsp_filter_init(&rsp_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateAMREncoder(void)
{
  amrnb_encoder_cfg_t amrnb_cfg = DEFAULT_AMRNB_ENCODER_CONFIG();

  return amrnb_encoder_init(&amrnb_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateSPIFFSStream(int sample_rates, int bits, int channels, audio_stream_type_t type)
{
  spiffs_stream_cfg_t spiffs_cfg = SPIFFS_STREAM_CFG_DEFAULT();
  spiffs_cfg.type = type;
  audio_element_handle_t spiffs_stream = spiffs_stream_init(&spiffs_cfg);
  mem_assert(spiffs_stream);
  audio_element_info_t writer_info = {0};
  audio_element_getinfo(spiffs_stream, &writer_info);
  writer_info.bits = bits;
  writer_info.channels = channels;
  writer_info.sample_rates = sample_rates;
  audio_element_setinfo(spiffs_stream, &writer_info);

  return spiffs_stream;
}
#endif
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
