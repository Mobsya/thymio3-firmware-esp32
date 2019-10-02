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
#include "freertos/timers.h"

#include "esp_log.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
//#include "audio_hal.h"

#include "i2s_stream.h"

#include "mp3_decoder.h"

#include "filter_resample.h"

#include "wav_encoder.h"
#include "wav_decoder.h"

#include "esp_peripherals.h"
#include "spiffs_stream.h"
#include "periph_spiffs.h"

#include "driver/ledc.h"

#include "codec.h"

#include "board.h"
#include "es8374.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MCLK_FREQUENCY_Hz    20000000

#define RECORD_TIME_SECONDS        10

#define MP3_PLAYER_RATE         48000
#define MP3_PLAYER_CHANNEL          1  //!< Mono = 1
#define MP3_PLAYER_BITS            16

#define RECORD_RATE             48000
#define RECORD_CHANNEL              1  //!< Mono = 1
#define RECORD_BITS                16

#define WAV_PLAYER_RATE         48000
#define WAV_PLAYER_CHANNEL          1  //!< Mono = 1
#define WAV_PLAYER_BITS            16

#define SAVE_FILE_RATE           8000
#define SAVE_FILE_CHANNEL           1  //!< Mono = 1
#define SAVE_FILE_BITS             16

#define DEFAULT_ESP_PERIPH_STACK_SIZE      (4*1024)
#define DEFAULT_ESP_PERIPH_TASK_PRIO       (5)
#define DEFAULT_ESP_PERIPH_TASK_CORE       (0)

#define DEFAULT_ESP_PERIPH_SET_CONFIG() {\
    .task_stack         = DEFAULT_ESP_PERIPH_STACK_SIZE,   \
    .task_prio          = DEFAULT_ESP_PERIPH_TASK_PRIO,    \
    .task_core          = DEFAULT_ESP_PERIPH_TASK_CORE,    \
}

#define AUDIO_HAL_ES8374_DEFAULT(){                     \
        .adc_input  = AUDIO_HAL_ADC_INPUT_LINE2,        \
        .dac_output = AUDIO_HAL_DAC_OUTPUT_LINE1,       \
        .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH,        \
        .i2s_iface = {                                  \
            .mode = AUDIO_HAL_MODE_SLAVE,               \
            .fmt = AUDIO_HAL_I2S_NORMAL,                \
            .samples = AUDIO_HAL_48K_SAMPLES,           \
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,        \
        },                                              \
};

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
extern const uint8_t adf_music_mp3_end[]   asm("_binary_adf_music_mp3_end");

extern const uint8_t chicken_mp3_start[]   asm("_binary_chicken_mp3_start");
extern const uint8_t chicken_mp3_end[]     asm("_binary_chicken_mp3_end");

extern const uint8_t harry_mp3_start[]     asm("_binary_harry_mp3_start");
extern const uint8_t harry_mp3_end[]       asm("_binary_harry_mp3_end");

extern const uint8_t blop_mp3_start[]     asm("_binary_blop_mp3_start");
extern const uint8_t blop_mp3_end[]       asm("_binary_blop_mp3_end");

extern const uint8_t tick_mp3_start[]     asm("_binary_tick_mp3_start");
extern const uint8_t tick_mp3_end[]       asm("_binary_tick_mp3_end");

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "codec";

static TaskHandle_t MP3PlayerTask = NULL;
static TaskHandle_t WAVRecorderTask = NULL;
static TaskHandle_t WAVPlayerTask = NULL;

static audio_pipeline_handle_t MP3PlayerPipeline;
static audio_element_handle_t MP3PlayerI2SStream;
static audio_element_handle_t mp3_decoder;
static audio_event_iface_handle_t MP3PlayerEvt;

static audio_pipeline_handle_t WAVRecorderPipeline;
static audio_element_handle_t WAVRecorderI2SStream;
static audio_element_handle_t WAVRecorderFilter;
static audio_element_handle_t WAVEncoder;
static audio_element_handle_t WAVRecorderSPIFFSStream;
static audio_event_iface_handle_t WAVRecorderEvt;

static audio_pipeline_handle_t WAVPlayerPipeline;
static audio_element_handle_t WAVPlayerI2SStream;
static audio_element_handle_t WAVPlayerFilter;
static audio_element_handle_t WAVDecoder;
static audio_element_handle_t WAVPlayerSPIFFSStream;
static audio_event_iface_handle_t WAVPlayerEvt;

static bool MP3PlayerIsBusy   = false;
static bool WAVRecorderIsBusy = false;
static bool WAVPlayerIsBusy   = false;

static T_File File;

static int16_t Number = -1;

static esp_periph_set_handle_t Set;

audio_hal_func_t AUDIO_CODEC_ES8374_DEFAULT_HANDLE =
{
  .audio_codec_initialize = ES8374_Init,
  .audio_codec_deinitialize = ES8374_Deinit,
  .audio_codec_ctrl = ES8374_ControlState,
  .audio_codec_config_iface = ES8374_ConfigureI2S,
  .audio_codec_set_volume = ES8374_SetVoiceVolume,
  .audio_codec_get_volume = ES8374_GetVoiceVolume
};

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static void InitSPIFFS(void);

static audio_element_handle_t CreateSPIFFSStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

static audio_element_handle_t CreateI2SStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

static audio_element_handle_t CreateFilter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type);

static audio_element_handle_t CreateWAVEncoder(void);

static audio_element_handle_t CreateWAVDecoder(void);

static void RunMP3PlayerTask(void* arg);

static void RunWAVRecorderTask(void* arg);

static void RunWAVPlayerTask(void* arg);

static void StopMP3Player(void);

static void StopWAVRecorder(void);

static void StopWAVPlayer(void);

static void GenerateMasterClock(void);

static void SelectFile(int16_t index);

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Codec_Init(void)
{
  //GenerateMasterClock();

  InitSPIFFS();

#if 0  // Old
//  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2;
  //audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t hal = audio_hal_init(&audio_hal_codec_cfg, 1);  // 1 is selected to use the ES8374 codec
  audio_hal_ctrl_codec(hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
#endif

//#if 0
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t codec_hal = audio_hal_init(&audio_hal_codec_cfg, &AUDIO_CODEC_ES8374_DEFAULT_HANDLE);
  audio_hal_ctrl_codec(codec_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
//#endif
}

//_____________________________________________________________________________

void Codec_StartMP3Player(int number)
{
  Number = number;
#if 0
  if (!MP3PlayerIsBusy)
  {
    MP3PlayerIsBusy = true;

    xTaskCreatePinnedToCore(
      RunMP3PlayerTask,  // Function to implement the task
      "player",          // Name of the task
      4096,              // Stack size in words
      NULL,              // Task input parameter
      0,                 // Priority of the task
      &MP3PlayerTask,    // Task handle
      1);                // Core where the task should run
  }
#endif

//#if 0
  if (MP3PlayerIsBusy)
  {
    StopMP3Player();
    //audio_pipeline_stop(MP3PlayerPipeline);
    //audio_pipeline_wait_for_stop(MP3PlayerPipeline);
    //MP3PlayerIsBusy = false;
  }

  MP3PlayerIsBusy = true;

  //vTaskDelete(TxProxIRTask);

  xTaskCreatePinnedToCore(
    RunMP3PlayerTask,  // Function to implement the task
    "player",          // Name of the task
    4096,              // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    &MP3PlayerTask,    // Task handle
    0);                // Core where the task should run
//#endif
}

//_____________________________________________________________________________

void Codec_StartWAVRecorder(int number)
{
  // FIXME Number = number;

  if (WAVRecorderIsBusy)
  {
    StopWAVRecorder();
  }

  WAVRecorderIsBusy = true;

  xTaskCreatePinnedToCore(
    RunWAVRecorderTask,  // Function to implement the task
    "recorder",          // Name of the task
    4096,                // Stack size in words
    NULL,                // Task input parameter
    1,                   // Priority of the task
    &WAVRecorderTask,       // Task handle
    0);                  // Core where the task should run
}

//_____________________________________________________________________________

void Codec_StartWAVPlayer(int number)
{
  Number = number;

  if (WAVPlayerIsBusy)
  {
    StopWAVPlayer();
  }

  WAVPlayerIsBusy = true;

  xTaskCreatePinnedToCore(
    RunWAVPlayerTask,  // Function to implement the task
    "player",          // Name of the task
    4096,              // Stack size in words
    NULL,              // Task input parameter
    1,                 // Priority of the task
    &WAVPlayerTask,    // Task handle
    0);                // Core where the task should run
}

//_____________________________________________________________________________

static void InitSPIFFS(void)
{
  // Initialize peripherals management
  esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
  Set = esp_periph_set_init(&periph_cfg);

  // Initialize Spiffs peripheral
  periph_spiffs_cfg_t spiffs_cfg =
  {
    .root = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  esp_periph_handle_t spiffs_handle = periph_spiffs_init(&spiffs_cfg);

  // Start spiffs peripheral
  esp_periph_start(Set, spiffs_handle);

  // Wait until spiffs was mounted
  while (!periph_spiffs_is_mounted(spiffs_handle))
  {
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
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

//_____________________________________________________________________________

static audio_element_handle_t CreateI2SStream(int sample_rates, int bits, int channels, audio_stream_type_t type)
{
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = type;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;
  //i2s_cfg.i2s_pin_config.bck_io_num = I2S_SCLK_PIN;
  //i2s_cfg.i2s_pin_config.ws_io_num  = I2S_LCLK_PIN;
  //i2s_cfg.i2s_pin_config.data_out_num = I2S_DSIN_PIN;
  //i2s_cfg.i2s_pin_config.data_in_num = I2S_DOUT_PIN;
  i2s_cfg.i2s_config.sample_rate = sample_rates;

  audio_element_handle_t i2s_stream = i2s_stream_init(&i2s_cfg);
  mem_assert(i2s_stream);

  audio_element_info_t i2s_info = {0};
  audio_element_getinfo(i2s_stream, &i2s_info);
  i2s_info.bits = bits;
  i2s_info.channels = channels;
  i2s_info.sample_rates = sample_rates;
  audio_element_setinfo(i2s_stream, &i2s_info);

  return i2s_stream;
}

//_____________________________________________________________________________

static audio_element_handle_t CreateFilter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type)
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

static audio_element_handle_t CreateWAVEncoder(void)
{
  wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();

  return wav_encoder_init(&wav_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateWAVDecoder(void)
{
  wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();

  return wav_decoder_init(&wav_cfg);
}

//_____________________________________________________________________________

static void RunMP3PlayerTask(void* arg)
{
  SelectFile(Number);

#if 0
//  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t codec_hal = audio_hal_init(&audio_hal_codec_cfg, &AUDIO_CODEC_ES8374_DEFAULT_HANDLE);
  audio_hal_ctrl_codec(codec_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
#endif

//  ESP_LOGI(Tag, "[ 2 ] Create audio pipeline, add all elements to pipeline, and subscribe pipeline event");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  MP3PlayerPipeline = audio_pipeline_init(&pipeline_cfg);
  mem_assert(MP3PlayerPipeline);

//  ESP_LOGI(Tag, "[2.1] Create mp3 decoder to decode mp3 file and set custom read callback");
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
  mp3_decoder = mp3_decoder_init(&mp3_cfg);
  audio_element_set_read_cb(mp3_decoder, mp3_music_read_cb, NULL);

//  ESP_LOGI(Tag, "[2.2] Create i2s stream to write data to codec chip");
  MP3PlayerI2SStream = CreateI2SStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

//  ESP_LOGI(Tag, "[2.3] Register all elements to audio pipeline");
  audio_pipeline_register(MP3PlayerPipeline, mp3_decoder, "mp3");
  audio_pipeline_register(MP3PlayerPipeline, MP3PlayerI2SStream, "i2s");

//  ESP_LOGI(Tag, "[2.4] Link it together [mp3_music_read_cb]-->mp3_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(MP3PlayerPipeline, (const char* []){"mp3", "i2s"}, 2);

//  ESP_LOGI(Tag, "[ 3 ] Set up event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  MP3PlayerEvt = audio_event_iface_init(&evt_cfg);

//  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(MP3PlayerPipeline, MP3PlayerEvt);

//  ESP_LOGI(Tag, "[ 4 ] Start audio_pipeline");
  audio_pipeline_run(MP3PlayerPipeline);

  while (1)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(MP3PlayerEvt, &msg, portMAX_DELAY);

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

//      ESP_LOGI(Tag, "[ * ] Receive music info from mp3 decoder, sample_rates=%d, bits=%d, ch=%d",
//               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(MP3PlayerI2SStream, &music_info);
      i2s_stream_set_clk(MP3PlayerI2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }

    // Stop when the last pipeline element (MP3PlayerI2SStream in this case) receives stop event
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*) MP3PlayerI2SStream &&
        msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int)msg.data == AEL_STATUS_STATE_STOPPED) ||
        ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
      //ESP_LOGW(Tag, "[ * ] Stop event received");
      break;
    }
  }

//  ESP_LOGI(Tag, "[ 5 ] Stop audio_pipeline");
  StopMP3Player();
}

//_____________________________________________________________________________

static void RunWAVRecorderTask(void* arg)
{
#if 0
  //  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t codec_hal = audio_hal_init(&audio_hal_codec_cfg, &AUDIO_CODEC_ES8374_DEFAULT_HANDLE);
  audio_hal_ctrl_codec(codec_hal, AUDIO_HAL_CODEC_MODE_ENCODE, AUDIO_HAL_CTRL_START);
#endif

  ESP_LOGI(Tag, "[1.1] Create audio pipeline for recording");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  WAVRecorderPipeline = audio_pipeline_init(&pipeline_cfg);
  //mem_assert(WAVRecorderPipeline);

  ESP_LOGI(Tag, "[1.2] Create I2S stream to read audio data from codec chip");
  WAVRecorderI2SStream = CreateI2SStream(RECORD_RATE, RECORD_BITS, RECORD_CHANNEL, AUDIO_STREAM_READER);

  ESP_LOGI(Tag, "[1.3] Create filter to convert to 8 [kHz]");
  WAVRecorderFilter = CreateFilter(RECORD_RATE, RECORD_CHANNEL, SAVE_FILE_RATE, SAVE_FILE_CHANNEL, AUDIO_CODEC_TYPE_ENCODER);

  ESP_LOGI(Tag, "[1.4] Create WAV encoder to encode WAV format");
  WAVEncoder = CreateWAVEncoder();

  ESP_LOGI(Tag, "[1.5] Create SPIFFS stream to write data to spi flash");
  WAVRecorderSPIFFSStream = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[1.6] Register all elements to audio pipeline");
  audio_pipeline_register(WAVRecorderPipeline, WAVRecorderI2SStream, "i2s_reader");
  audio_pipeline_register(WAVRecorderPipeline, WAVRecorderFilter, "filter_downsample");
  audio_pipeline_register(WAVRecorderPipeline, WAVEncoder, "wav_encoder");
  audio_pipeline_register(WAVRecorderPipeline, WAVRecorderSPIFFSStream, "file_writer");

  ESP_LOGI(Tag, "[2] Set up  event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  WAVRecorderEvt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[2.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), WAVRecorderEvt);

  ESP_LOGI(Tag, "[2.2] Link it together [codec_chip]-->i2s_stream-->filter-->wav_encoder-->spiffs_stream-->[flash]");
  audio_pipeline_link(WAVRecorderPipeline, (const char *[]) {"i2s_reader", "filter_downsample", "wav_encoder", "file_writer"}, 4);

  i2s_stream_set_clk(WAVRecorderI2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);

  ESP_LOGI(Tag, "[2.3] Setup uri (file_writer as spiffs_stream, wav_encoder as wav encoder)");
  audio_element_set_uri(WAVRecorderSPIFFSStream, "/spiffs/rec.wav");

  //ESP_LOGI(Tag, "[4.1] Listening event from pipeline");
  //audio_pipeline_set_listener(WAVRecorderPipeline, WAVRecorderEvt);

  ESP_LOGI(Tag, "[ 3 ] Start audio_pipeline");
  audio_pipeline_run(WAVRecorderPipeline);

  ESP_LOGI(Tag, "[ 6 ] Listen for all pipeline events, record for %d Seconds", RECORD_TIME_SECONDS);
  int second_recorded = 0;

  while (1)
  {
    audio_event_iface_msg_t msg;

    if (audio_event_iface_listen(WAVRecorderEvt, &msg, (1000 / portTICK_RATE_MS)) != ESP_OK)
    {
      second_recorded ++;

      ESP_LOGI(Tag, "[ * ] Recording ... %d", second_recorded);

      if (second_recorded >= RECORD_TIME_SECONDS)
      {
        break;
      }

      continue;
    }
#if 0
    /* Stop when the last pipeline element (WAVRecorderI2SStream in this case) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) WAVRecorderI2SStream &&
        msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int) msg.data == AEL_STATUS_STATE_STOPPED) ||
        ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
      ESP_LOGW(Tag, "[ * ] Stop event received");
      break;
    }
#endif
  }

  ESP_LOGI(Tag, "[ 7 ] Stop audio_pipeline");
  StopWAVRecorder();
}

//_____________________________________________________________________________

static void RunWAVPlayerTask(void* arg)
{
#if 0
  //  ESP_LOGI(Tag, "[ 1 ] Start audio codec chip");
  audio_hal_codec_config_t audio_hal_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_codec_cfg.i2s_iface.samples = AUDIO_HAL_44K_SAMPLES;
  audio_hal_handle_t codec_hal = audio_hal_init(&audio_hal_codec_cfg, &AUDIO_CODEC_ES8374_DEFAULT_HANDLE);
  audio_hal_ctrl_codec(codec_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
#endif

  ESP_LOGI(Tag, "[1.1] Create audio pipeline for replay");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  WAVPlayerPipeline = audio_pipeline_init(&pipeline_cfg);
  mem_assert(WAVPlayerPipeline);

  ESP_LOGI(Tag, "[1.2] Create spiffs stream to read data from spi flash");
  WAVPlayerSPIFFSStream = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_READER);

  ESP_LOGI(Tag, "[1.3] Create WAV decoder to decode WAV format");
  WAVDecoder = CreateWAVDecoder();

  ESP_LOGI(Tag, "[1.4] Create filter to convert to 48 [kHz]");
  WAVPlayerFilter = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, WAV_PLAYER_RATE, WAV_PLAYER_CHANNEL, AUDIO_CODEC_TYPE_DECODER);

  ESP_LOGI(Tag, "[1.5] Create i2s stream to write audio data to codec chip");
  WAVPlayerI2SStream = CreateI2SStream(WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[1.6] Register all elements to audio pipeline");
  audio_pipeline_register(WAVPlayerPipeline, WAVPlayerSPIFFSStream, "file_reader");
  audio_pipeline_register(WAVPlayerPipeline, WAVDecoder, "wav_decoder");
  audio_pipeline_register(WAVPlayerPipeline, WAVPlayerFilter, "filter_upsample");
  audio_pipeline_register(WAVPlayerPipeline, WAVPlayerI2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  WAVPlayerEvt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[2.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), WAVPlayerEvt);

  ESP_LOGI(Tag, "[2.2] Link it together [flash]-->spiffs_stream-->wav_decoder-->filter-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(WAVPlayerPipeline, (const char *[]) {"file_reader", "wav_decoder", "filter_upsample", "i2s_writer"}, 4);

  i2s_stream_set_clk(WAVPlayerI2SStream, WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL);

  ESP_LOGI(Tag, "[2.3] Setup uri (file_reader as spiffs_stream, wav_decoder as wav decoder)");
  audio_element_set_uri(WAVPlayerSPIFFSStream, "/spiffs/rec.wav");

  //ESP_LOGI(Tag, "[4.1] Listening event from pipeline");
  audio_pipeline_set_listener(WAVPlayerPipeline, WAVPlayerEvt);

  ESP_LOGI(Tag, "[3] Start audio_pipeline");
  audio_pipeline_run(WAVPlayerPipeline);

  while (1)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(WAVPlayerEvt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }
#if 0
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*)WAVDecoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      ESP_LOGI(Tag, "[ 5.1 ]");
      audio_element_info_t music_info = {0};
      ESP_LOGI(Tag, "[ 5.2 ]");
      audio_element_getinfo(WAVDecoder, &music_info);
      ESP_LOGI(Tag, "[ 5.3 ]");

      ESP_LOGI(Tag, "[ * ] Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(WAVPlayerI2SStream, &music_info);
      i2s_stream_set_clk(WAVPlayerI2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }
#endif

    /* Stop when the last pipeline element (WAVPlayerI2SStream in this case) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) WAVPlayerI2SStream &&
        msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int) msg.data == AEL_STATUS_STATE_STOPPED) ||
        ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
      StopWAVPlayer();
      break;
    }

  }

  //ESP_LOGI(Tag, "[ 6 ] Stop audio_pipeline");
  //StopWAVPlayer();
}

//_____________________________________________________________________________

static void StopMP3Player(void)
{
  audio_pipeline_terminate(MP3PlayerPipeline);

  audio_pipeline_unregister(MP3PlayerPipeline, mp3_decoder);
  audio_pipeline_unregister(MP3PlayerPipeline, MP3PlayerI2SStream);

  // Terminate the pipeline before removing the listener
  audio_pipeline_remove_listener(MP3PlayerPipeline);

  // Make sure audio_pipeline_remove_listener is called before destroying event_iface
  audio_event_iface_destroy(MP3PlayerEvt);

  audio_pipeline_unregister(MP3PlayerPipeline, MP3PlayerI2SStream);
  audio_pipeline_unregister(MP3PlayerPipeline, mp3_decoder);

  // Release all resources
  audio_pipeline_deinit(MP3PlayerPipeline);
  audio_element_deinit(MP3PlayerI2SStream);
  audio_element_deinit(mp3_decoder);

  MP3PlayerIsBusy = false;
  //portYIELD();
  vTaskDelete(MP3PlayerTask);
}

//_____________________________________________________________________________

static void StopWAVRecorder(void)
{
  audio_pipeline_terminate(WAVRecorderPipeline);

  // Terminal the pipeline before removing the listener
  audio_pipeline_remove_listener(WAVRecorderPipeline);

  // Stop all periph before removing the listener
  esp_periph_set_stop_all(Set);
  audio_event_iface_remove_listener(esp_periph_set_get_event_iface(Set), WAVRecorderEvt);

  // Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface
  audio_event_iface_destroy(WAVRecorderEvt);

  // Release all resources
  audio_pipeline_unregister(WAVRecorderPipeline, WAVRecorderI2SStream);
  audio_pipeline_unregister(WAVRecorderPipeline, WAVRecorderFilter);
  audio_pipeline_unregister(WAVRecorderPipeline, WAVEncoder);
  audio_pipeline_unregister(WAVRecorderPipeline, WAVRecorderSPIFFSStream);

  //audio_pipeline_deinit(WAVRecorderPipeline);

  audio_element_deinit(WAVRecorderI2SStream);
  audio_element_deinit(WAVRecorderFilter);
  audio_element_deinit(WAVEncoder);
  audio_element_deinit(WAVRecorderSPIFFSStream);

  //esp_periph_set_destroy(Set);

  WAVRecorderIsBusy = false;

  Codec_StartWAVPlayer(0);

  vTaskDelete(WAVRecorderTask);
}

//_____________________________________________________________________________

static void StopWAVPlayer(void)
{
  audio_pipeline_terminate(WAVPlayerPipeline);

  // Terminal the pipeline before removing the listener
  audio_pipeline_remove_listener(WAVPlayerPipeline);

  // Stop all periph before removing the listener
  esp_periph_set_stop_all(Set);
  audio_event_iface_remove_listener(esp_periph_set_get_event_iface(Set), WAVPlayerEvt);

  // Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface
  audio_event_iface_destroy(WAVPlayerEvt);

  // Release all resources
  audio_pipeline_unregister(WAVPlayerPipeline, WAVPlayerSPIFFSStream);
  audio_pipeline_unregister(WAVPlayerPipeline, WAVDecoder);
  audio_pipeline_unregister(WAVPlayerPipeline, WAVPlayerFilter);
  audio_pipeline_unregister(WAVPlayerPipeline, WAVPlayerI2SStream);

  //audio_pipeline_deinit(WAVPlayerPipeline);
  audio_element_deinit(WAVPlayerSPIFFSStream);
  audio_element_deinit(WAVDecoder);
  audio_element_deinit(WAVPlayerFilter);
  audio_element_deinit(WAVPlayerI2SStream);

  //esp_periph_set_destroy(Set);

  WAVPlayerIsBusy = false;
  vTaskDelete(WAVPlayerTask);
}

//_____________________________________________________________________________

static void GenerateMasterClock(void)
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

static void SelectFile(int16_t index)
{
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
      File.Start = harry_mp3_start;
      File.End   = harry_mp3_end;
      break;

    case 3:
      File.Start = blop_mp3_start;
      File.End   = blop_mp3_end;
      break;

    case 4:
      File.Start = tick_mp3_start;
      File.End   = tick_mp3_end;
      break;

    default:
      ESP_LOGW(Tag, "Not supported index = %d", index);
      break;
  }

  File.Position = 0;
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
