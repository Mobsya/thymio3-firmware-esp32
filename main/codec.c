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

#include "esp_log.h"

#include "audio_element.h"
#include "audio_pipeline.h"
#include "audio_event_iface.h"
#include "audio_mem.h"
#include "audio_common.h"
#include "tone_stream.h"

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

#include "thymio_es8374.h"
#include "file_system.h"
#include "pins_def.h"
#include "board.h"
#include "audio_tone_uri.h"
#include "sensors.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MP3_PLAYER_RATE         48000
#define MP3_PLAYER_CHANNEL          1  //!< Mono = 1
#define MP3_PLAYER_BITS            16

#define RECORD_RATE             8000
#define RECORD_CHANNEL              1  //!< Mono = 1
#define RECORD_BITS                16

#define WAV_PLAYER_RATE          8000
#define WAV_PLAYER_CHANNEL          1  //!< Mono = 1
#define WAV_PLAYER_BITS            16

#define SAVE_FILE_RATE           8000
#define SAVE_FILE_CHANNEL           1  //!< Mono = 1
#define SAVE_FILE_BITS             16

#define DEFAULT_AUDIO_TASK_STACK (4*1024)
#define DEFAULT_AUDIO_TASK_PRIO (5)

#define BUF_SIZE (SAVE_FILE_RATE * 1) /* 2 second buffer */

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

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

typedef struct AudioPlayer* T_PlayerHandle;
typedef struct AudioRecorder* T_RecorderHandle;

typedef struct AudioPlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t Mp3Decoder;
  audio_element_handle_t SPIFFSStream;
  audio_element_handle_t FlashToneStream;
  audio_element_handle_t WavDecoder;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Playing;
  uint8_t mode; // 0 = play mp3 from flash, 1 = play mp3 from file system, 2 = play wav from file system
} T_Player;

typedef struct AudioRecorder
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
} T_Recorder;

typedef enum
{
  E_SoundStatus_Started,
  E_SoundStatus_Finished
} T_SoundStatus;


//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "codec";

static TaskHandle_t AudioTask 	= NULL;

static int16_t FileIndex = -1;
static int16_t FileIndexFromFlash = 0; //-1;
static uint16_t RecordingDuration_s = 0;

static esp_periph_set_handle_t Set;

static audio_board_handle_t BoardHandle = 0;

static T_PlayerHandle          Player          = NULL;
static T_RecorderHandle        Recorder        = NULL;

//static int16_t buffer[4 * BUF_SIZE];

static T_SoundStatus SoundStatus[15];
int64_t start_rec_time, end_rec_time;

#define MY_DEFAULT_MP3_DECODER_CONFIG() {                  \
    .out_rb_size        = MP3_DECODER_RINGBUFFER_SIZE,  \
    .task_stack         = MP3_DECODER_TASK_STACK_SIZE,  \
    .task_core          = MP3_DECODER_TASK_CORE,        \
    .task_prio          = MP3_DECODER_TASK_PRIO,        \
    .stack_in_ext       = false,                         \
}

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the SPI file system
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static void InitSPIFFS(void);

//! \brief     Initialize the MP3 player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_PlayerHandle InitPlayer(void);

//! \brief     Initialize the WAV recorder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_RecorderHandle InitRecorder(void);

//! \brief     Create the SPI file system stream
//! \pre       First initialize the codec
//! \param     sampleRates - Sample rates of the SPIFFS stream in [Hz]
//! \param     bits - Bit wide
//! \param     channels - Number of audio channels, mono is 1, stereo is 2
//! \param     type - Direction of the SPIFFS stream (reader or writer)
//! \return    None
static audio_element_handle_t CreateSPIFFSStream(int sampleRates, int bits, int channels, audio_stream_type_t type);

//! \brief     Create the I2S stream
//! \pre       First initialize the codec
//! \param     sampleRates - Sample rates of the I2S stream in [Hz]
//! \param     bits - Bit wide
//! \param     channels - Number of audio channels, mono is 1, stereo is 2
//! \param     type - Direction of the I2S stream (reader or writer)
//! \return    None
static audio_element_handle_t CreateI2SStream(int sampleRates, int bits, int channels, audio_stream_type_t type);

//! \brief     Create the filter
//! \pre       First initialize the codec
//! \param     sourceRate - Input rate of the filter in [Hz]
//! \param     sourceChannel - Input channel of the filter, mono is 1, stereo is 2
//! \param     destRate - Output rate of the filter in [Hz]
//! \param     destChannel - Output channel of the filter, mono is 1, stereo is 2
//! \param     mode - Resampling mode
//! \return    None
//static audio_element_handle_t CreateFilter(int sourceRate, int sourceChannel, int destRate, int destChannel, int mode);
static audio_element_handle_t CreateFilter(int sourceRate, int sourceChannel, int destRate, int destChannel,
    audio_codec_type_t type);

//! \brief     Create the MP3 decoder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static audio_element_handle_t CreateMP3Decoder(void);

//! \brief     Create the WAV decoder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static audio_element_handle_t CreateWAVDecoder(void);

//! \brief     Create the WAV encoder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static audio_element_handle_t CreateWAVEncoder(void);

//! \brief     Select the file (from/to the flash)
//! \pre       First initialize the codec
//! \param     index - Index of the selected file
//! \return    None
static void SelectFile(T_SoundIndex index);

//! \brief     Run the MP3 player task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunAudioTask(void* arg);

//! \brief     Play a MP3 file (from the flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayMP3FromFlash(T_PlayerHandle ap);

//! \brief     Play a MP3 file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayMP3(T_PlayerHandle ap, const char* url);

//! \brief     Play a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayWAV(T_PlayerHandle ap, const char* url);

//! \brief     Pause a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PauseMP3(T_PlayerHandle ap);

//! \brief     Pause a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PauseWAV(T_PlayerHandle ap);

//! \brief     Resume a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t ResumeMP3(T_PlayerHandle ap);

//! \brief     Resume a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t ResumeWAV(T_PlayerHandle ap);

//! \brief     Get the played time [s/10] of a MP3 file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static int GetMP3PlayedTime(T_PlayerHandle ap);

//! \brief     Get the played time [s/10] of a WAV file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static int GetWAVPlayedTime(T_PlayerHandle ap);

//! \brief     Record a WAV file (to the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t RecordWAV(T_RecorderHandle ap, const char* url);

//! \brief     Stop playing a MP3 file
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopMP3(T_PlayerHandle ap);

//! \brief     Stop playing a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopWAV(T_PlayerHandle ap);

//! \brief     Stop recording a WAV file (to the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopWAVRecord(T_RecorderHandle ap);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Codec_Init(void)
{
	gpio_set_direction(GPIO_NUM_17, GPIO_MODE_INPUT);
	gpio_set_pull_mode(GPIO_NUM_17, GPIO_FLOATING);

  InitSPIFFS();

  //ESP_LOGI(Tag, "[2] Start codec chip");
  BoardHandle = audio_board_init();
  audio_hal_ctrl_codec(BoardHandle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);
  //audio_hal_ctrl_codec(BoardHandle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);

  ESP_LOGE(Tag, "INIT PLAYER");
  Player = InitPlayer();

  ESP_LOGE(Tag, "INIT RECORDER");
  Recorder = InitRecorder();

}

//_____________________________________________________________________________

void Codec_CreateWAVFile(int16_t index, int16_t freq_Hz)
{
	/*
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

  for (uint16_t i = BUF_SIZE; i < 2 * BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  phase = 0;
  freq_radians_per_sample = ((659.25 * 2 * M_PI) / SAVE_FILE_RATE);

  for (uint16_t i = 2 * BUF_SIZE; i < 3 * BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  phase = 0;
  freq_radians_per_sample = ((880.00 * 2 * M_PI) / SAVE_FILE_RATE);

  for (uint16_t i = 3 * BUF_SIZE; i < 4 * BUF_SIZE; i++)
  {
    phase += freq_radians_per_sample;
    buffer[i] = (int16_t)(amplitude * sin(phase));
  }

  FileSystem_SelectFile(&fileName, index, E_Extension_WAV);

  FileSystem_WriteWAVFile(fileName, 4 * BUF_SIZE, buffer, SAVE_FILE_RATE, WAV_PLAYER_CHANNEL);
  */
}

//_____________________________________________________________________________

void Codec_PlayMP3FileFromFlash(T_SoundIndex index)
{
	//return; // Used for debugging in order to not use the player.

  FileIndexFromFlash = index;

  SelectFile(FileIndexFromFlash);

  PlayMP3FromFlash(Player);

}

//_____________________________________________________________________________

void Codec_PlayMP3File(int16_t index)
{
	//return; // Used for debugging in order to not use the player.

  char* fileName;

  FileIndex = index;

  FileSystem_SelectFile(&fileName, FileIndex, E_Extension_MP3);

  // Check that the file exists
  if (FileSystem_DoesFileExist(fileName))
  {
    PlayMP3(Player, fileName);
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
	  Sensors_buttons_pause();
    PlayWAV(Player, fileName);
  }
}

//_____________________________________________________________________________

void Codec_PauseMP3File(void)
{
  PauseMP3(Player);
}

//_____________________________________________________________________________

void Codec_PauseWAVFile(void)
{
  PauseWAV(Player);
}

//_____________________________________________________________________________

void Codec_ResumeMP3File(void)
{
  ResumeMP3(Player);
}

//_____________________________________________________________________________

void Codec_ResumeWAVFile(void)
{
  ResumeWAV(Player);
}

//_____________________________________________________________________________

int Codec_GetMP3PlayedTime(void)
{
  return GetMP3PlayedTime(Player);
}

//_____________________________________________________________________________

int Codec_GetWAVPlayedTime(void)
{
  return GetWAVPlayedTime(Player);
}

//_____________________________________________________________________________

void Codec_RecordWAVFile(int16_t index, uint16_t duration_s)
{
  char* fileName;

  RecordingDuration_s = duration_s;

  FileIndex = index;

  FileSystem_SelectFile(&fileName, FileIndex, E_Extension_WAV);

  Sensors_buttons_pause();
  RecordWAV(Recorder, fileName);
}

//_____________________________________________________________________________

void Codec_SetVolume(int16_t volume)
{
  ES8374_SetVoiceVolume(volume);
}

//_____________________________________________________________________________

bool Codec_IsSoundFinished(T_SoundIndex index)
{
  return (SoundStatus[index] == E_SoundStatus_Finished);
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

static T_PlayerHandle InitPlayer(void)
{
  T_PlayerHandle ap = calloc(1, sizeof(T_Player));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);
  //ap->Pipeline2 = audio_pipeline_init(&pipeline_cfg);
  //AUDIO_MEM_CHECK(Tag, ap->Pipeline2, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create spiffs stream to read data from spi flash");
  ap->SPIFFSStream = CreateSPIFFSStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_READER);
  AUDIO_MEM_CHECK(Tag, ap->SPIFFSStream, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.2] Create MP3 decoder to decode MP3 format");
  ap->Mp3Decoder = CreateMP3Decoder();
  AUDIO_MEM_CHECK(Tag, ap->Mp3Decoder, goto _audio_init_failed);

  tone_stream_cfg_t tone_cfg = TONE_STREAM_CFG_DEFAULT();
  tone_cfg.type = AUDIO_STREAM_READER;
  ap->FlashToneStream = tone_stream_init(&tone_cfg);
  AUDIO_MEM_CHECK(Tag, ap->FlashToneStream, goto _audio_init_failed);

  //ESP_LOGI(Tag, "[2.3] Create filter to convert to 48 [kHz]");
  //ap->Filter = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, WAV_PLAYER_RATE, WAV_PLAYER_CHANNEL, AUDIO_CODEC_TYPE_DECODER);
  //AUDIO_MEM_CHECK(Tag, ap->Filter, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.3] Create WAV decoder to decode WAV format");
  ap->WavDecoder = CreateWAVDecoder();
  AUDIO_MEM_CHECK(Tag, ap->WavDecoder, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(MP3_PLAYER_RATE, MP3_PLAYER_BITS, MP3_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->Mp3Decoder,   "mp3_flash_decoder");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "mp3_file_reader");
  //audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_upsample");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");
  audio_pipeline_register(ap->Pipeline, ap->FlashToneStream,   "flash_tone");
  audio_pipeline_register(ap->Pipeline, ap->WavDecoder,   "wav_decoder");

  ESP_LOGI(Tag, "[2.6] Link it together [mp3_music_read_cb]-->mp3_flash_decoder-->i2s_stream-->[codec_chip]");
  //audio_pipeline_link(ap->Pipeline, (const char* []) {"mp3_file_reader", "mp3_flash_decoder", "i2s_writer"}, 3);
  //audio_pipeline_link(ap->Pipeline, (const char* []) {"mp3_flash_decoder", "i2s_writer"}, 2);
  audio_pipeline_link(ap->Pipeline, (const char* []) {"flash_tone", "mp3_flash_decoder", "i2s_writer"}, 3);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;
  ap->mode = 0;
/*
  if (xTaskCreatePinnedToCore(
        RunAudioTask,
        "sys_player",
        DEFAULT_AUDIO_TASK_STACK,
        ap,
        DEFAULT_AUDIO_TASK_PRIO,
        &AudioTask,
        0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Player task");
    goto _audio_init_failed;
  }
*/
  return ap;
_audio_init_failed:
  return NULL;
}


//_____________________________________________________________________________

static T_RecorderHandle InitRecorder(void)
{
  T_RecorderHandle ap = calloc(1, sizeof(T_Recorder));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for record");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create I2S stream to read audio data from codec chip");
  //ap->I2SStream = CreateI2SStream(RECORD_RATE, RECORD_BITS, RECORD_CHANNEL, AUDIO_STREAM_READER);
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_READER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;
  i2s_cfg.i2s_config.sample_rate = RECORD_RATE;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);

  ESP_LOGI(Tag, "[2.2] Create filter to convert to 8 [kHz]");
  ap->Filter = CreateFilter(RECORD_RATE, RECORD_CHANNEL, SAVE_FILE_RATE, SAVE_FILE_CHANNEL, AUDIO_CODEC_TYPE_ENCODER);

  ESP_LOGI(Tag, "[2.3] Create WAV encoder to encode WAV format");
  ap->Encoder = CreateWAVEncoder();
  AUDIO_MEM_CHECK(Tag, ap->Encoder, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create spiffs stream to write data to spi flash");
  ap->SPIFFSStream = CreateSPIFFSStream(SAVE_FILE_RATE, SAVE_FILE_BITS, SAVE_FILE_CHANNEL, AUDIO_STREAM_WRITER);
  //ap->SPIFFSStream = CreateSPIFFSStream(RECORD_RATE, SAVE_FILE_BITS, RECORD_CHANNEL, AUDIO_STREAM_WRITER);
  AUDIO_MEM_CHECK(Tag, ap->SPIFFSStream, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_reader");
  audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_downsample");
  audio_pipeline_register(ap->Pipeline, ap->Encoder, "wav_encoder");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "file_writer");

  ESP_LOGI(Tag, "[2.6] Link it together [codec_chip]-->i2s_stream-->wav_encoder-->spiffs_stream-->[flash]");
  //audio_pipeline_link(ap->Pipeline, (const char* []) {"i2s_reader", "filter_downsample", "wav_encoder", "file_writer"}, 4);
  audio_pipeline_link(ap->Pipeline, (const char* []) {"i2s_reader", "wav_encoder", "file_writer"}, 3);

  //ESP_LOGI(Tag, "[3.0] Setup event listener");
  //audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG(); //commented
  //ap->Evt = audio_event_iface_init(&evt_cfg); //commented
  ap->Evt = Player->Evt;

  //ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  //audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt); //commented

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Recording = false;

  //i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);

  if (xTaskCreatePinnedToCore(
        RunAudioTask,
        "sys_player",
        DEFAULT_AUDIO_TASK_STACK,
        ap,
        DEFAULT_AUDIO_TASK_PRIO,
        &AudioTask,
        0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Player task");
    goto _audio_init_failed;
  }

  return ap;
_audio_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static audio_element_handle_t CreateSPIFFSStream(int sampleRates, int bits, int channels, audio_stream_type_t type)
{
  spiffs_stream_cfg_t spiffs_cfg = SPIFFS_STREAM_CFG_DEFAULT();
  spiffs_cfg.type = type;
  //spiffs_cfg.task_prio = 7;

  audio_element_handle_t spiffs_stream = spiffs_stream_init(&spiffs_cfg);
  mem_assert(spiffs_stream);

  audio_element_info_t writer_info = {0};
  audio_element_getinfo(spiffs_stream, &writer_info);
  writer_info.bits = bits;
  writer_info.channels = channels;
  writer_info.sample_rates = sampleRates;
  audio_element_setinfo(spiffs_stream, &writer_info);

  return spiffs_stream;
}

//_____________________________________________________________________________

static audio_element_handle_t CreateI2SStream(int sampleRates, int bits, int channels, audio_stream_type_t type)
{
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = type;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ALL_RIGHT;
  i2s_cfg.i2s_config.sample_rate = sampleRates;

  audio_element_handle_t i2s_stream = i2s_stream_init(&i2s_cfg);
/*
  // Define the pins of the I2S
  i2s_pin_config_t i2s_pin_cfg =
  {
    .bck_io_num   = I2S_SCLK_PIN,
    .ws_io_num    = I2S_LCLK_PIN,
    .data_out_num = I2S_DSIN_PIN,
    .data_in_num  = I2S_DOUT_PIN
  };

  i2s_set_pin(i2s_cfg.i2s_port, &i2s_pin_cfg);

  mem_assert(i2s_stream);

  audio_element_info_t i2s_info = {0};
  audio_element_getinfo(i2s_stream, &i2s_info);
  i2s_info.bits = bits;
  i2s_info.channels = channels;
  i2s_info.sample_rates = sampleRates;
  audio_element_setinfo(i2s_stream, &i2s_info);
*/
  return i2s_stream;
}

//_____________________________________________________________________________

//static audio_element_handle_t CreateFilter(int sourceRate, int sourceChannel, int destRate, int destChannel, int mode)
static audio_element_handle_t CreateFilter(int sourceRate, int sourceChannel, int destRate, int destChannel,
    audio_codec_type_t type)
{
  rsp_filter_cfg_t rsp_cfg = DEFAULT_RESAMPLE_FILTER_CONFIG();
  rsp_cfg.src_rate = sourceRate;
  rsp_cfg.src_ch = sourceChannel;
  rsp_cfg.dest_rate = destRate;
  rsp_cfg.dest_ch = destChannel;
  rsp_cfg.out_rb_size = (8 * 1024);
  //rsp_cfg.type = type;
  //rsp_cfg.task_prio = 8;
  rsp_cfg.stack_in_ext = false;

  return rsp_filter_init(&rsp_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateMP3Decoder(void)
{
  mp3_decoder_cfg_t mp3_cfg = MY_DEFAULT_MP3_DECODER_CONFIG();

  return mp3_decoder_init(&mp3_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateWAVDecoder(void)
{
  wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
  wav_cfg.stack_in_ext = false;
  //wav_cfg.out_rb_size = (16 * 1024);
  //wav_cfg.task_stack = (8 * 1024);

  return wav_decoder_init(&wav_cfg);
}

//_____________________________________________________________________________

static audio_element_handle_t CreateWAVEncoder(void)
{
  wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
  wav_cfg.stack_in_ext = 0;
  //wav_cfg.task_prio = 8;

  return wav_encoder_init(&wav_cfg);
}

//_____________________________________________________________________________

static void SelectFile(T_SoundIndex index)
{
  switch (index)
  {
    case E_SoundIndex_Startup:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_MAGIC_44100]);
      break;

    case E_SoundIndex_Tick:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_TICK_44100]);
      break;

    case E_SoundIndex_Blop:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_BLOP_44100]);
      break;

    case E_SoundIndex_Fall:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_FALL_44100]);
      break;

    case E_SoundIndex_Detection:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_DETECT_44100]);
      break;

    case E_SoundIndex_Bye:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_BYE_44100]);
      break;

    case E_SoundIndex_C3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_C3_44100]);
      break;

    case E_SoundIndex_D3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_D3_44100]);
      break;

    case E_SoundIndex_E3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_E3_44100]);
      break;

    case E_SoundIndex_F3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_F3_44100]);
      break;

    case E_SoundIndex_G3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_G3_44100]);
      break;

    case E_SoundIndex_A3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_A3_44100]);
      break;

    case E_SoundIndex_B3:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_B3_44100]);
      break;

    case E_SoundIndex_Alarm:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_ALARM_44100]);
      break;

    case E_SoundIndex_Good:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_GOOD_44100]);
      break;

    case E_SoundIndex_Bad:
    	audio_element_set_uri(Player->FlashToneStream, tone_uri[TONE_TYPE_BAD_44100]);
      break;

    default:
      ESP_LOGW(Tag, "Not supported index = %d", index);
      break;
  }

  SoundStatus[index] = E_SoundStatus_Started;
}

//_____________________________________________________________________________

static void RunAudioTask(void* arg)
{
  //T_PlayerHandle ap = (T_PlayerHandle) arg;
  T_RecorderHandle ap = (T_RecorderHandle) arg;

  i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);
  int second_recorded = 0;
  while (ap->Run)
  {
    audio_event_iface_msg_t msg;
    //esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, portMAX_DELAY);
    esp_err_t ret = audio_event_iface_listen(ap->Evt, &msg, 1000 / portTICK_RATE_MS);

    /*
    if (Recorder->Recording) {
    	end_rec_time = esp_timer_get_time();
    	if((end_rec_time - start_rec_time) >= ((int64_t)RecordingDuration_s*1000000)) {
    		StopWAVRecord(Recorder);
    		//audio_element_set_ringbuf_done(ap->I2SStream);
    	}
    }
    */

    if (ret != ESP_OK)
    {
      //ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      if (Recorder->Recording)
      {
        second_recorded++;

        ESP_LOGE(Tag, "[ * ] Recording ... %d", second_recorded);

        if (second_recorded >= RecordingDuration_s)
        {
          //break;
          StopWAVRecord(Recorder);
        	//audio_element_set_ringbuf_done(ap->I2SStream);
          second_recorded = 0;
        }
      }

      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) Player->Mp3Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_RUNNING)
    {
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) Player->Mp3Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
      continue;
    }

    // Handle rate for mp3 files.
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) Player->Mp3Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(Player->Mp3Decoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(Player->I2SStream, &music_info);
      i2s_stream_set_clk(Player->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);
      continue;
    }

    // Handle rate for wav files.
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) Player->WavDecoder
        && msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO)
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(Player->WavDecoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      audio_element_setinfo(Player->I2SStream, &music_info);
      //i2s_stream_set_clk(Player->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels);	// If using this function, then when playing a wav there is a little pause (no sound) just after the start and then the wav is played normally.
      	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	  	// So do not use it to avoid this problem.
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives stop event
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*)Player->I2SStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
		&& ((int)msg.data == AEL_STATUS_STATE_FINISHED)) // STOPPED state already handled by Stop... functions
        //&& ap->Playing)
    {
      ESP_LOGE(Tag, "Stop pipeline from task");
      Sensors_buttons_resume();
/*
      //audio_pipeline_stop(ap->Pipeline);
      //audio_pipeline_wait_for_stop(ap->Pipeline);
      //audio_pipeline_terminate(ap->Pipeline);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_elements(ap->Pipeline);
      audio_pipeline_change_state(ap->Pipeline, AEL_STATE_INIT);
      ap->Playing = false;
*/

    /*
      audio_pipeline_stop(ap->Pipeline);
      audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_element_reset_state(ap->Mp3Decoder);
      audio_element_reset_state(ap->SPIFFSStream);
      audio_element_reset_state(ap->I2SStream);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_items_state(ap->Pipeline);
      ap->Playing = false;
      //audio_pipeline_terminate(ap->Pipeline);
		*/

      //if(Player->mode == 0) {
    	  SoundStatus[FileIndexFromFlash] = E_SoundStatus_Finished;
      //}
    }

    /* Enable buttons when the last pipeline element (spiffs_stream_writer) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void *)ap->SPIFFSStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)
            || ((int)msg.data == AEL_STATUS_ERROR_OPEN))) {
        //ESP_LOGW(TAG, "[ * ] Stop event received");
        //break;
    	//StopWAVRecord(Recorder);
    	Sensors_buttons_resume();
    }


  }

  vTaskDelete(AudioTask);
}

//_____________________________________________________________________________

static esp_err_t PlayMP3FromFlash(T_PlayerHandle ap)
{
	audio_element_state_t el_state = audio_element_get_state(ap->I2SStream);
    switch (el_state) {
        case AEL_STATE_INIT:
        	ESP_LOGE(Tag, "[Flash] AEL_STATE_INIT");
            break;
        case AEL_STATE_RUNNING:
        	ESP_LOGE(Tag, "[Flash] AEL_STATE_RUNNING");
            audio_pipeline_stop(ap->Pipeline);
            audio_pipeline_wait_for_stop(ap->Pipeline);
            audio_pipeline_terminate(ap->Pipeline);
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            ap->Playing = false;
            break;
        case AEL_STATE_PAUSED:
        	ESP_LOGE(Tag, "[Flash] AEL_STATE_PAUSED");
        	if(ap->mode != 0) {
				audio_pipeline_stop(ap->Pipeline);
				audio_pipeline_wait_for_stop(ap->Pipeline);
				audio_pipeline_terminate(ap->Pipeline);
				audio_pipeline_reset_ringbuffer(ap->Pipeline);
				audio_pipeline_reset_elements(ap->Pipeline);
				ap->Playing = false;
        	}
            break;
        case AEL_STATE_FINISHED:
        	ESP_LOGE(Tag, "[Flash] AEL_STATE_FINISHED");
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            audio_pipeline_change_state(ap->Pipeline, AEL_STATE_INIT);
            break;
        default:
            ESP_LOGE(Tag, "[Flash] Not supported state %d", el_state);
        	break;
    }

	ESP_LOGE(Tag, "[Flash] reader = %d", audio_element_get_state(ap->SPIFFSStream));
	ESP_LOGE(Tag, "[Flash] decoder = %d", audio_element_get_state(ap->Mp3Decoder));
	ESP_LOGE(Tag, "[Flash] writer = %d", audio_element_get_state(ap->I2SStream));

	if (ap->mode == 1) { // If the pipeline is configured to play mp3 from file system
		vTaskDelete(AudioTask);
		ESP_LOGE(Tag, "[Flash] changing pipeline to flashtone=>mp3=>i2s");
		// From  "spiffs => mp3 => i2s" to "flashtone => mp3 => i2s"; break pipeline at first element.
		audio_pipeline_breakup_elements(ap->Pipeline, ap->SPIFFSStream);
		audio_pipeline_relink(ap->Pipeline, (const char* []) {"flash_tone", "mp3_flash_decoder", "i2s_writer"}, 3);
		audio_pipeline_set_listener(ap->Pipeline, ap->Evt);
		audio_pipeline_set_listener(Recorder->Pipeline, ap->Evt);
		ap->mode = 0;
		xTaskCreatePinnedToCore(RunAudioTask, "sys_player", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, &AudioTask, 0);

	} else if (ap->mode == 2) { // If the pipeline is configured to play wav from file system
		vTaskDelete(AudioTask);
		ESP_LOGE(Tag, "[Flash] changing pipeline to flashtone=>mp3=>i2s");
		// From  "spiffs => wav => i2s" to "flashtone => mp3 => i2s"; break pipeline at second element.
		audio_pipeline_breakup_elements(ap->Pipeline, ap->WavDecoder);
		audio_pipeline_relink(ap->Pipeline, (const char* []) {"flash_tone", "mp3_flash_decoder", "i2s_writer"}, 3);
		audio_pipeline_set_listener(ap->Pipeline, ap->Evt);
		audio_pipeline_set_listener(Recorder->Pipeline, ap->Evt);
		ap->mode = 0;
		xTaskCreatePinnedToCore(RunAudioTask, "sys_player", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, &AudioTask, 0);
	}

  audio_pipeline_run(ap->Pipeline);
  ap->Playing = true;

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PlayMP3(T_PlayerHandle ap, const char* url)
{
	audio_element_state_t el_state = audio_element_get_state(ap->I2SStream);
    switch (el_state) {
        case AEL_STATE_INIT:
        	ESP_LOGE(Tag, "[FS] AEL_STATE_INIT");
            break;
        case AEL_STATE_RUNNING:
        	ESP_LOGE(Tag, "[FS] AEL_STATE_RUNNING");
            audio_pipeline_stop(ap->Pipeline);
            audio_pipeline_wait_for_stop(ap->Pipeline); // This function set pipeline to state "AEL_STATE_INIT"
            audio_pipeline_terminate(ap->Pipeline);
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            ap->Playing = false;
            break;
        case AEL_STATE_PAUSED:
        	ESP_LOGE(Tag, "[FS] AEL_STATE_PAUSED");
        	if(ap->mode != 1) {
				audio_pipeline_stop(ap->Pipeline);
				audio_pipeline_wait_for_stop(ap->Pipeline);
				audio_pipeline_terminate(ap->Pipeline);
				audio_pipeline_reset_ringbuffer(ap->Pipeline);
				audio_pipeline_reset_elements(ap->Pipeline);
				ap->Playing = false;
        	}
            break;
        case AEL_STATE_FINISHED:
        	ESP_LOGE(Tag, "[FS] AEL_STATE_FINISHED");
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            audio_pipeline_change_state(ap->Pipeline, AEL_STATE_INIT);
            break;
        default:
        	ESP_LOGE(Tag, "[FS] Not supported state %d", el_state);
        	break;
    }

	ESP_LOGE(Tag, "[FS] reader = %d", audio_element_get_state(ap->SPIFFSStream));
	ESP_LOGE(Tag, "[FS] decoder = %d", audio_element_get_state(ap->Mp3Decoder));
	ESP_LOGE(Tag, "[FS] writer = %d", audio_element_get_state(ap->I2SStream));

	if (ap->mode == 0) { // If the pipeline is configured to play mp3 from flash
		vTaskDelete(AudioTask);
		ESP_LOGE(Tag, "[FS] changing pipeline to spiffs=>mp3=>i2s");
		// From  "flashtone => mp3 => i2s" to "spiffs => mp3 => i2s"; break pipeline at first element.
		audio_pipeline_breakup_elements(ap->Pipeline, ap->FlashToneStream);
		audio_pipeline_relink(ap->Pipeline, (const char *[]) {"mp3_file_reader", "mp3_flash_decoder", "i2s_writer"}, 3);
		audio_pipeline_set_listener(ap->Pipeline, ap->Evt);
		audio_pipeline_set_listener(Recorder->Pipeline, ap->Evt);
		ap->mode = 1;
		xTaskCreatePinnedToCore(RunAudioTask, "sys_player", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, &AudioTask, 0);
	} else if (ap->mode == 2) { // If the pipeline is configured to play wav from file system
		vTaskDelete(AudioTask);
		ESP_LOGE(Tag, "[FS] changing pipeline to spiffs=>mp3=>i2s");
		// From  "spiffs => wav => i2s" to "spiffs => mp3 => i2s"; break pipeline at second element.
		audio_pipeline_breakup_elements(ap->Pipeline, ap->WavDecoder);
		audio_pipeline_relink(ap->Pipeline, (const char *[]) {"mp3_file_reader", "mp3_flash_decoder", "i2s_writer"}, 3);
		audio_pipeline_set_listener(ap->Pipeline, ap->Evt);
		audio_pipeline_set_listener(Recorder->Pipeline, ap->Evt);
		ap->mode = 1;
		xTaskCreatePinnedToCore(RunAudioTask, "sys_player", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, &AudioTask, 0);
	}

  if (url)
  {
	ESP_LOGI(Tag, "Played MP3 file: %s\n", url);
    audio_element_set_uri(ap->SPIFFSStream, url);
    audio_pipeline_run(ap->Pipeline);
    ap->Playing = true;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PlayWAV(T_PlayerHandle ap, const char* url)
{

	audio_element_state_t el_state = audio_element_get_state(ap->I2SStream);
    switch (el_state) {
        case AEL_STATE_INIT:
        	ESP_LOGE(Tag, "[WAV] AEL_STATE_INIT");
            break;
        case AEL_STATE_RUNNING:
        	ESP_LOGE(Tag, "[WAV] AEL_STATE_RUNNING");
            audio_pipeline_stop(ap->Pipeline);
            audio_pipeline_wait_for_stop(ap->Pipeline); // This function set pipeline to state "AEL_STATE_INIT"
            audio_pipeline_terminate(ap->Pipeline);
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            ap->Playing = false;
            break;
        case AEL_STATE_PAUSED:
        	ESP_LOGE(Tag, "[WAV] AEL_STATE_PAUSED");
        	if(ap->mode != 2) {
				audio_pipeline_stop(ap->Pipeline);
				audio_pipeline_wait_for_stop(ap->Pipeline);
				audio_pipeline_terminate(ap->Pipeline);
				audio_pipeline_reset_ringbuffer(ap->Pipeline);
				audio_pipeline_reset_elements(ap->Pipeline);
				ap->Playing = false;
        	}
            break;
        case AEL_STATE_FINISHED:
        	ESP_LOGE(Tag, "[WAV] AEL_STATE_FINISHED");
            audio_pipeline_reset_ringbuffer(ap->Pipeline);
            audio_pipeline_reset_elements(ap->Pipeline);
            audio_pipeline_change_state(ap->Pipeline, AEL_STATE_INIT);
            break;
        default:
        	ESP_LOGE(Tag, "[FS] Not supported state %d", el_state);
        	break;
    }

	ESP_LOGE(Tag, "[WAV] reader = %d", audio_element_get_state(ap->SPIFFSStream));
	ESP_LOGE(Tag, "[WAV] decoder = %d", audio_element_get_state(ap->Mp3Decoder));
	ESP_LOGE(Tag, "[WAV] writer = %d", audio_element_get_state(ap->I2SStream));

	if (ap->mode != 2) { // If the pipeline is configured to play mp3 either from flash or from file system
		vTaskDelete(AudioTask);
		ESP_LOGE(Tag, "[WAV] changing pipeline to spiffs=>wav=>i2s");
		// From  "spiffs => mp3 => i2s" to "spiffs => wav => i2s"; break pipeline at second element.
		// Or from "flashtone => mp3 => i2s" to "spiffs => wav => i2s"; break pipeline at second element.
		audio_pipeline_breakup_elements(ap->Pipeline, ap->Mp3Decoder);
		audio_pipeline_relink(ap->Pipeline, (const char *[]) {"mp3_file_reader", "wav_decoder", "i2s_writer"}, 3);
		audio_pipeline_set_listener(ap->Pipeline, ap->Evt);
		audio_pipeline_set_listener(Recorder->Pipeline, ap->Evt);
		ap->mode = 2;
		xTaskCreatePinnedToCore(RunAudioTask, "sys_player", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, &AudioTask, 0);
	}

  if (url)
  {
	  ESP_LOGI(Tag, "Played WAV file: %s\n", url);
	    audio_element_set_uri(ap->SPIFFSStream, url);
	    audio_pipeline_run(ap->Pipeline);
	    ap->Playing = true;
  }

  return ESP_OK;

}

//_____________________________________________________________________________

static esp_err_t PauseMP3(T_PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Pause MP3 file");
    audio_pipeline_pause(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t PauseWAV(T_PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Pause WAV file");
    audio_pipeline_pause(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t ResumeMP3(T_PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Resume MP3 file");
    audio_pipeline_resume(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t ResumeWAV(T_PlayerHandle ap)
{
  if (ap->Playing)
  {
    ESP_LOGE(Tag, "Resume WAV file");
    audio_pipeline_resume(ap->Pipeline);
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static int GetMP3PlayedTime(T_PlayerHandle ap)
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

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

static int GetWAVPlayedTime(T_PlayerHandle ap)
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

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

static esp_err_t RecordWAV(T_RecorderHandle ap, const char* url)
{
  ESP_LOGE(Tag, "COUCOU STOP WAV FROM RECORD");
  StopWAVRecord(ap);

  if (url)
  {
    ESP_LOGE(Tag, "COUCOU RECORD WAV");
    i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);
    audio_element_set_uri(ap->SPIFFSStream, url);
    audio_pipeline_run(ap->Pipeline);
    ap->Recording = true;
    start_rec_time = esp_timer_get_time();
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopMP3(T_PlayerHandle ap)
{
  if (ap->Playing)
  {
	  //ESP_LOGE(Tag, "StopMP3");

    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->Mp3Decoder);
    audio_element_reset_state(ap->SPIFFSStream);
    //audio_element_reset_state(ap->Filter);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Playing = false;

/*
	  audio_pipeline_stop(ap->Pipeline);
	  audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_elements(ap->Pipeline);
      audio_pipeline_change_state(ap->Pipeline, AEL_STATE_INIT);
      ap->Playing = false;
      */
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopWAV(T_PlayerHandle ap)
{
  //ESP_LOGE(Tag, "COUCOU STOP WAV");

  if (ap->Playing)
  {
    audio_pipeline_stop(ap->Pipeline);
    audio_pipeline_wait_for_stop(ap->Pipeline);
    audio_element_reset_state(ap->SPIFFSStream);
    //audio_element_reset_state(ap->Filter);
    audio_element_reset_state(ap->Mp3Decoder);
    audio_element_reset_state(ap->I2SStream);
    audio_pipeline_reset_ringbuffer(ap->Pipeline);
    audio_pipeline_reset_items_state(ap->Pipeline);
    audio_pipeline_terminate(ap->Pipeline);
    ap->Playing = false;
  }

  return ESP_OK;
}

//_____________________________________________________________________________

static esp_err_t StopWAVRecord(T_RecorderHandle ap)
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


