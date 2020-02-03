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

#define DEFAULT_ESP_PERIPH_SET_CONFIG() {                  \
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
  audio_hal_handle_t audio_hal;  // Audio hardware abstract layer handle
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
typedef struct MP3AudioPlayer*          T_MP3PlayerHandle;
typedef struct WAVAudioPlayer*          T_WAVPlayerHandle;
typedef struct WAVAudioRecorder*        T_WAVRecorderHandle;

typedef struct MP3AudioPlayerFromFlash
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t Decoder;
  audio_event_iface_handle_t Evt;
  audio_hal_handle_t Hal;
  bool Run;
  bool Playing;
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
} T_WAVRecorder;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

extern const uint8_t magic_44100_mp3_start[]     asm("_binary_magic_44100_mp3_start");
extern const uint8_t magic_44100_mp3_end[]       asm("_binary_magic_44100_mp3_end");

extern const uint8_t tick_44100_mp3_start[]      asm("_binary_tick_44100_mp3_start");
extern const uint8_t tick_44100_mp3_end[]        asm("_binary_tick_44100_mp3_end");

extern const uint8_t blop_44100_mp3_start[]      asm("_binary_blop_44100_mp3_start");
extern const uint8_t blop_44100_mp3_end[]        asm("_binary_blop_44100_mp3_end");

extern const uint8_t fall_44100_mp3_start[]      asm("_binary_fall_44100_mp3_start");
extern const uint8_t fall_44100_mp3_end[]        asm("_binary_fall_44100_mp3_end");

extern const uint8_t detect_44100_mp3_start[]    asm("_binary_detect_44100_mp3_start");
extern const uint8_t detect_44100_mp3_end[]      asm("_binary_detect_44100_mp3_end");

extern const uint8_t bye_44100_mp3_start[]       asm("_binary_bye_44100_mp3_start");
extern const uint8_t bye_44100_mp3_end[]         asm("_binary_bye_44100_mp3_end");

extern const uint8_t c3_44100_mp3_start[]       asm("_binary_c3_44100_mp3_start");
extern const uint8_t c3_44100_mp3_end[]         asm("_binary_c3_44100_mp3_end");

extern const uint8_t d3_44100_mp3_start[]       asm("_binary_d3_44100_mp3_start");
extern const uint8_t d3_44100_mp3_end[]         asm("_binary_d3_44100_mp3_end");

extern const uint8_t e3_44100_mp3_start[]       asm("_binary_e3_44100_mp3_start");
extern const uint8_t e3_44100_mp3_end[]         asm("_binary_e3_44100_mp3_end");

extern const uint8_t f3_44100_mp3_start[]       asm("_binary_f3_44100_mp3_start");
extern const uint8_t f3_44100_mp3_end[]         asm("_binary_f3_44100_mp3_end");

extern const uint8_t g3_44100_mp3_start[]       asm("_binary_g3_44100_mp3_start");
extern const uint8_t g3_44100_mp3_end[]         asm("_binary_g3_44100_mp3_end");

extern const uint8_t a3_44100_mp3_start[]       asm("_binary_a3_44100_mp3_start");
extern const uint8_t a3_44100_mp3_end[]         asm("_binary_a3_44100_mp3_end");

extern const uint8_t b3_44100_mp3_start[]       asm("_binary_b3_44100_mp3_start");
extern const uint8_t b3_44100_mp3_end[]         asm("_binary_b3_44100_mp3_end");

#if 0
extern const uint8_t bell_mp3_start[]       asm("_binary_bell_mp3_start");
extern const uint8_t bell_mp3_end[]         asm("_binary_bell_mp3_end");

extern const uint8_t blopblop_mp3_start[]      asm("_binary_blopblop_mp3_start");
extern const uint8_t blopblop_mp3_end[]        asm("_binary_blopblop_mp3_end");

extern const uint8_t tuck_mp3_start[]      asm("_binary_tuck_mp3_start");
extern const uint8_t tuck_mp3_end[]        asm("_binary_tuck_mp3_end");

extern const uint8_t blop2_mp3_start[]      asm("_binary_blop2_mp3_start");
extern const uint8_t blop2_mp3_end[]        asm("_binary_blop2_mp3_end");

extern const uint8_t tick_8000_mp3_start[]      asm("_binary_tick_8000_mp3_start");
extern const uint8_t tick_8000_mp3_end[]        asm("_binary_tick_8000_mp3_end");

extern const uint8_t tick_11025_mp3_start[]      asm("_binary_tick_11025_mp3_start");
extern const uint8_t tick_11025_mp3_end[]        asm("_binary_tick_11025_mp3_end");

extern const uint8_t tick_16000_mp3_start[]      asm("_binary_tick_16000_mp3_start");
extern const uint8_t tick_16000_mp3_end[]        asm("_binary_tick_16000_mp3_end");

extern const uint8_t tick_22050_mp3_start[]      asm("_binary_tick_22050_mp3_start");
extern const uint8_t tick_22050_mp3_end[]        asm("_binary_tick_22050_mp3_end");

extern const uint8_t tick_32000_mp3_start[]      asm("_binary_tick_32000_mp3_start");
extern const uint8_t tick_32000_mp3_end[]        asm("_binary_tick_32000_mp3_end");

extern const uint8_t tick_48000_mp3_start[]      asm("_binary_tick_48000_mp3_start");
extern const uint8_t tick_48000_mp3_end[]        asm("_binary_tick_48000_mp3_end");
#endif

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
static int16_t FileIndexFromFlash = -1;
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

static audio_board_handle_t BoardHandle = 0;

static T_MP3PlayerFromFlashHandle MP3PlayerFromFlash = NULL;
static T_MP3PlayerHandle          MP3Player          = NULL;
static T_WAVPlayerHandle          WAVPlayer          = NULL;
static T_WAVRecorderHandle        WAVRecorder        = NULL;

static int16_t buffer[4 * BUF_SIZE];

static bool ClosingSoundIsFinished = false;

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the board
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static audio_board_handle_t InitBoard(void);

//! \brief     Initialize the codec
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static audio_hal_handle_t InitCodec(void);

//! \brief     Initialize the SPI file system
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static void InitSPIFFS(void);

//! \brief     Initialize the MP3 player from flash
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_MP3PlayerFromFlashHandle InitMP3PlayerFromFlash(void);

//! \brief     Initialize the MP3 player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_MP3PlayerHandle InitMP3Player(void);

//! \brief     Initialize the WAV player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_WAVPlayerHandle InitWAVPlayer(void);

//! \brief     Initialize the WAV recorder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_WAVRecorderHandle InitWAVRecorder(void);

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

//! \brief     Generate the master clock (MCLK)
//! \pre       First initialize the codec
//! \param     clock_Hz - Clock frequency in [Hz]
//! \return    None
static void GenerateMasterClock(uint32_t clock_Hz);

//! \brief     Select the file (from/to the flash)
//! \pre       First initialize the codec
//! \param     index - Index of the selected file
//! \return    None
static void SelectFile(T_SystemSound index);

//! \brief     Run the MP3 player (from the flash) task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunMP3PlayerFromFlashTask(void* arg);

//! \brief     Run the MP3 player (from the SPI file system) task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunMP3PlayerTask(void* arg);

//! \brief     Run the WAV player (from the SPI file system) task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunWAVPlayerTask(void* arg);

//! \brief     Run the WAV recorder (to the SPI file system) task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunWAVRecorderTask(void* arg);

//! \brief     Play a MP3 file (from the flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

//! \brief     Play a MP3 file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayMP3(T_MP3PlayerHandle ap, const char* url);

//! \brief     Play a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PlayWAV(T_WAVPlayerHandle ap, const char* url);

//! \brief     Pause a MP3 file (from the flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PauseMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

//! \brief     Pause a MP3 file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PauseMP3(T_MP3PlayerHandle ap);

//! \brief     Pause a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t PauseWAV(T_WAVPlayerHandle ap);

//! \brief     Resume a MP3 file (from the flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t ResumeMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

//! \brief     Resume a MP3 file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t ResumeMP3(T_MP3PlayerHandle ap);

//! \brief     Resume a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t ResumeWAV(T_WAVPlayerHandle ap);

//! \brief     Get the played time [s/10] of a MP3 file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static int GetMP3PlayedTime(T_MP3PlayerHandle ap);

//! \brief     Get the played time [s/10] of a WAV file  (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static int GetWAVPlayedTime(T_WAVPlayerHandle ap);

//! \brief     Record a WAV file (to the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t RecordWAV(T_WAVRecorderHandle ap, const char* url);

//! \brief     Stop playing a MP3 file (from the flash)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopMP3FromFlash(T_MP3PlayerFromFlashHandle ap);

//! \brief     Stop playing a MP3 file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopMP3(T_MP3PlayerHandle ap);

//! \brief     Stop playing a WAV file (from the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopWAV(T_WAVPlayerHandle ap);

//! \brief     Stop recording a WAV file (to the SPI file system)
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static esp_err_t StopWAVRecord(T_WAVRecorderHandle ap);

int mp3_music_read_cb(audio_element_handle_t el, char* buf, int len, TickType_t wait_time, void* ctx);

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
  BoardHandle = InitBoard();
  audio_hal_ctrl_codec(BoardHandle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

//#if 0
  ESP_LOGE(Tag, "INIT MP3 PLAYER FROM FLASH");
  MP3PlayerFromFlash = InitMP3PlayerFromFlash();
//#endif

//#if 0
  ESP_LOGE(Tag, "INIT MP3 PLAYER");
  MP3Player = InitMP3Player();
//#endif

#if 0
  ESP_LOGE(Tag, "INIT WAV PLAYER");
  WAVPlayer = InitWAVPlayer();
#endif

#if 0
  ESP_LOGE(Tag, "INIT WAV RECORDER");
  WAVRecorder = InitWAVRecorder();
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
}

//_____________________________________________________________________________

void Codec_PlayMP3FileFromFlash(T_SystemSound index)
{
  FileIndexFromFlash = index;

  SelectFile(FileIndexFromFlash);

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

bool Codec_IsClosingSoundFinished(void)
{
  return ClosingSoundIsFinished;
}

//_____________________________________________________________________________

static audio_board_handle_t InitBoard(void)
{
  if (BoardHandle)
  {
    ESP_LOGW(Tag, "The board has already been initialized!");
    return BoardHandle;
  }

  BoardHandle = (audio_board_handle_t) audio_calloc(1, sizeof(struct audio_board_handle));
  AUDIO_MEM_CHECK(Tag, BoardHandle, return NULL);
  BoardHandle->audio_hal = InitCodec();

  return BoardHandle;
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

static T_MP3PlayerFromFlashHandle InitMP3PlayerFromFlash(void)
{
  T_MP3PlayerFromFlashHandle ap = calloc(1, sizeof(T_MP3PlayerFromFlash));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
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
  audio_pipeline_link(ap->Pipeline, (const char* [])
  {"mp3_flash_decoder", "i2s_writer"
  }, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;

  if (xTaskCreatePinnedToCore(
        RunMP3PlayerFromFlashTask,
        "sys_player",
        DEFAULT_PLAYER_TASK_STACK,
        ap,
        DEFAULT_PLAYER_TASK_PRIO,
        &MP3PlayerFromFlashTask,
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

static T_MP3PlayerHandle InitMP3Player(void)
{
  T_MP3PlayerHandle ap = calloc(1, sizeof(T_MP3Player));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
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
  audio_pipeline_link(ap->Pipeline, (const char* [])
  {"mp3_file_reader", "mp3_decoder", "i2s_writer"
  }, 3);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Playing = false;

  if (xTaskCreatePinnedToCore(
        RunMP3PlayerTask,
        "mp3_player",
        DEFAULT_PLAYER_TASK_STACK,
        ap,
        DEFAULT_PLAYER_TASK_PRIO,
        &MP3PlayerTask,
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

static T_WAVPlayerHandle InitWAVPlayer(void)
{
  T_WAVPlayerHandle ap = calloc(1, sizeof(T_WAVPlayer));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
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
  ap->Filter = CreateFilter(SAVE_FILE_RATE, SAVE_FILE_CHANNEL, WAV_PLAYER_RATE, WAV_PLAYER_CHANNEL,
                            AUDIO_CODEC_TYPE_DECODER);
  AUDIO_MEM_CHECK(Tag, ap->Filter, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.4] Create I2S stream to write audio data to codec chip");
  ap->I2SStream = CreateI2SStream(WAV_PLAYER_RATE, WAV_PLAYER_BITS, WAV_PLAYER_CHANNEL, AUDIO_STREAM_WRITER);

  ESP_LOGI(Tag, "[2.5] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->SPIFFSStream, "file_reader");
  audio_pipeline_register(ap->Pipeline, ap->Decoder, "wav_decoder");
  audio_pipeline_register(ap->Pipeline, ap->Filter, "filter_upsample");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.6] Link it together [flash]-->spiffs_stream-->wav_decoder-->filter-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char* [])
  {"file_reader", "wav_decoder", "filter_upsample", "i2s_writer"
  }, 4);
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

  if (xTaskCreatePinnedToCore(
        RunWAVPlayerTask,
        "wav_player",
        DEFAULT_PLAYER_TASK_STACK,
        ap,
        DEFAULT_PLAYER_TASK_PRIO,
        &WAVPlayerTask,
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

static T_WAVRecorderHandle InitWAVRecorder(void)
{
  T_WAVRecorderHandle ap = calloc(1, sizeof(T_WAVRecorder));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1] Start audio codec chip");
  ap->Hal = BoardHandle->audio_hal;
  AUDIO_MEM_CHECK(Tag, ap->Hal, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.0] Create audio pipeline for record");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _audio_init_failed);

  ESP_LOGI(Tag, "[2.1] Create I2S stream to read audio data from codec chip");
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
  audio_pipeline_link(ap->Pipeline, (const char* [])
  {"i2s_reader", "filter_downsample", "wav_encoder", "file_writer"
  }, 4);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from peripherals");
  audio_event_iface_set_listener(esp_periph_set_get_event_iface(Set), ap->Evt);

  ESP_LOGI(Tag, "[3.2] Listening event from pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->Recording = false;

  if (xTaskCreatePinnedToCore(
        RunWAVRecorderTask,
        "recorder",
        DEFAULT_RECORDER_TASK_STACK,
        ap,
        DEFAULT_RECORDER_TASK_PRIO,
        &WAVRecorderTask,
        0) != pdTRUE)
  {
    ESP_LOGE(Tag, "Error creating the Recorder task");
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
  mem_assert(i2s_stream);

  audio_element_info_t i2s_info = {0};
  audio_element_getinfo(i2s_stream, &i2s_info);
  i2s_info.bits = bits;
  i2s_info.channels = channels;
  i2s_info.sample_rates = sampleRates;
  audio_element_setinfo(i2s_stream, &i2s_info);

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
  rsp_cfg.type = type;

  return rsp_filter_init(&rsp_cfg);
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

static audio_element_handle_t CreateWAVEncoder(void)
{
  wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();

  return wav_encoder_init(&wav_cfg);
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

static void SelectFile(T_SystemSound index)
{
  switch (index)
  {
    case 0:
      File.Start = magic_44100_mp3_start;
      File.End   = magic_44100_mp3_end;
      break;

    case 1:
      File.Start = tick_44100_mp3_start;
      File.End   = tick_44100_mp3_end;
      break;

    case 2:
      File.Start = blop_44100_mp3_start;
      File.End   = blop_44100_mp3_end;
      break;

    case 3:
      File.Start = fall_44100_mp3_start;
      File.End   = fall_44100_mp3_end;
      break;

    case 4:
      File.Start = detect_44100_mp3_start;
      File.End   = detect_44100_mp3_end;
      break;

    case 5:
      File.Start = bye_44100_mp3_start;
      File.End   = bye_44100_mp3_end;
      break;

    case 6:
      File.Start = c3_44100_mp3_start;
      File.End   = c3_44100_mp3_end;
      break;

    case 7:
      File.Start = d3_44100_mp3_start;
      File.End   = d3_44100_mp3_end;
      break;

    case 8:
      File.Start = e3_44100_mp3_start;
      File.End   = e3_44100_mp3_end;
      break;

    case 9:
      File.Start = f3_44100_mp3_start;
      File.End   = f3_44100_mp3_end;
      break;

    case 10:
      File.Start = g3_44100_mp3_start;
      File.End   = g3_44100_mp3_end;
      break;

    case 11:
      File.Start = a3_44100_mp3_start;
      File.End   = a3_44100_mp3_end;
      break;

    case 12:
      File.Start = b3_44100_mp3_start;
      File.End   = b3_44100_mp3_end;
      break;

#if 0
    case 6:
      File.Start = bell_mp3_start;
      File.End   = bell_mp3_end;
      break;

    case 7:
      File.Start = blopblop_mp3_start;
      File.End   = blopblop_mp3_end;
      break;

    case 8:
      File.Start = tuck_mp3_start;
      File.End   = tuck_mp3_end;
      break;

    case 9:
      File.Start = blop2_mp3_start;
      File.End   = blop2_mp3_end;
      break;

    case 6:
      File.Start = tick_8000_mp3_start;
      File.End   = tick_8000_mp3_end;
      break;

    case 7:
      File.Start = tick_11025_mp3_start;
      File.End   = tick_11025_mp3_end;
      break;

    case 8:
      File.Start = tick_16000_mp3_start;
      File.End   = tick_16000_mp3_end;
      break;

    case 9:
      File.Start = tick_22050_mp3_start;
      File.End   = tick_22050_mp3_end;
      break;

    case 10:
      File.Start = tick_32000_mp3_start;
      File.End   = tick_32000_mp3_end;
      break;

    case 11:
      File.Start = tick_48000_mp3_start;
      File.End   = tick_48000_mp3_end;
      break;
#endif

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
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
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
      audio_pipeline_stop(ap->Pipeline);
      audio_pipeline_wait_for_stop(ap->Pipeline);
      audio_element_reset_state(ap->Decoder);
      audio_element_reset_state(ap->I2SStream);
      audio_pipeline_reset_ringbuffer(ap->Pipeline);
      audio_pipeline_reset_items_state(ap->Pipeline);
      ap->Playing = false;
      //audio_pipeline_terminate(ap->Pipeline);

      if (FileIndexFromFlash == 5)
      {
        ClosingSoundIsFinished = true;
      }
      else
      {
        ClosingSoundIsFinished = false;
      }
    }
  }

  vTaskDelete(MP3PlayerFromFlashTask);
}

//_____________________________________________________________________________

static void RunMP3PlayerTask(void* arg)
{
  T_MP3PlayerHandle ap = (T_MP3PlayerHandle) arg;

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
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
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
      continue;
    }

    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT
        && msg.source == (void*) ap->Decoder
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (int)msg.data == AEL_STATUS_STATE_PAUSED)
    {
      //ESP_LOGE(Tag, "[ * ] COUCOU SEND PAUSE");
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

  i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);

  int second_recorded = 0;
  while (ap->Run)
  {
    audio_event_iface_msg_t msg;

    if (audio_event_iface_listen(ap->Evt, &msg, 1000 / portTICK_RATE_MS) != ESP_OK)
    {
      ESP_LOGE(Tag, "RUN RUN");
      if (ap->Recording)
      {
        second_recorded++;

        ESP_LOGE(Tag, "[ * ] Recording ... %d", second_recorded);

        if (second_recorded >= RecordingDuration_s)
        {
          //break;
          StopWAVRecord(ap);
        }
      }

      continue;
    }
#if 0
    /* Stop when the last pipeline element (i2s_stream_reader in this case) receives stop event */
    if (msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT && msg.source == (void*) ap->I2SStream
        && msg.cmd == AEL_MSG_CMD_REPORT_STATUS
        && (((int)msg.data == AEL_STATUS_STATE_STOPPED) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
      ESP_LOGE(Tag, "[ * ] Stop event received");
      break;
    }
#endif
  }

  ESP_LOGE(Tag, "STOP STOP");
  //StopWAVRecord(ap);

  vTaskDelete(WAVRecorderTask);
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

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
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

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
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
