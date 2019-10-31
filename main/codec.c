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
#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include <sys/stat.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include "freertos/portmacro.h"
//#include "freertos/timers.h"

#include "esp_log.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
//#include "audio_hal.h"

#include "i2s_stream.h"
#include "spiffs_stream.h"

#include "mp3_decoder.h"

#include "wav_encoder.h"
#include "wav_decoder.h"

#include "filter_resample.h"

#include "esp_peripherals.h"
#include "periph_spiffs.h"

#include "driver/ledc.h"

#include "codec.h"

#include "board.h"
#include "es8374.h"
#include "file_system.h"

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

#define WAV_PLAYER_RATE          8000
#define WAV_PLAYER_CHANNEL          1  //!< Mono = 1
#define WAV_PLAYER_BITS            16

#define SAVE_FILE_RATE           8000
#define SAVE_FILE_CHANNEL           1  //!< Mono = 1
#define SAVE_FILE_BITS             16

#define DEFAULT_PLAYER_TASK_STACK (4*1024)
#define DEFAULT_PLAYER_TASK_PRIO (5)

#define DEFAULT_RECORDER_TASK_STACK (4*1024)
#define DEFAULT_RECORDER_TASK_PRIO (5)

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

#define AUDIO_CODEC_DEFAULT_CONFIG(){                   \
        .adc_input  = AUDIO_HAL_ADC_INPUT_LINE1,        \
        .dac_output = AUDIO_HAL_DAC_OUTPUT_ALL,         \
        .codec_mode = AUDIO_HAL_CODEC_MODE_BOTH,        \
        .i2s_iface = {                                  \
            .mode = AUDIO_HAL_MODE_SLAVE,               \
            .fmt = AUDIO_HAL_I2S_NORMAL,                \
            .samples = AUDIO_HAL_48K_SAMPLES,           \
            .bits = AUDIO_HAL_BIT_LENGTH_16BITS,        \
        },                                              \
};

#define BUF_SIZE (SAVE_FILE_RATE * 1) /* 2 second buffer */

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  int Position;
  const uint8_t* Start;
  const uint8_t* End;
} T_File;

struct audio_board_handle
{
  audio_hal_handle_t audio_hal; /*!< audio hardware abstract layer handle */
};

typedef struct audio_board_handle* audio_board_handle_t;

typedef enum
{
  PLAYER_EVENT_NONE = 0,
  PLAYER_EVENT_PLAY,
  PLAYER_EVENT_STOP,
  PLAYER_EVENT_PAUSE,
  PLAYER_EVENT_RESUME,
} T_PlayerEvent;

typedef enum
{
  RECORDER_EVENT_NONE = 0,
  RECORDER_EVENT_RECORD,
  RECORDER_EVENT_STOP,
  RECORDER_EVENT_PAUSE,
  RECORDER_EVENT_RESUME,
} T_RecorderEvent;

typedef struct MP3AudioPlayerFromFlash* T_MP3PlayerFromFlashHandle;
typedef esp_err_t (*MP3PlayerFromFlashEvent)(T_MP3PlayerFromFlashHandle ap, T_PlayerEvent event);

typedef struct MP3AudioPlayer* T_MP3PlayerHandle;
typedef esp_err_t (*MP3PlayerEvent)(T_MP3PlayerHandle ap, T_PlayerEvent event);

typedef struct WAVAudioPlayer* T_WAVPlayerHandle;
typedef esp_err_t (*WAVPlayerEvent)(T_WAVPlayerHandle ap, T_PlayerEvent event);

typedef struct WAVAudioRecorder* T_WAVRecorderHandle;
typedef esp_err_t (*WAVRecorderEvent)(T_WAVRecorderHandle ap, T_RecorderEvent event);

typedef struct
{
  int task_stack;
  int task_prio;
  MP3PlayerFromFlashEvent event_handler;
} T_MP3PlayerFromFlashConfig;

typedef struct
{
  int task_stack;
  int task_prio;
  MP3PlayerEvent event_handler;
} T_MP3PlayerConfig;

typedef struct
{
  int task_stack;
  int task_prio;
  WAVPlayerEvent event_handler;
} T_WAVPlayerConfig;

typedef struct
{
  int task_stack;
  int task_prio;
  WAVRecorderEvent event_handler;
} T_WAVRecorderConfig;

typedef struct MP3AudioPlayerFromFlash
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t Decoder;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Playing;
  MP3PlayerFromFlashEvent EventHandler;
} T_MP3PlayerFromFlash;

typedef struct MP3AudioPlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t SPIFFSStream;
  audio_element_handle_t Decoder;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Playing;
  MP3PlayerEvent EventHandler;
} T_MP3Player;

typedef struct WAVAudioPlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t SPIFFSStream;
  audio_element_handle_t Decoder;
  audio_element_handle_t Filter;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Playing;
  WAVPlayerEvent EventHandler;
} T_WAVPlayer;

typedef struct WAVAudioRecorder
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t SPIFFSStream;
  audio_element_handle_t Encoder;
  audio_element_handle_t Filter;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Recording;
  WAVRecorderEvent EventHandler;
} T_WAVRecorder;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern const uint8_t adf_music_mp3_start[] asm("_binary_adf_music_mp3_start");
extern const uint8_t adf_music_mp3_end[]   asm("_binary_adf_music_mp3_end");

extern const uint8_t chicken_mp3_start[]   asm("_binary_chicken_mp3_start");
extern const uint8_t chicken_mp3_end[]     asm("_binary_chicken_mp3_end");

extern const uint8_t harry_mp3_start[]     asm("_binary_harry_mp3_start");
extern const uint8_t harry_mp3_end[]       asm("_binary_harry_mp3_end");

extern const uint8_t blop_mp3_start[]      asm("_binary_blop_mp3_start");
extern const uint8_t blop_mp3_end[]        asm("_binary_blop_mp3_end");

extern const uint8_t tick_mp3_start[]      asm("_binary_tick_mp3_start");
extern const uint8_t tick_mp3_end[]        asm("_binary_tick_mp3_end");

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "codec";

static TaskHandle_t MP3PlayerFromFlashTask = NULL;
static TaskHandle_t MP3PlayerTask          = NULL;
static TaskHandle_t WAVPlayerTask          = NULL;
static TaskHandle_t WAVRecorderTask        = NULL;

static T_File File;

static int16_t FileIndex = -1;
static uint16_t RecordingDuration_s = 0;

static esp_periph_set_handle_t Set;

audio_hal_func_t AUDIO_CODEC_ES8374_DEFAULT_HANDLE =
{
  .audio_codec_initialize   = ES8374_Init,
  .audio_codec_deinitialize = ES8374_Deinit,
  .audio_codec_ctrl         = ES8374_ControlState,
  .audio_codec_config_iface = ES8374_ConfigureI2S,
  .audio_codec_set_volume   = ES8374_SetVoiceVolume,
  .audio_codec_get_volume   = ES8374_GetVoiceVolume
};

static audio_board_handle_t board_handle = 0;

static T_MP3PlayerFromFlashHandle MP3PlayerFromFlash = NULL;
static T_MP3PlayerHandle          MP3Player          = NULL;
static T_WAVPlayerHandle          WAVPlayer          = NULL;
static T_WAVRecorderHandle        WAVRecorder        = NULL;

static int16_t buffer[4*BUF_SIZE];

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static audio_board_handle_t Init(void);

static audio_hal_handle_t InitCodec(void);

static void InitSPIFFS(void);

static T_MP3PlayerFromFlashHandle InitMP3PlayerFromFlash(T_MP3PlayerFromFlashConfig* config);

static T_MP3PlayerHandle InitMP3Player(T_MP3PlayerConfig* config);

static T_WAVPlayerHandle InitWAVPlayer(T_WAVPlayerConfig* config);

static T_WAVRecorderHandle InitWAVRecorder(T_WAVRecorderConfig* config);

static audio_element_handle_t CreateSPIFFSStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

static audio_element_handle_t CreateI2SStream(int sample_rates, int bits, int channels, audio_stream_type_t type);

static audio_element_handle_t CreateFilter(int source_rate, int source_channel, int dest_rate, int dest_channel, audio_codec_type_t type);

static audio_element_handle_t CreateWAVEncoder(void);

static audio_element_handle_t CreateMP3Decoder(void);

static audio_element_handle_t CreateWAVDecoder(void);

static void GenerateMasterClock(uint32_t clock_Hz);

static void SelectFile(int16_t index);

static void RunMP3PlayerFromFlashTask(void* arg);

static void RunMP3PlayerTask(void* arg);

static void RunWAVPlayerTask(void* arg);

static void RunWAVRecorderTask(void* arg);

static esp_err_t SendMP3PlayerFromFlashEvent(T_MP3PlayerFromFlashHandle player, T_PlayerEvent event);

static esp_err_t SendMP3PlayerEvent(T_MP3PlayerHandle player, T_PlayerEvent event);

static esp_err_t SendWAVPlayerEvent(T_WAVPlayerHandle player, T_PlayerEvent event);

static esp_err_t SendWAVRecorderEvent(T_WAVRecorderHandle recorder, T_RecorderEvent event);

static esp_err_t PlayMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

static esp_err_t PlayMP3(T_MP3PlayerHandle ap, const char* url);

static esp_err_t PlayWAV(T_WAVPlayerHandle ap, const char* url);

static esp_err_t PauseMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

static esp_err_t PauseMP3(T_MP3PlayerHandle ap);

static esp_err_t PauseWAV(T_WAVPlayerHandle ap);

static esp_err_t ResumeMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

static esp_err_t ResumeMP3(T_MP3PlayerHandle ap);

static esp_err_t ResumeWAV(T_WAVPlayerHandle ap);

static int GetMP3PlayedTime(T_MP3PlayerHandle ap);

static int GetWAVPlayedTime(T_WAVPlayerHandle ap);

static esp_err_t RecordWAV(T_WAVRecorderHandle ap, const char* url);

static esp_err_t StopMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

static esp_err_t StopMP3(T_MP3PlayerHandle ap);

static esp_err_t StopWAV(T_WAVPlayerHandle ap);

static esp_err_t StopWAVRecord(T_WAVRecorderHandle ap);

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx);

esp_err_t HandleMP3PlayerFromFlashEvent(T_MP3PlayerFromFlashHandle ap, T_PlayerEvent event);

esp_err_t HandleMP3PlayerEvent(T_MP3PlayerHandle ap, T_PlayerEvent event);

esp_err_t HandleWAVPlayerEvent(T_WAVPlayerHandle ap, T_PlayerEvent event);

esp_err_t HandleWAVRecorderEvent(T_WAVRecorderHandle ap, T_RecorderEvent event);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Codec_Init(void)
{
  GenerateMasterClock(MCLK_FREQUENCY_Hz);

  InitSPIFFS();

  //ESP_LOGI(Tag, "[2] Start codec chip");
  board_handle = Init();
  audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

//#if 0
  ESP_LOGE(Tag, "INIT MP3 PLAYER FROM FLASH");
  T_MP3PlayerFromFlashConfig MP3PlayerFromFlashConfig =
  {
    .event_handler = HandleMP3PlayerFromFlashEvent,
  };

  MP3PlayerFromFlash = InitMP3PlayerFromFlash(&MP3PlayerFromFlashConfig);
//#endif

//#if 0
  ESP_LOGE(Tag, "INIT MP3 PLAYER");
  T_MP3PlayerConfig MP3PlayerConfig =
  {
    .event_handler = HandleMP3PlayerEvent,
  };

  MP3Player = InitMP3Player(&MP3PlayerConfig);
//#endif

#if 0
  ESP_LOGE(Tag, "INIT WAV PLAYER");
  T_WAVPlayerConfig WAVPlayerConfig =
  {
    .event_handler = HandleWAVPlayerEvent,
  };

  WAVPlayer = InitWAVPlayer(&WAVPlayerConfig);
#endif

#if 0
  ESP_LOGE(Tag, "INIT WAV RECORDER");
  T_WAVRecorderConfig WAVRecorderConfig =
  {
    .event_handler = HandleWAVRecorderEvent,
  };

  WAVRecorder = InitWAVRecorder(&WAVRecorderConfig);
#endif
}

//_____________________________________________________________________________

void Codec_CreateWAVFile(int16_t index, int16_t freq_Hz)
{
  //float t;
  float amplitude = 2000;
  //float freq_Hz = 440;  //440;523.25;
  float phase = 0;
  char* fileName;

  float freq_radians_per_sample = ((freq_Hz * 2 * M_PI) / SAVE_FILE_RATE);

  // Fill buffer with a sine wave
  for (uint16_t i = 0u; i < BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  phase = 0;
  freq_radians_per_sample = ((554.37 * 2 * M_PI) / SAVE_FILE_RATE);

  for (uint16_t i = BUF_SIZE; i < 2*BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  phase = 0;
  freq_radians_per_sample = ((659.25 * 2 * M_PI) / SAVE_FILE_RATE);

  for (uint16_t i = 2*BUF_SIZE; i < 3*BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  phase = 0;
  freq_radians_per_sample = ((880.00 * 2 * M_PI) / SAVE_FILE_RATE);

  for (uint16_t i = 3*BUF_SIZE; i < 4*BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  FileSystem_SelectFile(&fileName, index, E_Extension_WAV);

  FileSystem_WriteWAVFile(fileName, 4 * BUF_SIZE, buffer, SAVE_FILE_RATE, WAV_PLAYER_CHANNEL);
}

//_____________________________________________________________________________

void Codec_PlayMP3FileFromFlash(int16_t index)
{
  SelectFile(index);
  //FileIndex = index;

  PlayMP3FromFlash(MP3PlayerFromFlash);
}

//_____________________________________________________________________________

void Codec_PlayMP3File(int16_t index)
{
  char* fileName;

  FileIndex = index;

  FileSystem_SelectFile(&fileName, FileIndex, E_Extension_MP3);

  // Check that the file exists
  if (FileSystem_DoesFileExist(fileName))
  {
    PlayMP3(MP3Player, fileName);
  }
}

//_____________________________________________________________________________

void Codec_PlayWAVFile(int16_t index)
{
  char* fileName;

  FileIndex = index;

  FileSystem_SelectFile(&fileName, FileIndex, E_Extension_WAV);

  // Check that the file exists
  if (FileSystem_DoesFileExist(fileName))
  {
    PlayWAV(WAVPlayer, fileName);
  }
}

//_____________________________________________________________________________

void Codec_PauseMP3FileFromFlash(void)
{
  PauseMP3FromFlash(MP3PlayerFromFlash);
}

//_____________________________________________________________________________

void Codec_PauseMP3File(void)
{
  PauseMP3(MP3Player);
}

//_____________________________________________________________________________

void Codec_PauseWAVFile(void)
{
  PauseWAV(WAVPlayer);
}

//_____________________________________________________________________________

void Codec_ResumeMP3FileFromFlash(void)
{
  ResumeMP3FromFlash(MP3PlayerFromFlash);
}

//_____________________________________________________________________________

void Codec_ResumeMP3File(void)
{
  ResumeMP3(MP3Player);
}

//_____________________________________________________________________________

void Codec_ResumeWAVFile(void)
{
  ResumeWAV(WAVPlayer);
}

//_____________________________________________________________________________

int Codec_GetMP3PlayedTime(void)
{
  return GetMP3PlayedTime(MP3Player);
}

//_____________________________________________________________________________

int Codec_GetWAVPlayedTime(void)
{
  return GetWAVPlayedTime(WAVPlayer);
}

//_____________________________________________________________________________

void Codec_RecordWAVFile(int16_t index, uint16_t duration_s)
{
  char* fileName;

  RecordingDuration_s = duration_s;

  FileIndex = index;

  FileSystem_SelectFile(&fileName, FileIndex, E_Extension_WAV);

  RecordWAV(WAVRecorder, fileName);
}

//_____________________________________________________________________________

void Codec_SetVolume(int16_t volume)
{
  ES8374_SetVoiceVolume(volume);
}

//_____________________________________________________________________________

static audio_board_handle_t Init(void)
{
  if (board_handle)
  {
    ESP_LOGW(Tag, "The board has already been initialized!");
    return board_handle;
  }

  board_handle = (audio_board_handle_t) audio_calloc(1, sizeof(struct audio_board_handle));
  AUDIO_MEM_CHECK(Tag, board_handle, return NULL);
  board_handle->audio_hal = InitCodec();

  return board_handle;
}

//_____________________________________________________________________________

static audio_hal_handle_t InitCodec(void)
{
  audio_hal_codec_config_t audio_codec_cfg = AUDIO_HAL_ES8374_DEFAULT();
  audio_hal_handle_t codec_hal = audio_hal_init(&audio_codec_cfg, &AUDIO_CODEC_ES8374_DEFAULT_HANDLE);
  AUDIO_NULL_CHECK(Tag, codec_hal, return NULL);
  return codec_hal;
}

//_____________________________________________________________________________

static void InitSPIFFS(void)
{
  // Initialize peripherals management
  esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
  Set = esp_periph_set_init(&periph_cfg);

  ESP_LOGI(Tag, "[1] Mount spiffs");
  // Initialize Spiffs peripheral
  periph_spiffs_cfg_t spiffs_cfg =
  {
    .root = "/spiffs",
    .partition_label = NULL,
    .max_files = 5,
    .format_if_mount_failed = true
  };

  esp_periph_handle_t spiffs_handle = periph_spiffs_init(&spiffs_cfg);

  // Start spiffs
  esp_periph_start(Set, spiffs_handle);

  // Wait until spiffs is mounted
  while (!periph_spiffs_is_mounted(spiffs_handle))
  {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  ESP_LOGI(Tag, "[1.1] SPIFFS is mounted");
}

//_____________________________________________________________________________

static T_MP3PlayerFromFlashHandle InitMP3PlayerFromFlash(T_MP3PlayerFromFlashConfig* config)
{
  T_MP3PlayerFromFlashHandle ap = calloc(1, sizeof(T_MP3PlayerFromFlash));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  //board_handle = Init();
  //audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
  ap->Hal = board_handle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.2] Create MP3 decoder to decode MP3 format");
  ap->Decoder = CreateMP3Decoder();
  AUDIO_MEM_CHECK(Tag, ap->Decoder, goto _audio_init_failed);
  audio_element_set_read_cb(ap->Decoder, mp3_music_read_cb, NULL);

  ESP_LOGI(Tag, "[2.4] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->Decoder,   "mp3_flash_decoder");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.6] Link it together [mp3_music_read_cb]-->mp3_flash_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]) {"mp3_flash_decoder", "i2s_writer"}, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;
  ap->EventHandler = config->event_handler;

  int task_stack = config->task_stack ? config->task_stack : DEFAULT_PLAYER_TASK_STACK;
  int task_prio = config->task_prio ? config->task_prio : DEFAULT_PLAYER_TASK_PRIO;

  //if (xTaskCreate(RunMP3PlayerFromFlashTask, "replayer", task_stack, ap, task_prio, NULL) != pdTRUE)
  if (xTaskCreatePinnedToCore(RunMP3PlayerFromFlashTask, "player", task_stack, ap, task_prio, &MP3PlayerFromFlashTask, 0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Player task");
    goto _audio_init_failed;
  }

  return ap;
_audio_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static T_MP3PlayerHandle InitMP3Player(T_MP3PlayerConfig* config)
{
  T_MP3PlayerHandle ap = calloc(1, sizeof(T_MP3Player));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  //board_handle = Init();
  //audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
  ap->Hal = board_handle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create spiffs stream to read data from spi flash");
  ap->SPIFFSStream = CreateSPIFFSStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_READER);
  AUDIO_MEM_CHECK(Tag, ap->SPIFFSStream, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.2] Create MP3 decoder to decode MP3 format");
  ap->Decoder = CreateMP3Decoder();
  AUDIO_MEM_CHECK(Tag, ap->Decoder, goto _audio_init_failed);

  //ESP_LOGI(Tag, "[2.3] Create filter to convert to 48 [kHz]");
  //ap->Filter = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, WAV_PLAYER_RATE, WAV_PLAYER_CHANNEL, AUDIO_CODEC_TYPE_DECODER);
  //AUDIO_MEM_CHECK(Tag, ap->Filter, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "mp3_file_reader");
  audio_pipeline_register(ap->Pipeline, ap->Decoder, "mp3_decoder");
  //audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_upsample");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  //ESP_LOGI(Tag, "[2.6] Link it together [flash]-->spiffs_stream-->mp3_decoder-->filter-->i2s_stream-->[codec_chip]");
  //audio_pipeline_link(ap->Pipeline, (const char *[]) {"mp3_file_reader", "mp3_decoder", "filter_upsample", "i2s_writer"}, 4);
  ESP_LOGI(Tag, "[2.6] Link it together [flash]-->spiffs_stream-->mp3_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]) {"mp3_file_reader", "mp3_decoder", "i2s_writer"}, 3);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;
  ap->EventHandler = config->event_handler;

  int task_stack = config->task_stack ? config->task_stack : DEFAULT_PLAYER_TASK_STACK;
  int task_prio = config->task_prio ? config->task_prio : DEFAULT_PLAYER_TASK_PRIO;

  //if (xTaskCreate(RunWAVPlayerTask, "replayer", task_stack, ap, task_prio, NULL) != pdTRUE)
  if (xTaskCreatePinnedToCore(RunMP3PlayerTask, "mp3_player", task_stack, ap, task_prio, &MP3PlayerTask, 0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Player task");
    goto _audio_init_failed;
  }

  return ap;
_audio_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static T_WAVPlayerHandle InitWAVPlayer(T_WAVPlayerConfig* config)
{
  T_WAVPlayerHandle ap = calloc(1, sizeof(T_WAVPlayer));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  //board_handle = Init();
  //audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
  ap->Hal = board_handle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create spiffs stream to read data from spi flash");
  ap->SPIFFSStream = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_READER);
  AUDIO_MEM_CHECK(Tag, ap->SPIFFSStream, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.2] Create WAV decoder to decode WAV format");
  ap->Decoder = CreateWAVDecoder();
  AUDIO_MEM_CHECK(Tag, ap->Decoder, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.3] Create filter to convert to 48 [kHz]");
  ap->Filter = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, WAV_PLAYER_RATE, WAV_PLAYER_CHANNEL, AUDIO_CODEC_TYPE_DECODER);
  AUDIO_MEM_CHECK(Tag, ap->Filter, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "file_reader");
  audio_pipeline_register(ap->Pipeline, ap->Decoder, "wav_decoder");
  audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_upsample");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.6] Link it together [flash]-->spiffs_stream-->wav_decoder-->filter-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]) {"file_reader", "wav_decoder", "filter_upsample", "i2s_writer"}, 4);
  //ESP_LOGI(Tag, "[2.6] Link it together [flash]-->spiffs_stream-->wav_decoder-->i2s_stream-->[codec_chip]");
  //audio_pipeline_link(ap->Pipeline, (const char *[]) {"file_reader", "wav_decoder", "i2s_writer"}, 3);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);  // Listen SetEvent from ap->Evt

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;
  ap->EventHandler = config->event_handler;

  int task_stack = config->task_stack ? config->task_stack : DEFAULT_PLAYER_TASK_STACK;
  int task_prio = config->task_prio ? config->task_prio : DEFAULT_PLAYER_TASK_PRIO;

  //if (xTaskCreate(RunWAVPlayerTask, "replayer", task_stack, ap, task_prio, NULL) != pdTRUE)
  if (xTaskCreatePinnedToCore(RunWAVPlayerTask, "replayer", task_stack, ap, task_prio, &WAVPlayerTask, 0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Player task");
    goto _audio_init_failed;
  }

  return ap;
_audio_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static T_WAVRecorderHandle InitWAVRecorder(T_WAVRecorderConfig* config)
{
  T_WAVRecorderHandle ap = calloc(1, sizeof(T_WAVRecorder));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  //board_handle = Init();
  //audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
  ap->Hal = board_handle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for record");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(RECORD_RATE, RECORD_BITS, RECORD_CHANNEL, AUDIO_STREAM_READER);

  ESP_LOGI(Tag, "[2.2] Create filter to convert to 8 [kHz]");
  ap->Filter = CreateFilter(RECORD_RATE, RECORD_CHANNEL, SAVE_FILE_RATE, SAVE_FILE_CHANNEL, AUDIO_CODEC_TYPE_ENCODER);

  ESP_LOGI(Tag, "[2.3] Create WAV encoder to encode WAV format");
  ap->Encoder = CreateWAVEncoder();
  AUDIO_MEM_CHECK(Tag, ap->Encoder, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create spiffs stream to write data to spi flash");
  ap->SPIFFSStream = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_WRITER);
  AUDIO_MEM_CHECK(Tag, ap->SPIFFSStream, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_reader");
  audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_downsample");
  audio_pipeline_register(ap->Pipeline, ap->Encoder, "wav_encoder");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "file_writer");

  ESP_LOGI(Tag, "[2.6] Link it together [codec_chip]-->i2s_stream-->wav_encoder-->spiffs_stream-->[flash]");
  audio_pipeline_link(ap->Pipeline, (const char *[]) {"i2s_reader", "filter_downsample", "wav_encoder", "file_writer"}, 4);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Recording = false;
  ap->EventHandler = config->event_handler;

  int task_stack = config->task_stack ? config->task_stack : DEFAULT_RECORDER_TASK_STACK;
  int task_prio = config->task_prio ? config->task_prio : DEFAULT_RECORDER_TASK_PRIO;

  //if (xTaskCreate(RunWAVRecorderTask, "replayer", task_stack, ap, task_prio, NULL) != pdTRUE)
  if (xTaskCreatePinnedToCore(RunWAVRecorderTask, "recorder", task_stack, ap, task_prio, &WAVRecorderTask, 0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Recorder task");
    goto _audio_init_failed;
  }

  return ap;
_audio_init_failed:
  return NULL;
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

static audio_element_handle_t CreateMP3Decoder(void)
{
  mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();

  return mp3_decoder_init(&mp3_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateWAVDecoder(void)
{
  wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();

  return wav_decoder_init(&wav_cfg);
}

//_____________________________________________________________________________

static void GenerateMasterClock(uint32_t clock_Hz)
{
  ledc_timer_config_t ledc_timer =
  {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num  = LEDC_TIMER_0,
    .bit_num    = 2,
    .freq_hz    = clock_Hz
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

static void RunMP3PlayerFromFlashTask(void* arg)
{
  T_MP3PlayerFromFlashHandle ap = (T_MP3PlayerFromFlashHandle) arg;

  //i2s_stream_set_clk(ap->I2SStream, WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL);

  while (ap->Run)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_RUNNING)
    {
      SendMP3PlayerFromFlashEvent(ap, PLAYER_EVENT_PLAY);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
      SendMP3PlayerFromFlashEvent(ap, PLAYER_EVENT_PAUSE);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(ap->Decoder, &music_info);

      //ESP_LOGE(Tag, "[ * ] Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d",
      //         music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(ap->I2SStream, &music_info);
      i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives stop event
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*)ap->I2SStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))
        && ap->Playing)
    {
      ESP_LOGI(Tag, "Stop pipeline");
      SendMP3PlayerFromFlashEvent(ap, PLAYER_EVENT_STOP);
      audio_pipeline_stop(ap->Pipeline);
      audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_element_reset_state(ap->Decoder);
      audio_element_reset_state(ap->I2SStream);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_items_state(ap->Pipeline);
      ap->Playing = false;
      //audio_pipeline_terminate(ap->Pipeline);
    }
  }

  vTaskDelete(MP3PlayerFromFlashTask);
}

//_____________________________________________________________________________

static void RunMP3PlayerTask(void* arg)
{
  T_MP3PlayerHandle ap = (T_MP3PlayerHandle) arg;

  //i2s_stream_set_clk(ap->I2SStream, MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL);

  while (ap->Run)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_RUNNING)
    {
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND PLAY");
      SendMP3PlayerEvent(ap, PLAYER_EVENT_PLAY);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND PAUSE");
      SendMP3PlayerEvent(ap, PLAYER_EVENT_PAUSE);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(ap->Decoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(ap->I2SStream, &music_info);
      i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives stop event

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*)ap->I2SStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))
        && ap->Playing)
    {
      ESP_LOGI(Tag, "Stop pipeline");
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND STOP");
      SendMP3PlayerEvent(ap, PLAYER_EVENT_STOP);
      audio_pipeline_stop(ap->Pipeline);
      audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_element_reset_state(ap->SPIFFSStream);
      audio_element_reset_state(ap->Decoder);
      audio_element_reset_state(ap->I2SStream);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_items_state(ap->Pipeline);
      audio_pipeline_terminate(ap->Pipeline);
      ap->Playing = false;
    }
  }

  vTaskDelete(MP3PlayerTask);
}

//_____________________________________________________________________________

static void RunWAVPlayerTask(void* arg)
{
  T_WAVPlayerHandle ap = (T_WAVPlayerHandle) arg;

  i2s_stream_set_clk(ap->I2SStream, WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL);

  while (ap->Run)
  {
    audio_event_iface_msg_t msg;
    esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_RUNNING)
    {
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND PLAY");
      SendWAVPlayerEvent(ap, PLAYER_EVENT_PLAY);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND PAUSE");
      SendWAVPlayerEvent(ap, PLAYER_EVENT_PAUSE);
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(ap->Decoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(ap->I2SStream, &music_info);
      //i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      //i2s_stream_set_clk(ap->I2SStream, WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL);
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives stop event

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*)ap->I2SStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))
        && ap->Playing)
    {
      ESP_LOGI(Tag, "Stop pipeline");
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND STOP");
      SendWAVPlayerEvent(ap, PLAYER_EVENT_STOP);
      audio_pipeline_stop(ap->Pipeline);
      audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_element_reset_state(ap->SPIFFSStream);
      audio_element_reset_state(ap->Filter);
      audio_element_reset_state(ap->Decoder);
      audio_element_reset_state(ap->I2SStream);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_items_state(ap->Pipeline);
      audio_pipeline_terminate(ap->Pipeline);
      ap->Playing = false;
    }
  }

  vTaskDelete(WAVPlayerTask);
}

//_____________________________________________________________________________

static void RunWAVRecorderTask(void* arg)
{
  T_WAVRecorderHandle ap = (T_WAVRecorderHandle) arg;
  bool finished = false;
  static bool started = false;

  i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);

  ESP_LOGE(Tag, "[ 6 ] Listen for all pipeline events, record for %d Seconds", RecordingDuration_s);
  uint16_t second_recorded = 0u;

  while (ap->Run)
  {
    audio_event_iface_msg_t msg;

    ESP_LOGE(Tag, "BON");
    esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, 1000 /portMAX_DELAY);
    ESP_LOGE(Tag, "JOUR");

    if (ret != ESP_OK)
    {
      ESP_LOGE(Tag, "THYMIO");

      if (!started)
      {
        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void*) ap->Encoder
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (int)msg.data == AEL_STATUS_STATE_RUNNING)
        {
          ESP_LOGE(Tag, "[ * ] COUCOU SEND RECORD");
          started = true;
          SendWAVRecorderEvent(ap, RECORDER_EVENT_RECORD);
          continue;
        }
      }
      else
      {
        second_recorded++;

      	ESP_LOGE(Tag, "[ * ] Recording ... %d", second_recorded);

      	if (second_recorded >= RecordingDuration_s)
      	{
          finished = true;
      	}

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            //&& msg.source == (void*) ap->Encoder
            && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
        {
          audio_element_info_t music_info = {0};
          audio_element_getinfo(ap->Encoder, &music_info);

          ESP_LOGE(Tag, "[ * ] Receive music info from WAV encoder, sample_rates=%d, bits=%d, ch=%d",
                   music_info.sample_rates, music_info.bits, music_info.channels);

          audio_element_setinfo(ap->I2SStream, &music_info);
          //i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
          continue;
        }

      	ESP_LOGE(Tag, "3");

    	if (started && finished)
    	{
          ESP_LOGI(Tag, "Stop pipeline");
          ESP_LOGE(Tag, "[ * ] COUCOU SEND STOP");
          SendWAVRecorderEvent(ap, RECORDER_EVENT_STOP);
          audio_pipeline_stop(ap->Pipeline);
          audio_pipeline_wait_for_stop(ap->Pipeline);
          audio_element_reset_state(ap->Filter);
          audio_element_reset_state(ap->SPIFFSStream);
          audio_element_reset_state(ap->Encoder);
          audio_element_reset_state(ap->I2SStream);
          audio_pipeline_reset_ringbuffer(ap->Pipeline);
          audio_pipeline_reset_items_state(ap->Pipeline);
          audio_pipeline_terminate(ap->Pipeline);
          ap->Recording = false;

          finished = false;
          started = false;
          second_recorded = 0u;
          RecordingDuration_s = 0u;

          continue;
    	}

        ESP_LOGE(Tag, "VERSION");

    	ESP_LOGE(Tag, "1");
    	//vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    }
    else
    {
      ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      continue;
    }

    ESP_LOGE(Tag, "END");

	continue;

#if 0
	/* Stop when the last pipeline element (WAVRecorderI2SStream in this case) receives stop event */
	if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *) ap->I2SStream &&
	    msg.cmd == AEL_MSG_CMD_REPORT_STATUS && (((int) msg.data == AEL_STATUS_STATE_STOPPED) ||
	    ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
	{
	  ESP_LOGE(Tag, "[ * ] Stop event received");
	  break;
	}
#endif

#if 0
    audio_event_iface_msg_t msg;
    if (!started)
    {
      ESP_LOGE(Tag, "BON");
      esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, portMAX_DELAY);
      ESP_LOGE(Tag, "JOUR");

      if (ret != ESP_OK)
      {
        ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
        continue;
      }

      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
          && msg.source == (void*) ap->Encoder
          && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
          && (int)msg.data == AEL_STATUS_STATE_RUNNING)
      {
        ESP_LOGE(Tag, "[ * ] COUCOU SEND RECORD");
        started = true;
        SendWAVRecorderEvent(ap, RECORDER_EVENT_RECORD);
        continue;
      }

      ESP_LOGE(Tag, "THYMIO");
    }
    else
    {
      second_recorded++;

      ESP_LOGE(Tag, "[ * ] Recording ... %d", second_recorded);

      if (second_recorded >= RecordingDuration_s)
      {
        //break;
        finished = true;
      }

      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
          && msg.source == (void*) ap->Encoder
          && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
      {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(ap->Encoder, &music_info);

        ESP_LOGE(Tag, "[ * ] Receive music info from WAV encoder, sample_rates=%d, bits=%d, ch=%d",
                 music_info.sample_rates, music_info.bits, music_info.channels);

        audio_element_setinfo(ap->I2SStream, &music_info);
        //i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
        continue;
      }

      if (started && finished)
      {
        // Stop when the last pipeline element (I2SStream in this case) receives stop event
        //if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        //&& msg.source == (void *)ap->I2SStream
        //&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        //&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))
        //&& ap->Recording)
        {
          ESP_LOGI(Tag, "Stop pipeline");
          ESP_LOGE(Tag, "[ * ] COUCOU SEND STOP");
          SendWAVRecorderEvent(ap, RECORDER_EVENT_STOP);
          audio_pipeline_stop(ap->Pipeline);
          audio_pipeline_wait_for_stop(ap->Pipeline);
          audio_element_reset_state(ap->SPIFFSStream);
          audio_element_reset_state(ap->Encoder);
          audio_element_reset_state(ap->I2SStream);
          audio_pipeline_reset_ringbuffer(ap->Pipeline);
          audio_pipeline_reset_items_state(ap->Pipeline);
          audio_pipeline_terminate(ap->Pipeline);
          ap->Recording = false;

          finished = false;
          started = false;
          RecordingDuration_s = 0u;

          continue;
        }
      }
    }

    //vTaskDelay(1000 / portTICK_PERIOD_MS);

#endif

#if 0
    if (RecordingDuration_s > 0u)
    {
      ESP_LOGE(Tag, "0");

      //if (audio_event_iface_listen(ap->Evt, &msg, (1000 / portTICK_RATE_MS)) != ESP_OK)
      if (ret != ESP_OK)
      {
        second_recorded++;

        ESP_LOGE(Tag, "[ * ] Recording ... %d", second_recorded);

        if (second_recorded >= RecordingDuration_s)
        {
          //break;
          finished = true;
        }
        else
        {
          //continue;
        }

        if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
            && msg.source == (void*) ap->Encoder
            && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
            && (int)msg.data == AEL_STATUS_STATE_RUNNING)
        {
          ESP_LOGE(Tag, "[ * ] COUCOU SEND RECORD");
          SendWAVRecorderEvent(ap, RECORDER_EVENT_RECORD);
          continue;
        }

        if (finished)
        {
          // Stop when the last pipeline element (I2SStream in this case) receives stop event
          //if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
          //&& msg.source == (void *)ap->I2SStream
          //&& msg.cmd == AEL_MSG_CMD_REPORT_STATUS
          //&& (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED))
          //&& ap->Recording)
          {
            ESP_LOGI(Tag, "Stop pipeline");
            ESP_LOGE(Tag, "[ * ] COUCOU SEND STOP");
            SendWAVRecorderEvent(ap, RECORDER_EVENT_STOP);
            audio_pipeline_stop(ap->Pipeline);
            audio_pipeline_wait_for_stop(ap->Pipeline);
            audio_element_reset_state(ap->SPIFFSStream);
            audio_element_reset_state(ap->Encoder);
            audio_element_reset_state(ap->I2SStream);
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_items_state(ap->Pipeline);
            ap->Recording = false;

            finished = false;
            RecordingDuration_s = 0u;
          }
        }
      }
#if 0
      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
          && msg.source == (void*) ap->Encoder
          && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
          && (int)msg.data == AEL_STATUS_STATE_PAUSED)
      {
        ESP_LOGE(Tag, "[ * ] COUCOU SEND PAUSE");
        SendWAVPlayerEvent(ap, PLAYER_EVENT_PAUSE);
        continue;
      }

      if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
          && msg.source == (void*) ap->Encoder
          && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
      {
        audio_element_info_t music_info = {0};
        audio_element_getinfo(ap->Decoder, &music_info);

        ESP_LOGE(Tag, "[ * ] Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d",
                 music_info.sample_rates, music_info.bits, music_info.channels);

        audio_element_setinfo(ap->I2SStream, &music_info);
        //i2s_stream_set_clk(ap->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
        //i2s_stream_set_clk(ap->I2SStream, WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL);
        continue;
      }
#endif
    }
    //else
    {
      //ESP_LOGE(Tag, "WAITING");
      //vTaskPrioritySet(WAVRecorderTask, 1);
      //vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
#endif
  }
#if 0
  ESP_LOGI(Tag, "[ 7 ] Stop audio_pipeline");
  audio_pipeline_terminate(ap->Pipeline);

  audio_pipeline_unregister(ap->Pipeline, ap->Encoder);
  audio_pipeline_unregister(ap->Pipeline, ap->Filter);
  audio_pipeline_unregister(ap->Pipeline, ap->I2SStream);
  audio_pipeline_unregister(ap->Pipeline, ap->SPIFFSStream);

  /* Terminal the pipeline before removing the listener */
  audio_pipeline_remove_listener(ap->Pipeline);

  /* Stop all periph before removing the listener */
  esp_periph_set_stop_all(Set);
  audio_event_iface_remove_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  /* Make sure audio_pipeline_remove_listener & audio_event_iface_remove_listener are called before destroying event_iface */
  audio_event_iface_destroy(ap->Evt);

  /* Release all resources */
  audio_element_deinit(ap->I2SStream);
  audio_element_deinit(ap->Filter);
  audio_element_deinit(ap->Encoder);
  audio_element_deinit(ap->SPIFFSStream);
  esp_periph_set_destroy(Set);
#endif

  //vTaskDelete(WAVRecorderTask);
}

//_____________________________________________________________________________

static esp_err_t SendMP3PlayerFromFlashEvent(T_MP3PlayerFromFlashHandle player, T_PlayerEvent event)
{
  if (player->EventHandler)
  {
    return player->EventHandler(player, event);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t SendMP3PlayerEvent(T_MP3PlayerHandle player, T_PlayerEvent event)
{
  if (player->EventHandler)
  {
    return player->EventHandler(player, event);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t SendWAVPlayerEvent(T_WAVPlayerHandle player, T_PlayerEvent event)
{
  if (player->EventHandler)
  {
    return player->EventHandler(player, event);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t SendWAVRecorderEvent(T_WAVRecorderHandle recorder, T_RecorderEvent event)
{
  if (recorder->EventHandler)
  {
    return recorder->EventHandler(recorder, event);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PlayMP3FromFlash(T_MP3PlayerFromFlashHandle ap)
{
  StopMP3FromFlash(ap);

  audio_pipeline_run(ap->Pipeline);
  ap->Playing = true;

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PlayMP3(T_MP3PlayerHandle ap, const char* url)
{
  //ESP_LOGE(Tag, "COUCOU STOP WAV FROM PLAY");
  StopMP3(ap);

  if (url)
  {
    printf("Played MP3 file: %s\n", url);
    audio_element_set_uri(ap->SPIFFSStream, url);
    audio_pipeline_run(ap->Pipeline);
    ap->Playing = true;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PlayWAV(T_WAVPlayerHandle ap, const char* url)
{
  //ESP_LOGE(Tag, "COUCOU STOP WAV FROM PLAY");
  StopWAV(ap);

  if (url)
  {
    printf("Played WAV file: %s\n", url);
    audio_element_set_uri(ap->SPIFFSStream, url);
    audio_pipeline_run(ap->Pipeline);
    ap->Playing = true;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PauseMP3FromFlash(T_MP3PlayerFromFlashHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Pause MP3 file");
    audio_pipeline_pause(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PauseMP3(T_MP3PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Pause MP3 file");
    audio_pipeline_pause(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PauseWAV(T_WAVPlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Pause WAV file");
    audio_pipeline_pause(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t ResumeMP3FromFlash(T_MP3PlayerFromFlashHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Resume MP3 file");
    audio_pipeline_resume(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t ResumeMP3(T_MP3PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Resume MP3 file");
    audio_pipeline_resume(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t ResumeWAV(T_WAVPlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Resume WAV file");
    audio_pipeline_resume(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static int GetMP3PlayedTime(T_MP3PlayerHandle ap)
{
  audio_element_info_t info;

  if (ap == NULL)
  {
    return 0;
  }

  if (audio_element_getinfo(ap->I2SStream, &info) != ESP_OK)
  {
    return 0;
  }

  //int time_sec = info.byte_pos / (info.sample_rates * info.channels * info.bits / 8);
  //printf("Played Time: %d, pos: %lld, rate: %d, channel: %d, bit: %d\n", time_sec, info.byte_pos, info.sample_rates, info.channels, info.bits);

  int time_ms = (info.byte_pos * 8000) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

static int GetWAVPlayedTime(T_WAVPlayerHandle ap)
{
  audio_element_info_t info;

  if (ap == NULL)
  {
    return 0;
  }

  if (audio_element_getinfo(ap->I2SStream, &info) != ESP_OK)
  {
    return 0;
  }

  //int time_sec = info.byte_pos / (info.sample_rates * info.channels * info.bits / 8);
  //printf("Played Time: %d, pos: %lld, rate: %d, channel: %d, bit: %d\n", time_sec, info.byte_pos, info.sample_rates, info.channels, info.bits);

  int time_ms = (info.byte_pos * 8000) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

static esp_err_t RecordWAV(T_WAVRecorderHandle ap, const char* url)
{
  ESP_LOGE(Tag, "COUCOU STOP WAV FROM RECORD");
  StopWAVRecord(ap);

  if (url)
  {
    ESP_LOGE(Tag, "COUCOU RECORD WAV");
    audio_element_set_uri(ap->SPIFFSStream, url);
    audio_pipeline_run(ap->Pipeline);
    ap->Recording = true;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopMP3FromFlash(T_MP3PlayerFromFlashHandle ap)
{
  if (ap->Playing)
  {
    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->Decoder);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Playing = false;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopMP3(T_MP3PlayerHandle ap)
{
  //ESP_LOGE(Tag, "COUCOU STOP MP3");

  if (ap->Playing)
  {
    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->SPIFFSStream);
    //audio_element_reset_state(ap->Filter);
    audio_element_reset_state(ap->Decoder);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Playing = false;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopWAV(T_WAVPlayerHandle ap)
{
  //ESP_LOGE(Tag, "COUCOU STOP WAV");

  if (ap->Playing)
  {
    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->SPIFFSStream);
    //audio_element_reset_state(ap->Filter);
    audio_element_reset_state(ap->Decoder);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Playing = false;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopWAVRecord(T_WAVRecorderHandle ap)
{
  if (ap->Recording)
  {
    ESP_LOGE(Tag, "COUCOU STOP WAV RECORDER");
    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->SPIFFSStream);
    audio_element_reset_state(ap->Encoder);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Recording = false;
  }

  return ESP_OK;
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

//_____________________________________________________________________________

esp_err_t HandleMP3PlayerFromFlashEvent(T_MP3PlayerFromFlashHandle ap, T_PlayerEvent event)
{
  if (event == PLAYER_EVENT_PLAY)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE MP3 PLAY");
  }

  if (event == PLAYER_EVENT_STOP)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE MP3 STOP");
  }

  if (event == PLAYER_EVENT_PAUSE)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE MP3 PAUSE");
  }

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t HandleMP3PlayerEvent(T_MP3PlayerHandle ap, T_PlayerEvent event)
{
  if (event == PLAYER_EVENT_PLAY)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV PLAY");
  }

  if (event == PLAYER_EVENT_STOP)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV STOP");
  }

  if (event == PLAYER_EVENT_PAUSE)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV PAUSE");
  }

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t HandleWAVPlayerEvent(T_WAVPlayerHandle ap, T_PlayerEvent event)
{
  if (event == PLAYER_EVENT_PLAY)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV PLAY");
  }

  if (event == PLAYER_EVENT_STOP)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV STOP");
  }

  if (event == PLAYER_EVENT_PAUSE)
  {
    //ESP_LOGE(Tag, "COUCOU HANDLE WAV PAUSE");
  }

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t HandleWAVRecorderEvent(T_WAVRecorderHandle ap, T_RecorderEvent event)
{
  //ESP_LOGE(Tag, "COUCOU WAV HANDLE");

  if (event == RECORDER_EVENT_RECORD)
  {
    ESP_LOGE(Tag, "COUCOU HANDLE WAV RECORD %d SECONDS", RecordingDuration_s);
    //PlayWAV(WAVPlayer, file);
  }

  if (event == RECORDER_EVENT_STOP)
  {
    ESP_LOGE(Tag, "COUCOU HANDLE WAV RECORD STOP");
    //StopWAV(WAVPlayer);
  }

  if (event == RECORDER_EVENT_PAUSE)
  {
    ESP_LOGE(Tag, "COUCOU HANDLE WAV RECORD PAUSE");
  }

  return ESP_OK;
}
