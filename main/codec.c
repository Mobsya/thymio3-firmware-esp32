//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    codec.c
//! \brief   This module provides the useful functions to use the audio codec.
//!           A pipeline for each type of sound processing is created, in total
//!           4 pipelines are used:
//!           - pre-built sounds saved in flash: Flash => MP3 decoder => I2S out stream
//!           - wav player: RAM => WAV decoder => I2S out stream
//!           - mp3 player: RAM => MP3 decoder => I2S out stream
//!           - recoerder: I2S in stream => WAV encoder => RAM
//!
//! \author  Vincent Gonet, Stefano Morgani
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
#include "freertos/semphr.h"

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
#include "esp_peripherals.h"
#include "codec.h"
#include "thymio_es8374.h"
#include "file_system.h"
#include "pins_def.h"
#include "board.h"
#include "audio_tone_uri.h"
#include "sensors.h"
#include "utility.h"
#include "wav_head.h"
#include "settings.h"
#include "behavior.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MP3_PLAYER_RATE 12000
#define MP3_PLAYER_CHANNEL 1 //!< Mono = 1
#define MP3_PLAYER_BITS 16

#define RECORD_RATE 12000
#define RECORD_CHANNEL 1 //!< Mono = 1
#define RECORD_BITS 16

#define WAV_PLAYER_RATE 12000
#define WAV_PLAYER_CHANNEL 1 //!< Mono = 1
#define WAV_PLAYER_BITS 16

#define TONE_PLAYER_RATE 12000
#define TONE_PLAYER_CHANNEL 1 //!< Mono = 1
#define TONE_PLAYER_BITS 16
#define TONE_FRAME_SAMPLES 1024
#define TONE_AMPLITUDE 7000
#define TONE_MAX_FREQ 3000.0f

#define DEFAULT_AUDIO_TASK_STACK (4 * 1024)
#define DEFAULT_AUDIO_TASK_PRIO (5)

// #define BUF_SIZE (SAVE_FILE_RATE * 1) /* 2 second buffer */

#define STATE_RUNNING 0
#define STATE_PAUSED 1
#define STATE_ALMOST_STOPPED 2
#define STATE_STOPPED 3


#define PLAY_NEAR_END_THR 4800 // 12 KHz sampling rae * 2 bytes per sample * 1 second = 24000 bytes/sec => 200 ms = 24000/5 = 4800

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct AudioMp3Player *T_Mp3PlayerHandle;
typedef struct AudioMp3Player
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t Mp3Decoder;
  audio_element_handle_t I2SStream;
  audio_event_iface_handle_t Evt;
  bool Run;      // Indicates if the pipeline event handling task is running
  uint8_t state; // Running (playing), paused
  bool Played;
} T_Mp3Player;

typedef struct AudioOnboardPlayer *T_OnboardPlayerHandle;
typedef struct AudioOnboardPlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t FlashToneStream;
  audio_element_handle_t Mp3Decoder;
  audio_element_handle_t I2SStream;
  audio_event_iface_handle_t Evt;
  bool Run;      // Indicates if the pipeline event handling task is running
  uint8_t state; // Running (playing), paused
  bool Played;
} T_OnboardPlayer;

typedef struct AudioWavPlayer *T_WavPlayerHandle;
typedef struct AudioWavPlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t WavDecoder;
  audio_element_handle_t I2SStream;
  audio_event_iface_handle_t Evt;
  bool Run;      // Indicates if the pipeline event handling task is running
  uint8_t state; // Running (playing), paused
  bool Played;
} T_WavPlayer;

typedef struct
{
    float freq_Hz;        // Already clamped to [0, TONE_MAX_FREQ]
    uint32_t tot_samples; // Note duration converted to samples; 0 means play forever
} tone_note_t;

typedef struct {
    int16_t *buffer;
    // --- Shared with the caller task, protected by "ToneMutex" ---
    tone_note_t notes[TONE_MELODY_MAX_NOTES];
    uint8_t num_notes;       // Number of valid notes in "notes"
    uint32_t seq;            // Incremented each time the melody is replaced
    // --- Owned by the tone generator task only, no protection needed ---
    uint32_t played_seq;     // Sequence number of the melody currently being played
    uint8_t current_note;    // Index of the note currently played
    uint32_t samples_played; // Samples already played of the current note
    float phase;
} tone_melody_t;

int16_t tone_buffer[TONE_FRAME_SAMPLES] = {0};
typedef struct AudioTonePlayer *T_TonePlayerHandle;
typedef struct AudioTonePlayer
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t ToneGenerator;
  audio_element_handle_t I2SStream;
  audio_event_iface_handle_t Evt;
  bool Run;      // Indicates if the pipeline event handling task is running
  uint8_t state; // Running (playing), paused
  bool Played;
  tone_melody_t *melody;
} T_TonePlayer;

typedef struct AudioRecorder *T_RecorderHandle;
typedef struct AudioRecorder
{
  audio_pipeline_handle_t Pipeline;
  audio_element_handle_t I2SStream;
  audio_element_handle_t Encoder;
  audio_event_iface_handle_t Evt;
  bool Run;      // Indicates if the pipeline event handling task is running
  uint8_t state; // Running (recording), paused
  bool Recorded;
} T_Recorder;

typedef enum
{
  E_SoundStatus_Started,
  E_SoundStatus_Finished
} T_SoundStatus;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char *Tag = "codec";

static int16_t OnboardSoundIndex = 0; //-1;
static uint16_t RecordingDuration_s = 0;

static audio_board_handle_t BoardHandle = 0;

static T_Mp3PlayerHandle Mp3Player = NULL;
static T_RecorderHandle Recorder = NULL;
static T_OnboardPlayerHandle OnboardPlayer = NULL;
static T_WavPlayerHandle WavPlayer = NULL;
static T_TonePlayerHandle TonePlayer = NULL;
// Protects the melody shared between the caller task and the tone generator task. A mutex is
// enough because both sides run in task context: unlike a critical section, it does not disable
// the interrupts. Never call "Codec_PlayToneMelody" from an ISR.
static SemaphoreHandle_t ToneMutex = NULL;

// static int16_t buffer[4 * BUF_SIZE];

uint8_t *recordBuffer;
uint32_t recordBufferIndex = 44; // First 44 bytes are reserved for wav header
uint32_t playerBufferIndex = 0;
uint8_t *playerBuffer;
uint32_t playerBufferSize = 0;

static T_SoundStatus SoundStatus[TONE_TYPE_MAX];
int64_t start_rec_time, end_rec_time;

extern bool printStats;

#define MY_DEFAULT_MP3_DECODER_CONFIG()         \
  {                                             \
    .out_rb_size = MP3_DECODER_RINGBUFFER_SIZE, \
    .task_stack = MP3_DECODER_TASK_STACK_SIZE,  \
    .task_core = MP3_DECODER_TASK_CORE,         \
    .task_prio = MP3_DECODER_TASK_PRIO,         \
    .stack_in_ext = false,                      \
  }

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the MP3 player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_Mp3PlayerHandle InitMp3Player(void);

//! \brief     Initialize the pre-built sounds player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_OnboardPlayerHandle InitOnboardPlayer(void);

//! \brief     Initialize the WAV player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_WavPlayerHandle InitWavPlayer(void);

//! \brief     Initialize the TONE player
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_TonePlayerHandle InitTonePlayer(void);

//! \brief     Initialize the WAV recorder
//! \pre       First initialize the codec
//! \param     None
//! \return    None
static T_RecorderHandle InitRecorder(void);

//! \brief     Select the file (from/to the flash)
//! \pre       First initialize the codec
//! \param     index - Index of the selected file
//! \return    None
static void SelectFile(tone_type_t index);

//! \brief     Run the recorder task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunRecordTask(void *arg);

//! \brief     Run the pre-built sounds task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunOnboardPlayerTask(void *arg);

//! \brief     Run the wav player task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunWavPlayerTask(void *arg);

//! \brief     Run the mp3 player task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunMp3PlayerTask(void *arg);

//! \brief     Run the tone player task
//! \pre       First initialize the codec
//! \param     arg - Task parameter
//! \return    None
static void RunTonePlayerTask(void *arg);

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

  recordBuffer = (uint8_t *)malloc(MAX_RECORD_SIZE + 44); // 12 KHz sampling rate * 2 bytes per sample * 10 seconds + wav header

  ESP_LOGI(Tag, "[2] Start codec chip");
  BoardHandle = audio_board_init();

  // es8374_read_all_Thymio();
  if (BoardHandle->audio_hal != NULL) {
    // SUCCESS: The board is ready to use
    ESP_LOGI(Tag, "Audio board initialized successfully!");
  } else {
    // FAILURE: Something went wrong (likely out of memory)
    ESP_LOGE(Tag, "Audio board initialization failed!");
    // Handle the error (e.g., stop execution or retry)
    return;
  }

  audio_hal_ctrl_codec(BoardHandle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

  // es8374_read_all_Thymio();

  ESP_LOGI(Tag, "INIT MP3 PLAYER");
  Mp3Player = InitMp3Player();
  ESP_LOGI(Tag, "INIT ONBOARD PLAYER");
  OnboardPlayer = InitOnboardPlayer();
  ESP_LOGI(Tag, "INIT WAV PLAYER");
  WavPlayer = InitWavPlayer();
  ESP_LOGI(Tag, "INIT TONE PLAYER");
  TonePlayer = InitTonePlayer();  
  ESP_LOGI(Tag, "INIT RECORDER");
  Recorder = InitRecorder();

  
}

//_____________________________________________________________________________

int recorder_write_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
  if ((recordBufferIndex + len) > MAX_RECORD_SIZE)
  {
    len = MAX_RECORD_SIZE - recordBufferIndex;
    printf("Max recording size reached!!!\n");
  }
  memcpy(&recordBuffer[recordBufferIndex], buffer, len);
  recordBufferIndex += len;
  ESP_LOGD(Tag, "writecb, ind=%d, len=%d\n", recordBufferIndex, len);
  return len;
}

int player_read_cb(audio_element_handle_t self, char *buffer, int len, TickType_t ticks_to_wait, void *context)
{
  if (playerBufferIndex == playerBufferSize)
  {
    // printf("READ_CB DONE!!!\n");
    //  Change immediately the state of the players instead of waiting the related "finished" event.
    //  This is to avoid conflicts between delayed events and actual pipeline state.
    //  For example: 1) play end => read cb "finish" 2) user pause 3) "finish event" not queued, only "paused event" received => pipeline state lost.
    if(WavPlayer->state != STATE_STOPPED) { // If WavPlayer running
      WavPlayer->state = STATE_ALMOST_STOPPED;
    }
    if(Mp3Player->state != STATE_STOPPED) { // If Mp3Player running
      Mp3Player->state = STATE_ALMOST_STOPPED;
    }
    return AEL_IO_DONE;
  }
  if ((playerBufferIndex + len) > playerBufferSize)
  {
    len = playerBufferSize - playerBufferIndex;
  }
  memcpy(buffer, &playerBuffer[playerBufferIndex], len);
  playerBufferIndex += len;
  // printf("buffInd=%d, buffSize=%d, len=%d\n", playerBufferIndex, playerBufferSize, len);
  // printf("wav_queue=%d\n", uxQueueSpacesAvailable(audio_event_iface_get_msg_queue_handle(WavPlayer->Evt)));
  // printf("mp3_queue=%d\n", uxQueueSpacesAvailable(audio_event_iface_get_msg_queue_handle(Mp3Player->Evt)));
  ESP_LOGD(Tag, "read_cb, len=%d\n", len);
  return len;
}

bool Codec_IsRecordFinished(void)
{
  if (Recorder->Recorded)
  {
    Recorder->Recorded = false;
    return true;
  }
  else
  {
    return false;
  }
}

uint8_t *Codec_GetRecordPtr(void)
{
  return recordBuffer;
}

uint32_t Codec_GetRecordSize(void)
{
  return recordBufferIndex;
}

//_____________________________________________________________________________
uint32_t Codec_CreateWAVFile(int16_t *buffer, int16_t freq_Hz, uint16_t msec)
{
  float amplitude = 2000;
  float phase = 0;
  float freq_radians_per_sample = ((freq_Hz * 2 * M_PI) / WAV_PLAYER_RATE);
  uint32_t num_samples = (uint32_t)(12*msec); //(WAV_PLAYER_RATE*msec)/1000 => 12*msec  
  buffer = (int16_t *)malloc(num_samples*2+44); // num samples * 2 bytes per sample + wav header
  //printf("allocated %d\r\n", num_samples*2+44);

  // Fill buffer with a sine wave
  for (uint16_t i = 0u; i < num_samples; i++)
  {
    phase += freq_radians_per_sample;
    buffer[44+i] = (int16_t)(amplitude * sin(phase)); // Initial 44 bytes reserved for wav header
  }

  //printf("sizeof wav_header_t=%d\r\n", sizeof(wav_header_t));
  wav_header_t *wav_info = (wav_header_t *)malloc(sizeof(wav_header_t));
  wav_head_init(wav_info, 12000, 16, 1);
  wav_head_size(wav_info, (uint32_t)num_samples*2);
  memcpy(buffer, wav_info, sizeof(wav_header_t));
  free(wav_info);

  /*
  wav_header_t wav_info; // = (wav_header_t *)malloc(44); //sizeof(wav_header_t));
  wav_head_init(&wav_info, 12000, 16, 1);
  wav_head_size(&wav_info, (uint32_t)num_samples*2);
  memcpy(buffer, &wav_info, 44); //sizeof(wav_header_t));  
  */

  return (num_samples*2+44);
}

esp_err_t Codec_PlayOnboardSound(tone_type_t index)
{
  esp_err_t err = 0;
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }
  // return; // Used for debugging in order to not use the player.

  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED) || (TonePlayer->state != STATE_STOPPED))
  { // Finish previous play/recording before starting another one.
    return ESP_FAIL;
  }

  OnboardSoundIndex = index;
  SelectFile(OnboardSoundIndex);

  if (OnboardPlayer->Run)
  {
    err = audio_pipeline_resume(OnboardPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }
  else
  { // The first time, start the pipeline events handling task and run the pipeline (start elements tasks)
    OnboardPlayer->Run = true;
    xTaskCreatePinnedToCore(RunOnboardPlayerTask, "onboard_player", DEFAULT_AUDIO_TASK_STACK, OnboardPlayer, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);
    err = audio_pipeline_run(OnboardPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

esp_err_t Codec_PlayMP3File(uint8_t *mp3, uint32_t num_bytes)
{
  esp_err_t err = 0;
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }
  //printf("OnboardPlayer state = %d, WavPlayer state = %d, Mp3Player state = %d, Recorder state = %d\n", OnboardPlayer->state, WavPlayer->state, Mp3Player->state, Recorder->state);

  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED) || (TonePlayer->state != STATE_STOPPED))
  { // Finish previous play/recording before starting another one.
    return ESP_FAIL;
  }

  playerBufferIndex = 0;
  playerBuffer = mp3;
  playerBufferSize = num_bytes;

  if (Mp3Player->Run)
  {
    err = audio_pipeline_resume(Mp3Player->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }
  else
  { // The first time, start the pipeline events handling task and run the pipeline (start elements tasks)
    Mp3Player->Run = true;
    xTaskCreatePinnedToCore(RunMp3PlayerTask, "mp3_player", DEFAULT_AUDIO_TASK_STACK, Mp3Player, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);
    err = audio_pipeline_run(Mp3Player->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t Codec_PlayWAVFile(uint8_t *wav, uint32_t num_bytes)
{
  esp_err_t err = 0;
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }  
  // return; // Used for debugging
  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED) || (TonePlayer->state != STATE_STOPPED))
  { // Finish previous play/recording before starting another one.
    return ESP_FAIL;
  }

  playerBufferIndex = 0;
  playerBuffer = wav;
  playerBufferSize = num_bytes;

  if (WavPlayer->Run)
  {
    err = audio_pipeline_resume(WavPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }
  else
  { // The first time, start the pipeline events handling task and run the pipeline (start elements tasks)
    WavPlayer->Run = true;
    xTaskCreatePinnedToCore(RunWavPlayerTask, "wav_player", DEFAULT_AUDIO_TASK_STACK, WavPlayer, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);
    err = audio_pipeline_run(WavPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

esp_err_t Codec_PlayRecorded(void)
{
  esp_err_t err = 0;
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }  
  // return; // Used for debugging
  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED) || (TonePlayer->state != STATE_STOPPED))
  { // Finish previous play/recording before starting another one.
    return ESP_FAIL;
  }

  playerBufferIndex = 0;
  playerBuffer = recordBuffer;
  playerBufferSize = recordBufferIndex;

  if (WavPlayer->Run)
  {
    err = audio_pipeline_resume(WavPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }
  else
  { // The first time, start the pipeline events handling task and run the pipeline (start elements tasks)
    WavPlayer->Run = true;
    xTaskCreatePinnedToCore(RunWavPlayerTask, "wav_player", DEFAULT_AUDIO_TASK_STACK, WavPlayer, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);
    err = audio_pipeline_run(WavPlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t Codec_PlayToneMelody(const T_ToneNote *notes, uint8_t num_notes)
{
  esp_err_t err = 0;

  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }

  if ((notes == NULL) || (num_notes == 0) || (num_notes > TONE_MELODY_MAX_NOTES))
  {
    return ESP_ERR_INVALID_ARG;
  }

  // The tone player state is not tested on purpose: if it is already running, the melody
  // is replaced on the fly (this keeps the previous "Codec_PlayTone" behavior).
  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED))
  {
    return ESP_FAIL;
  }

  tone_melody_t *melody = TonePlayer->melody;

  xSemaphoreTake(ToneMutex, portMAX_DELAY);
  for (uint8_t i = 0; i < num_notes; i++)
  {
    float freq = notes[i].freq_Hz;
    if (freq > TONE_MAX_FREQ)
    {
      freq = TONE_MAX_FREQ; // Limit to 3 KHz
    }
    if (freq < 0.0f)
    {
      freq = 0.0f; // Negative frequencies are treated as rests
    }

    // The sample count is computed here, once per melody, to keep the generator loop free of
    // 64-bit divisions. A duration of 0 gives 0 samples, which means "play forever".
    uint64_t tot_samples = ((uint64_t)TONE_PLAYER_RATE * notes[i].duration_ms) / 1000;
    if (tot_samples > UINT32_MAX)
    {
      tot_samples = UINT32_MAX;
    }
    
    melody->notes[i].freq_Hz = freq;
    melody->notes[i].tot_samples = (uint32_t)tot_samples;
  }
  melody->num_notes = num_notes;
  melody->seq++; // The generator restarts from the first note as soon as it sees the new sequence
  xSemaphoreGive(ToneMutex);

  ESP_LOGI(Tag, "Melody start: %u note(s), first one %.1f Hz, %ums", num_notes, notes[0].freq_Hz, notes[0].duration_ms);

  if (TonePlayer->state == STATE_RUNNING)
  {
    return err; // Tone player already running, only the melody has been replaced
  }
  else if (TonePlayer->Run)
  {
    err = audio_pipeline_resume(TonePlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }
  else
  { // The first time, start the pipeline events handling task and run the pipeline (start elements tasks)
    TonePlayer->Run = true;
    xTaskCreatePinnedToCore(RunTonePlayerTask, "tone_player", DEFAULT_AUDIO_TASK_STACK, TonePlayer, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);
    err = audio_pipeline_run(TonePlayer->Pipeline);
    if (err != ESP_OK)
    {
      return err;
    }
  }

  return ESP_OK;
}

//_____________________________________________________________________________

int Codec_GetMP3PlayedTime(void)
{
  audio_element_info_t info;

  if (Mp3Player == NULL)
  {
    return 0;
  }

  if (audio_element_getinfo(Mp3Player->I2SStream, &info) != ESP_OK)
  {
    return 0;
  }

  // int time_sec = info.byte_pos / (info.sample_rates * info.channels * info.bits / 8);
  // printf("Played Time: %d, pos: %lld, rate: %d, channel: %d, bit: %d\n", time_sec, info.byte_pos, info.sample_rates, info.channels, info.bits);

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

int Codec_GetWAVPlayedTime(void)
{
  audio_element_info_t info;

  if (WavPlayer == NULL)
  {
    return 0;
  }

  if (audio_element_getinfo(WavPlayer->I2SStream, &info) != ESP_OK)
  {
    return 0;
  }

  // int time_sec = info.byte_pos / (info.sample_rates * info.channels * info.bits / 8);
  // printf("Played Time: %d, pos: %lld, rate: %d, channel: %d, bit: %d\n", time_sec, info.byte_pos, info.sample_rates, info.channels, info.bits);

  int time_ms = (info.byte_pos * 80) / (info.sample_rates * info.channels * info.bits);
  printf("Played Time: %d\n", time_ms);

  return time_ms;
}

//_____________________________________________________________________________

void Codec_RecordWAVFile(uint16_t duration_s)
{
  if ((OnboardPlayer->state != STATE_STOPPED) || (WavPlayer->state != STATE_STOPPED) || (Mp3Player->state != STATE_STOPPED) || (Recorder->state != STATE_STOPPED) || (TonePlayer->state != STATE_STOPPED))
  { // Finish previous play/recording before starting another one.
    return;
  }
  RecordingDuration_s = duration_s;
  //Sensors_buttons_pause();

  i2s_stream_set_clk(Recorder->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL); // Set the correct recording rate in case it is changed during the play (it should not happen...).

  /*
  // Field duration not used by the audio framework (at least not for the recording)...
  audio_element_info_t writer_info = {0};
  audio_element_getinfo(Recorder->SPIFFSStream, &writer_info);
  writer_info.uri = url;
  writer_info.duration = duration;
  audio_element_setinfo(Recorder->SPIFFSStream, &writer_info);
  */

  Behavior_Disable(B_LED_MIC);
  Behavior_Enable(B_LED_MIC_STATE); // Turn on MIC LED

  //...so we need to measure the time passed manually. This is done inside the recorder event handling task.
  // Once the recording time is elapsed, the task will be terminated.
  xTaskCreatePinnedToCore(RunRecordTask, "record_task", DEFAULT_AUDIO_TASK_STACK, Recorder, DEFAULT_AUDIO_TASK_PRIO, NULL, 0);

  recordBufferIndex = 44; // First 44 bytes are reserved for wav header
  audio_pipeline_run(Recorder->Pipeline);
  start_rec_time = esp_timer_get_time();
}

//_____________________________________________________________________________

void Codec_SetVolume(int16_t volume)
{
  Settings_SetVolumeSettings(volume);
  es8374_codec_set_voice_volume_Thymio(volume);
}

//_____________________________________________________________________________

extern bool Codec_IsSoundFinished(void)
{
  //if ((OnboardPlayer->state == STATE_STOPPED) && (WavPlayer->state == STATE_STOPPED) && (Mp3Player->state == STATE_STOPPED))
  //{
  //  return true; // Return immediately true if no sound played => this wrongly trigger an event in the "micropython Thymio API implementation"
  //}
  //else
  //{
    if (OnboardPlayer->Played || WavPlayer->Played || Mp3Player->Played || TonePlayer->Played) // Only one played at a time can run, so it is possible to test all in the same function
    {
      OnboardPlayer->Played = false;
      WavPlayer->Played = false;
      Mp3Player->Played = false;
      TonePlayer->Played = false;
      return true;
    }
    else
    {
      return false;
    }
  //}
}

//_____________________________________________________________________________

static T_Mp3PlayerHandle InitMp3Player(void)
{
  T_Mp3PlayerHandle ap = calloc(1, sizeof(T_Mp3Player));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _mp3_init_failed);

  ESP_LOGI(Tag, "[1.1] Create MP3 decoder to decode MP3 format");
  mp3_decoder_cfg_t mp3_cfg = MY_DEFAULT_MP3_DECODER_CONFIG();
  ap->Mp3Decoder = mp3_decoder_init(&mp3_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Mp3Decoder, goto _mp3_init_failed);

  ESP_LOGI(Tag, "[1.2] Create I2S stream to write audio data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S; // I2S_COMM_FORMAT_STAND_MSB; //I2S_COMM_FORMAT_STAND_I2S
  i2s_cfg.i2s_config.sample_rate = MP3_PLAYER_RATE;
  // i2s_cfg.task_core = 1;
  // i2s_cfg.i2s_config.fixed_mclk = 4096000;
  //i2s_cfg.stack_in_ext = true;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);
  AUDIO_MEM_CHECK(Tag, ap->I2SStream, goto _mp3_init_failed);

  ESP_LOGI(Tag, "[2.0] Register all elements to audio pipeline");

  audio_pipeline_register(ap->Pipeline, ap->Mp3Decoder, "mp3_decoder");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.1] Link it together [RAM]-->mp3_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]){"mp3_decoder", "i2s_writer"}, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  audio_element_set_read_cb(ap->Mp3Decoder, player_read_cb, NULL);

  ap->Run = false;
  ap->state = STATE_STOPPED;
  ap->Played = false;
  return ap;
_mp3_init_failed:
  return NULL;
}

static T_OnboardPlayerHandle InitOnboardPlayer(void)
{
  T_OnboardPlayerHandle ap = calloc(1, sizeof(T_OnboardPlayer));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _onboard_init_failed);

  ESP_LOGI(Tag, "[1.1] Create Tone stream to read prebuilt sound");
  tone_stream_cfg_t tone_cfg = TONE_STREAM_CFG_DEFAULT();
  tone_cfg.type = AUDIO_STREAM_READER;
  // tone_cfg.task_core = 1;
  tone_cfg.buf_sz = (8 * 1024);
  // tone_cfg.task_stack = (8 * 1024);
  tone_cfg.out_rb_size = (8 * 1024);
  // tone_cfg.task_prio = 5;
  // tone_cfg.extern_stack = true;
  ap->FlashToneStream = tone_stream_init(&tone_cfg);
  AUDIO_MEM_CHECK(Tag, ap->FlashToneStream, goto _onboard_init_failed);

  ESP_LOGI(Tag, "[1.2] Create MP3 decoder to decode MP3 format");
  mp3_decoder_cfg_t mp3_cfg = MY_DEFAULT_MP3_DECODER_CONFIG();
  ap->Mp3Decoder = mp3_decoder_init(&mp3_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Mp3Decoder, goto _onboard_init_failed);

  ESP_LOGI(Tag, "[1.3] Create I2S stream to write audio data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S; // I2S_COMM_FORMAT_STAND_MSB; //I2S_COMM_FORMAT_STAND_I2S
  i2s_cfg.i2s_config.sample_rate = MP3_PLAYER_RATE;
  // i2s_cfg.task_core = 1;
  // i2s_cfg.i2s_config.fixed_mclk = 4096000;
  //i2s_cfg.stack_in_ext = true;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);
  AUDIO_MEM_CHECK(Tag, ap->I2SStream, goto _onboard_init_failed);

  ESP_LOGI(Tag, "[2.0] Register all elements to audio pipeline");

  audio_pipeline_register(ap->Pipeline, ap->FlashToneStream, "flash_tone");
  audio_pipeline_register(ap->Pipeline, ap->Mp3Decoder, "mp3_flash_decoder");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.1] Link it together [flash_tone]-->mp3_flash_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]){"flash_tone", "mp3_flash_decoder", "i2s_writer"}, 3);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = false;
  ap->state = STATE_STOPPED;
  ap->Played = false;
  return ap;
_onboard_init_failed:
  return NULL;
}

static T_WavPlayerHandle InitWavPlayer(void)
{
  T_WavPlayerHandle ap = calloc(1, sizeof(T_WavPlayer));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1.0] Create audio pipeline for playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _wav_init_failed);

  ESP_LOGI(Tag, "[1.1] Create WAV decoder to decode WAV format");
  wav_decoder_cfg_t wav_cfg = DEFAULT_WAV_DECODER_CONFIG();
  wav_cfg.stack_in_ext = false;
  ap->WavDecoder = wav_decoder_init(&wav_cfg);
  AUDIO_MEM_CHECK(Tag, ap->WavDecoder, goto _wav_init_failed);

  ESP_LOGI(Tag, "[1.2] Create I2S stream to write audio data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S; // I2S_COMM_FORMAT_STAND_MSB; //I2S_COMM_FORMAT_STAND_I2S
  i2s_cfg.i2s_config.sample_rate = WAV_PLAYER_RATE;
  // i2s_cfg.task_core = 1;
  //  i2s_cfg.i2s_config.fixed_mclk = 4096000;
  //i2s_cfg.stack_in_ext = true;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);
  AUDIO_MEM_CHECK(Tag, ap->I2SStream, goto _wav_init_failed);

  ESP_LOGI(Tag, "[2.0] Register all elements to audio pipeline");

  audio_pipeline_register(ap->Pipeline, ap->WavDecoder, "wav_decoder");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.1] Link it together [RAM]-->wav_decoder-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]){"wav_decoder", "i2s_writer"}, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  audio_element_set_read_cb(ap->WavDecoder, player_read_cb, NULL);

  audio_element_info_t music_info = {0};
  audio_element_getinfo(ap->WavDecoder, &music_info);
  music_info.bits = WAV_PLAYER_BITS;
  music_info.channels = WAV_PLAYER_CHANNEL;
  music_info.sample_rates = WAV_PLAYER_RATE;
  audio_element_setinfo(ap->WavDecoder, &music_info);

  ap->Run = false;
  ap->state = STATE_STOPPED;
  ap->Played = false;
  return ap;
_wav_init_failed:
  return NULL;
}

static int tone_process(audio_element_handle_t self, char *in_buffer, int in_len)
{
    tone_melody_t *melody = TonePlayer->melody;
    float freq = 0.0f;
    int samples_to_gen = 0;
    bool melody_done = false;
    bool infinite_note = false;

    // Take a consistent snapshot of the note to be played: the melody can be replaced at any time
    // by "Codec_PlayToneMelody", which runs in another task. Only "seq", "num_notes" and "notes"
    // are shared, and the loop below only does integer comparisons, so the lock is held for a very
    // short time. The loop also skips the notes that are already completed, so that the melody is
    // played without any gap between notes.
    xSemaphoreTake(ToneMutex, portMAX_DELAY);
    if (melody->played_seq != melody->seq)
    { // The melody has been replaced, restart from its first note
        melody->played_seq = melody->seq;
        melody->current_note = 0;
        melody->samples_played = 0;
    }
    while (1)
    {
        if (melody->current_note >= melody->num_notes)
        { // All the notes have been played
            melody_done = true;
            break;
        }

        const tone_note_t *note = &melody->notes[melody->current_note];
        freq = note->freq_Hz;

        if (note->tot_samples == 0)
        { // 0 means play forever, a full frame is always generated
            infinite_note = true;
            samples_to_gen = TONE_FRAME_SAMPLES;
            break;
        }

        if (melody->samples_played >= note->tot_samples)
        { // Current note is over, move to the next one (notes shorter than one sample are simply skipped)
            melody->current_note++;
            melody->samples_played = 0;
            continue;
        }

        uint32_t remaining_samples = note->tot_samples - melody->samples_played;
        samples_to_gen = (remaining_samples < (uint32_t)TONE_FRAME_SAMPLES) ? (int)remaining_samples : TONE_FRAME_SAMPLES;
        break;
    }
    xSemaphoreGive(ToneMutex);

    if (melody_done)
    {
        ESP_LOGI(Tag, "Melody finished");
        return AEL_IO_DONE;
    }

    if (freq <= 0.0f)
    { // A frequency of 0 is a rest: output silence and keep the phase at 0, so that the next note starts without any click
        memset(melody->buffer, 0, samples_to_gen * sizeof(int16_t));
        melody->phase = 0;
    }
    else
    { // The phase is intentionally not reset between notes, to avoid a discontinuity (click) on the note change
        float phase_inc = 2 * M_PI * freq / TONE_PLAYER_RATE;
        for (int i = 0; i < samples_to_gen; i++)
        {
            float s = sin(melody->phase) + (1.0/3.0)*sin(melody->phase*3) + (1.0/5.0)*sin(melody->phase*5); // soft square wave to avoid "ringing" effect
            melody->buffer[i] = (int16_t)(TONE_AMPLITUDE * s);
            melody->phase += phase_inc;
            if (melody->phase >= 2 * M_PI)
            {
                melody->phase -= 2 * M_PI;
            }
        }
    }

    int rsize = audio_element_output(self, (char *)melody->buffer, samples_to_gen * sizeof(int16_t));
    if (rsize <= 0)
    { // Error, abort or pipeline stopped: nothing has been played
        return rsize;
    }

    // No lock here: the playback progress belongs to this task only. If the melody has been
    // replaced in the meantime, the next frame detects the new sequence number and restarts,
    // discarding this counter anyway. Infinite notes are not accounted, to avoid any wrap-around.
    if (!infinite_note)
    {
        melody->samples_played += (uint32_t)(rsize / sizeof(int16_t));
    }

    return rsize;
}

static esp_err_t tone_open(audio_element_handle_t self)
{
    ESP_LOGI(Tag, "tone_open called");
    // The playback progress is private to the tone generator task, which is the one calling this. No mutex needed.
    TonePlayer->melody->current_note = 0;
    TonePlayer->melody->samples_played = 0;
    TonePlayer->melody->phase = 0;
    return ESP_OK;
}


static esp_err_t tone_close(audio_element_handle_t self)
{
    ESP_LOGI(Tag, "tone_close called");
    return ESP_OK;
}

static T_TonePlayerHandle InitTonePlayer(void)
{
  T_TonePlayerHandle ap = calloc(1, sizeof(T_TonePlayer));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1.0] Create audio pipeline for tone playback");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _tone_init_failed);

  ESP_LOGI(Tag, "[1.1] Create TONE generator audio element");
  audio_element_cfg_t cfg = DEFAULT_AUDIO_ELEMENT_CONFIG();
  cfg.open = tone_open;
  cfg.process = tone_process;
  cfg.close = tone_close;
  cfg.task_stack = 4096;
  cfg.task_prio  = 5;
  cfg.out_rb_size = 4 * 1024;
  cfg.multi_out_rb_num = 1;
  cfg.read = NULL;
  cfg.write = NULL;
  cfg.tag = "tone";
  //cfg.stack_in_ext = true;
  ap->ToneGenerator = audio_element_init(&cfg);
  AUDIO_MEM_CHECK(Tag, ap->ToneGenerator, goto _tone_init_failed);

  ToneMutex = xSemaphoreCreateMutex();
  if (!ToneMutex) {
      ESP_LOGE(Tag, "ToneMutex creation failed");
      goto _tone_init_failed;
  }

  ap->melody = calloc(1, sizeof(tone_melody_t));
  if(!ap->melody) {
      ESP_LOGE(Tag, "tone_melody_t calloc failed");
      goto _tone_init_failed;
  }
  ap->melody->buffer = tone_buffer;

  ESP_LOGI(Tag, "[1.2] Create I2S stream to write audio data to codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_WRITER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  i2s_cfg.i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S; // I2S_COMM_FORMAT_STAND_MSB; //I2S_COMM_FORMAT_STAND_I2S
  i2s_cfg.i2s_config.sample_rate = TONE_PLAYER_RATE;
  //i2s_cfg.stack_in_ext = true;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);
  AUDIO_MEM_CHECK(Tag, ap->I2SStream, goto _tone_init_failed);

  ESP_LOGI(Tag, "[2.0] Register all elements to audio pipeline");

  audio_pipeline_register(ap->Pipeline, ap->ToneGenerator, "tone_gen");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_writer");

  ESP_LOGI(Tag, "[2.1] Link it together [RAM]-->tone_generator-->i2s_stream-->[codec_chip]");
  audio_pipeline_link(ap->Pipeline, (const char *[]){"tone_gen", "i2s_writer"}, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = false;
  ap->state = STATE_STOPPED;
  ap->Played = false;
  return ap;
_tone_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static T_RecorderHandle InitRecorder(void)
{
  T_RecorderHandle ap = calloc(1, sizeof(T_Recorder));
  AUDIO_MEM_CHECK(Tag, ap, NULL);

  ESP_LOGI(Tag, "[1.0] Create audio pipeline for record");
  audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
  ap->Pipeline = audio_pipeline_init(&pipeline_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Pipeline, goto _recorder_init_failed);

  ESP_LOGI(Tag, "[1.1] Create I2S stream to read audio data from codec chip");
  i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
  i2s_cfg.type = AUDIO_STREAM_READER;
  i2s_cfg.i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT;
  i2s_cfg.i2s_config.sample_rate = RECORD_RATE;
  // i2s_cfg.i2s_config.fixed_mclk = 4096000;
  // i2s_cfg.out_rb_size = 100*1024;
  // i2s_cfg.task_core = 1;
  //i2s_cfg.stack_in_ext = true;
  ap->I2SStream = i2s_stream_init(&i2s_cfg);
  i2s_stream_set_clk(ap->I2SStream, RECORD_RATE, RECORD_BITS, RECORD_CHANNEL);

  ESP_LOGI(Tag, "[1.2] Create WAV encoder to encode WAV format");
  wav_encoder_cfg_t wav_cfg = DEFAULT_WAV_ENCODER_CONFIG();
  wav_cfg.stack_in_ext = 0;
  // wav_cfg.out_rb_size = 100*1024;
  // wav_cfg.task_core = 1;
  ap->Encoder = wav_encoder_init(&wav_cfg);
  AUDIO_MEM_CHECK(Tag, ap->Encoder, goto _recorder_init_failed);

  ESP_LOGI(Tag, "[2.0] Register all elements to audio pipeline");
  audio_pipeline_register(ap->Pipeline, ap->I2SStream, "i2s_reader");
  audio_pipeline_register(ap->Pipeline, ap->Encoder, "wav_encoder");

  ESP_LOGI(Tag, "[2.1] Link it together [codec_chip]-->i2s_stream-->wav_encoder-->[RAM]");
  audio_pipeline_link(ap->Pipeline, (const char *[]){"i2s_reader", "wav_encoder"}, 2);

  ESP_LOGI(Tag, "[3.0] Setup event listener");
  audio_event_iface_cfg_t evt_cfg = AUDIO_EVENT_IFACE_DEFAULT_CFG();
  ap->Evt = audio_event_iface_init(&evt_cfg);

  ESP_LOGI(Tag, "[3.1] Listening event from all elements of pipeline");
  audio_pipeline_set_listener(ap->Pipeline, ap->Evt);

  ap->Run = true;
  ap->state = STATE_STOPPED;
  ap->Recorded = false;

  audio_element_set_write_cb(ap->Encoder, recorder_write_cb, NULL);

  audio_element_info_t music_info = {0};
  audio_element_getinfo(ap->Encoder, &music_info);
  music_info.bits = RECORD_BITS;
  music_info.channels = RECORD_CHANNEL;
  music_info.sample_rates = RECORD_RATE;
  audio_element_setinfo(ap->Encoder, &music_info);

  return ap;
_recorder_init_failed:
  return NULL;
}

//_____________________________________________________________________________

static void SelectFile(tone_type_t index)
{
  if(index >= TONE_TYPE_MAX)
  {
    ESP_LOGW(Tag, "Not supported index = %d", index);
  } 
  else 
  {
    audio_element_set_uri(OnboardPlayer->FlashToneStream, tone_uri[index]);
    SoundStatus[index] = E_SoundStatus_Started;
  }
}

//_____________________________________________________________________________

static void RunMp3PlayerTask(void *arg)
{
  audio_event_iface_msg_t msg;
  esp_err_t ret;

  while (Mp3Player->Run)
  {
    ret = audio_event_iface_listen(Mp3Player->Evt, &msg, portMAX_DELAY);

    if (ret != ESP_OK)
    {
      ESP_LOGI(Tag, "Mp3PlayerTask: event interface error : %d\n", ret);
      continue;
    }
/*
    if (msg.source == Mp3Player->Mp3Decoder)
    {
      printf("Mp3PlayerTask event: type=%d, source=Player->Mp3Decoder, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == Mp3Player->I2SStream)
    {
      printf("Mp3PlayerTask event: type=%d, source=Player->I2SStream, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == Mp3Player->Pipeline)
    {
      printf("Mp3PlayerTask event: type=%d, source=Player->Pipeline, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else
    {
      printf("Mp3PlayerTask event: type=%d, source=uknown(%p), cmd=%d, data=%d\n", msg.source_type, msg.source, msg.cmd, (int)msg.data);
    }
*/
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Mp3Player->Mp3Decoder) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_RUNNING))
    {
      ESP_LOGI(Tag, "mp3 running\n");
      Mp3Player->state = STATE_RUNNING;
      Mp3Player->Played = false;
      continue;
    }

    // Handle rate for mp3 files.
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Mp3Player->Mp3Decoder) && (msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO))
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(Mp3Player->Mp3Decoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);
      //printf("Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d\n", music_info.sample_rates, music_info.bits, music_info.channels);

      // i2s_stream_set_clk(Mp3Player->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels); // This is not needed because the parameters (12khz rate, 16 bits, 1 channel) are always the same for all players and recorder.
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives finished event
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Mp3Player->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN)))
    {
      ESP_LOGI(Tag, "Stop pipeline from task");
      audio_pipeline_stop(Mp3Player->Pipeline);
      audio_pipeline_wait_for_stop(Mp3Player->Pipeline);
      audio_pipeline_reset_ringbuffer(Mp3Player->Pipeline);
      Mp3Player->state = STATE_STOPPED;
      Mp3Player->Played = true;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Mp3Player->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_STOPPED))
    {
      Mp3Player->state = STATE_STOPPED;
      ESP_LOGI(Tag, "Pipeline stopped from Mp3PlayerTask\n");
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Mp3Player->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_PAUSED))
    {
      // Some events can be queued thus check that the current state is actually running before going to pause
      // For example: 1) user "pause" just before play ending 2) event "play end" => state stopped 3) event "paused" => state paused => not valid!
      if (Mp3Player->state == STATE_RUNNING)
      {
        Mp3Player->state = STATE_PAUSED;
        ESP_LOGI(Tag, "Mp3 paused\n");
      }
    }
  }

  vTaskDelete(NULL);
}

static void RunOnboardPlayerTask(void *arg)
{
  audio_event_iface_msg_t msg;
  esp_err_t ret;

  while (OnboardPlayer->Run)
  {
    ret = audio_event_iface_listen(OnboardPlayer->Evt, &msg, portMAX_DELAY);
    if (ret != ESP_OK)
    {
      ESP_LOGI(Tag, "OnboardPlayer: event interface error : %d\n", ret);
      continue;
    }
/*
    if (msg.source == OnboardPlayer->Mp3Decoder)
    {
      printf("OnboardPlayerTask event: type=%d, source=OnboardPlayer->Mp3Decoder, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == OnboardPlayer->FlashToneStream)
    {
      printf("OnboardPlayerTask event: type=%d, source=OnboardPlayer->FlashToneStream, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == OnboardPlayer->I2SStream)
    {
      printf("OnboardPlayerTask event: type=%d, source=OnboardPlayer->I2SStream, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == OnboardPlayer->Pipeline)
    {
      printf("OnboardPlayerTask event: type=%d, source=OnboardPlayer->Pipeline, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else
    {
      printf("OnboardPlayerTask event: type=%d, source=uknown(%p), cmd=%d, data=%d\n", msg.source_type, msg.source, msg.cmd, (int)msg.data);
    }
*/
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->Mp3Decoder) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_RUNNING))
    {
      OnboardPlayer->state = STATE_RUNNING;
      OnboardPlayer->Played = false;
      ESP_LOGI(Tag, "onboard running\n");
      continue;
    }

    // Handle rate for mp3 files.
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->Mp3Decoder) && (msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO))
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(OnboardPlayer->Mp3Decoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);

      //printf("Receive music info from MP3 decoder, sample_rates=%d, bits=%d, ch=%d\n", music_info.sample_rates, music_info.bits, music_info.channels);

      // i2s_stream_set_clk(OnboardPlayer->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels); // This is not needed because the parameters (12khz rate, 16 bits, 1 channel) are always the same for all players and recorder.
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives finished event
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN)))
    {
      ESP_LOGI(Tag, "Stop pipeline from OnboardPlayer\n");
      // break;
      audio_pipeline_stop(OnboardPlayer->Pipeline);
      audio_pipeline_wait_for_stop(OnboardPlayer->Pipeline);
      audio_pipeline_reset_ringbuffer(OnboardPlayer->Pipeline); // This is needed!
      SoundStatus[OnboardSoundIndex] = E_SoundStatus_Finished;
      OnboardPlayer->state = STATE_STOPPED;
      OnboardPlayer->Played = true;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->FlashToneStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN)))
    {
      ESP_LOGI(Tag, "FlashToneStream finished\n");
      // Set the "stopped state" as soon as one of the audio elements receive "finish event".
      // This avoid to pause/stop between the "flashtonestream" finished and the "i2sstream" not yet finished.
      OnboardPlayer->state = STATE_ALMOST_STOPPED;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_STOPPED))
    {
      OnboardPlayer->state = STATE_STOPPED;
      ESP_LOGI(Tag, "Pipeline stopped from OnboardPlayerTask\n");
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)OnboardPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_PAUSED))
    {
      // Some events can be queued thus check that the current state is actually running before going to pause
      // For example: 1) user "pause" just before play ending 2) event "play end" => state stopped 3) event "paused" => state paused => not valid!
      if (OnboardPlayer->state == STATE_RUNNING)
      {
        OnboardPlayer->state = STATE_PAUSED;
        ESP_LOGI(Tag, "Onboard sound paused");
      }
    }
  }

  /*
    audio_pipeline_stop(OnboardPlayer->Pipeline);
    audio_pipeline_wait_for_stop(OnboardPlayer->Pipeline);
    audio_pipeline_terminate(OnboardPlayer->Pipeline);
    audio_pipeline_reset_ringbuffer(OnboardPlayer->Pipeline); // This is needed!
    audio_event_iface_discard(OnboardPlayer->Evt);            // Discard the "stopped" event queued after "audio_pipeline_stop"
    OnboardPlayer->Playing = false;
  */
  vTaskDelete(NULL); // Delete this task
}

static void RunWavPlayerTask(void *arg)
{
  audio_event_iface_msg_t msg;
  esp_err_t ret;

  while (WavPlayer->Run)
  {
    ret = audio_event_iface_listen(WavPlayer->Evt, &msg, portMAX_DELAY);
    if (ret != ESP_OK)
    {
      ESP_LOGI(Tag, "WavPlayerTask: event interface error : %d\n", ret);
      continue;
    }
/*
    if (msg.source == WavPlayer->I2SStream)
    {
      printf("WavPlayerTask event: type=%d, source=Player->I2SStream, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == WavPlayer->Pipeline)
    {
      printf("WavPlayerTask event: type=%d, source=Player->Pipeline, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == WavPlayer->WavDecoder)
    {
      printf("WavPlayerTask event: type=%d, source=Player->WavDecoder, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else
    {
      printf("WavPlayerTask event: type=%d, source=uknown(%p), cmd=%d, data=%d\n", msg.source_type, msg.source, msg.cmd, (int)msg.data);
    }
*/
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)WavPlayer->WavDecoder) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_RUNNING))
    {
      ESP_LOGI(Tag, "wav running\n");
      WavPlayer->state = STATE_RUNNING;
      WavPlayer->Played = false;
      continue;
    }

    // Handle rate for wav files.
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)WavPlayer->WavDecoder) && (msg.cmd == AEL_MSG_CMD_REPORT_MUSIC_INFO))
    {
      audio_element_info_t music_info = {0};
      audio_element_getinfo(WavPlayer->WavDecoder, &music_info);

      ESP_LOGE(Tag, "[ * ] Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d",
               music_info.sample_rates, music_info.bits, music_info.channels);
      //printf("Receive music info from WAV decoder, sample_rates=%d, bits=%d, ch=%d\n", music_info.sample_rates, music_info.bits, music_info.channels);

      // i2s_stream_set_clk(WavPlayer->I2SStream, music_info.sample_rates, music_info.bits, music_info.channels); // This is not needed because the parameters (12khz rate, 16 bits, 1 channel) are always the same for all players and recorder.
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives finished event
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)WavPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN)))
    {
      ESP_LOGE(Tag, "Stop pipeline from task");
      audio_pipeline_stop(WavPlayer->Pipeline);
      audio_pipeline_wait_for_stop(WavPlayer->Pipeline);
      audio_pipeline_reset_ringbuffer(WavPlayer->Pipeline);
      WavPlayer->state = STATE_STOPPED;
      WavPlayer->Played = true;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)WavPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_STOPPED))
    {
      WavPlayer->state = STATE_STOPPED;
      ESP_LOGI(Tag, "Pipeline stopped from WavPlayerTask\n");
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)WavPlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_PAUSED))
    {
      // Some events can be queued thus check that the current state is actually running before going to pause
      // For example: 1) user "pause" just before play ending 2) event "play end" => state stopped 3) event "paused" => state paused => not valid!
      if (WavPlayer->state == STATE_RUNNING)
      {
        WavPlayer->state = STATE_PAUSED;
        ESP_LOGI(Tag, "Wav paused\n");
      }
    }
  }
}

static void RunRecordTask(void *arg)
{
  int second_recorded = 0;
  audio_event_iface_msg_t msg;
  esp_err_t ret;

  while (Recorder->Run)
  {
    ret = audio_event_iface_listen(Recorder->Evt, &msg, 1000 / portTICK_RATE_MS); // Wait at most 1 second (this is the time base for recording duration).
/*
    audio_element_info_t music_info = {0};
    audio_element_getinfo(Recorder->Encoder, &music_info);
    printf("Encoder sample_rates=%d, bits=%d, ch=%d\n", music_info.sample_rates, music_info.bits, music_info.channels);
    audio_element_getinfo(Recorder->I2SStream, &music_info);
    printf("I2SStream sample_rates=%d, bits=%d, ch=%d\n", music_info.sample_rates, music_info.bits, music_info.channels);
*/
    if (ret != ESP_OK)
    {
      // ESP_LOGE(Tag, "[ * ] Event interface error : %d", ret);
      if (Recorder->state == STATE_RUNNING)
      {
        ESP_LOGI(Tag, "recordBufferIndex = %d\n", recordBufferIndex);
        second_recorded++;
        ESP_LOGI(Tag, "RecorderTask: event interface error : %d\n", ret);
        ESP_LOGI(Tag, "RecorderTask: recording ... %d\n", second_recorded);

/*
        ringbuf_handle_t rb = audio_element_get_input_ringbuf(Recorder->I2SStream);
        if (rb != NULL)
        {
          printf("I2Sstream input buffer: %d (size), %d (free), %d (used)\n", rb_get_size(rb), rb_bytes_available(rb), rb_bytes_filled(rb));
        }
        rb = audio_element_get_output_ringbuf(Recorder->I2SStream);
        if (rb != NULL)
        {
          printf("I2Sstream output buffer: %d (size), %d (free), %d (used)\n", rb_get_size(rb), rb_bytes_available(rb), rb_bytes_filled(rb));
        }
        rb = audio_element_get_input_ringbuf(Recorder->Encoder);
        if (rb != NULL)
        {
          printf("Encoder input buffer: %d (size), %d (free), %d (used)\n", rb_get_size(rb), rb_bytes_available(rb), rb_bytes_filled(rb));
        }
        rb = audio_element_get_output_ringbuf(Recorder->Encoder);
        if (rb != NULL)
        {
          printf("Encoder output buffer: %d (size), %d (free), %d (used)\n", rb_get_size(rb), rb_bytes_available(rb), rb_bytes_filled(rb));
        }
*/
        // printStats = true;
        if (second_recorded >= RecordingDuration_s)
        {
          audio_element_set_ringbuf_done(Recorder->I2SStream);
          second_recorded = 0;
        }
      }

      continue;
    }

/*
    if (msg.source == Recorder->Encoder)
    {
      printf("RecorderTask event: type=%d, source=Recorder->Encoder, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == Recorder->I2SStream)
    {
      printf("RecorderTask event: type=%d, source=Recorder->I2SStream, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else if (msg.source == Recorder->Pipeline)
    {
      printf("RecorderTask event: type=%d, source=Recorder->Pipeline, cmd=%d, data=%d\n", msg.source_type, msg.cmd, (int)msg.data);
    }
    else
    {
      printf("RecorderTask event: type=%d, source=uknown(%p), cmd=%d, data=%d\n", msg.source_type, msg.source, msg.cmd, (int)msg.data);
    }
*/
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Recorder->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_RUNNING))
    {
      ESP_LOGI(Tag, "rec running\n");
      Recorder->state = STATE_RUNNING;
      continue;
    }

    /* Enable buttons when the last pipeline element (spiffs_stream_writer) receives stop event */
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Recorder->Encoder) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_ERROR_OPEN) || ((int)msg.data == AEL_STATUS_STATE_FINISHED)))
    {
      //ESP_LOGI(Tag, "recordBufferIndex = %d\n", recordBufferIndex);
      break;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)Recorder->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_PAUSED))
    {
      Recorder->state = STATE_PAUSED;
      ESP_LOGI(Tag, "Recording paused");
    }
  }

  audio_pipeline_stop(Recorder->Pipeline);
  audio_pipeline_wait_for_stop(Recorder->Pipeline);
  audio_pipeline_terminate(Recorder->Pipeline);
  audio_pipeline_reset_ringbuffer(Recorder->Pipeline); // This is needed!
  audio_event_iface_discard(Recorder->Evt);            // Discard the "stopped" event queued after "audio_pipeline_stop"
  // FileSystem_Write("/spiffs/prova2.wav", recordBuffer, recordBufferIndex);

  wav_header_t *wav_info = (wav_header_t *)malloc(sizeof(wav_header_t));
  wav_head_init(wav_info, 12000, 16, 1);
  wav_head_size(wav_info, (uint32_t)recordBufferIndex - 44);
  memcpy(recordBuffer, wav_info, sizeof(wav_header_t));
  free(wav_info);

  //Sensors_buttons_resume();
  Recorder->Recorded = true;
  Recorder->state = STATE_STOPPED;
  Behavior_Disable(B_LED_MIC_STATE); // Turn off MIC LED
  
  vTaskDelete(NULL);
}

static void RunTonePlayerTask(void *arg)
{
  audio_event_iface_msg_t msg;
  esp_err_t ret;

  while (TonePlayer->Run)
  {
    ret = audio_event_iface_listen(TonePlayer->Evt, &msg, portMAX_DELAY);
    if (ret != ESP_OK)
    {
      ESP_LOGI(Tag, "TonePlayerTask: event interface error : %d\n", ret);
      continue;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)TonePlayer->ToneGenerator) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_RUNNING))
    {
      ESP_LOGI(Tag, "tone running\n");
      TonePlayer->state = STATE_RUNNING;
      TonePlayer->Played = false;
      continue;
    }

    // Stop when the last pipeline element (I2SStream in this case) receives finished event
    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)TonePlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && (((int)msg.data == AEL_STATUS_STATE_FINISHED) || ((int)msg.data == AEL_STATUS_ERROR_OPEN)))
    {
      ESP_LOGE(Tag, "Stop pipeline from TonePlayerTask");
      audio_pipeline_stop(TonePlayer->Pipeline);
      audio_pipeline_wait_for_stop(TonePlayer->Pipeline);
      audio_pipeline_reset_ringbuffer(TonePlayer->Pipeline);
      TonePlayer->state = STATE_STOPPED;
      TonePlayer->Played = true;
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)TonePlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_STOPPED))
    {
      TonePlayer->state = STATE_STOPPED;
      ESP_LOGI(Tag, "Pipeline stopped from TonePlayerTask\n");
    }

    if ((msg.source_type == AUDIO_ELEMENT_TYPE_ELEMENT) && (msg.source == (void *)TonePlayer->I2SStream) && (msg.cmd == AEL_MSG_CMD_REPORT_STATUS) && ((int)msg.data == AEL_STATUS_STATE_PAUSED))
    {
      // Some events can be queued thus check that the current state is actually running before going to pause
      // For example: 1) user "pause" just before play ending 2) event "play end" => state stopped 3) event "paused" => state paused => not valid!
      if (TonePlayer->state == STATE_RUNNING)
      {
        TonePlayer->state = STATE_PAUSED;
        ESP_LOGI(Tag, "Tone paused\n");
      }
    }
  }
}

//_____________________________________________________________________________

esp_err_t Codec_Stop(void)
{
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }  
  if (WavPlayer->state != STATE_STOPPED)
  {
    audio_pipeline_stop(WavPlayer->Pipeline);
    audio_pipeline_wait_for_stop(WavPlayer->Pipeline);
    audio_pipeline_reset_ringbuffer(WavPlayer->Pipeline);
    audio_event_iface_discard(WavPlayer->Evt);
    WavPlayer->state = STATE_STOPPED;
  }

  if (OnboardPlayer->state != STATE_STOPPED)
  {
    audio_pipeline_stop(OnboardPlayer->Pipeline);
    audio_pipeline_wait_for_stop(OnboardPlayer->Pipeline);
    audio_pipeline_reset_ringbuffer(OnboardPlayer->Pipeline);
    audio_event_iface_discard(OnboardPlayer->Evt);
    SoundStatus[OnboardSoundIndex] = E_SoundStatus_Finished;
    OnboardPlayer->state = STATE_STOPPED;
  }

  if (Mp3Player->state != STATE_STOPPED)
  {
    audio_pipeline_stop(Mp3Player->Pipeline);
    audio_pipeline_wait_for_stop(Mp3Player->Pipeline);
    audio_pipeline_reset_ringbuffer(Mp3Player->Pipeline);
    audio_event_iface_discard(Mp3Player->Evt);
    Mp3Player->state = STATE_STOPPED;
  }

  if (TonePlayer->state != STATE_STOPPED)
  {
    audio_pipeline_stop(TonePlayer->Pipeline);
    audio_pipeline_wait_for_stop(TonePlayer->Pipeline);
    audio_pipeline_reset_ringbuffer(TonePlayer->Pipeline);
    audio_event_iface_discard(TonePlayer->Evt);
    TonePlayer->state = STATE_STOPPED;
  }  

  return ESP_OK;
}

//_____________________________________________________________________________

esp_err_t Codec_Pause(void)
{
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }  
  if (OnboardPlayer->state == STATE_RUNNING)
  {
    audio_element_info_t info;
    audio_element_getinfo(OnboardPlayer->FlashToneStream, &info);
    // uint32_t time_ms = 0;
    // time_ms = (info.byte_pos)/24;
    // printf("FlashToneStream Played Time: %d ms\n", time_ms);
    // printf("FlashToneStream: byte_pos=%lld, total_bytes=%lld\n", info.byte_pos, info.total_bytes);

    // Avoid pausing when near the end of the play to avoid conflicts between delayed events and actual pipeline state.
    // For example: 1) play end => read cb "finish" 2) user pause 3) "finish event" not queued, only "paused event" received => pipeline state lost.
    if ((info.total_bytes - info.byte_pos) < PLAY_NEAR_END_THR)
    {
      return ESP_FAIL;
    }
    audio_pipeline_pause(OnboardPlayer->Pipeline);
  }
  else if (WavPlayer->state == STATE_RUNNING)
  {
    // Avoid pausing when near the end of the play to avoid conflicts between delayed events and actual pipeline state.
    // For example: 1) play end => read cb "finish" 2) user pause 3) "finish event" not queued, only "paused event" received => pipeline state lost.
    if ((playerBufferSize - playerBufferIndex) < PLAY_NEAR_END_THR)
    {
      return ESP_FAIL;
    }
    audio_pipeline_pause(WavPlayer->Pipeline);
  }
  else if (Mp3Player->state == STATE_RUNNING)
  {
    // Avoid pausing when near the end of the play to avoid conflicts between delayed events and actual pipeline state.
    // For example: 1) play end => read cb "finish" 2) user pause 3) "finish event" not queued, only "paused event" received => pipeline state lost.
    if ((playerBufferSize - playerBufferIndex) < PLAY_NEAR_END_THR)
    {
      return ESP_FAIL;
    }
    audio_pipeline_pause(Mp3Player->Pipeline);
  }
  else if (TonePlayer->state == STATE_RUNNING)
  {
    audio_pipeline_pause(TonePlayer->Pipeline);
  }  
  else if (Recorder->state == STATE_RUNNING)
  {
    audio_pipeline_pause(Recorder->Pipeline);
  }
  else
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}

esp_err_t Codec_Resume(void)
{
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return ESP_FAIL;
  }  
  if (OnboardPlayer->state == STATE_PAUSED)
  {
    audio_pipeline_resume(OnboardPlayer->Pipeline);
  }
  else if (WavPlayer->state == STATE_PAUSED)
  {
    audio_pipeline_resume(WavPlayer->Pipeline);
  }
  else if (Mp3Player->state == STATE_PAUSED)
  {
    audio_pipeline_resume(Mp3Player->Pipeline);
  }
  else if (TonePlayer->state == STATE_PAUSED)
  {
    audio_pipeline_resume(TonePlayer->Pipeline);
  }  
  else if (Recorder->state == STATE_PAUSED)
  {
    audio_pipeline_resume(Recorder->Pipeline);
  }
  else
  {
    return ESP_FAIL;
  }
  return ESP_OK;
}

//_____________________________________________________________________________

void  Codec_ClearEvents(void) {
  if ((OnboardPlayer == NULL) || (Recorder == NULL) || (Mp3Player == NULL) || (WavPlayer == NULL) || (TonePlayer == NULL))
  {
    return;
  }  
  OnboardPlayer->Played = false;
  WavPlayer->Played = false;
  Mp3Player->Played = false;  
  Recorder->Recorded = false;
  TonePlayer->Played = false;  
}

/*
void playMarioTheme(void) 
{
  int i = 0;
  // Define musical notes in Hz (Standard pitch notation)
  const int C4 = 262; const int D4 = 294; const int E4 = 330; const int F4 = 349; 
  const int G4 = 392; const int A4 = 440; const int B4 = 494;
  const int C5 = 523; const int D5 = 587; const int E5 = 659; const int F5 = 698; const int G5 = 784;
  const int A5 = 880; const int C6 = 1047;
    
  // Tempo and duration constants in Milliseconds (ms)
  const int QUARTER = 200;
  const int EIGHTH = QUARTER / 2;     // 100 ms
  const int SIXTEENTH = QUARTER / 4;  // 50 ms
  const int DOT_EIGHTH = EIGHTH + SIXTEENTH; // 150 ms

  // Notes of the first phrase: E5, E5, E5, C5, E5, G5, G4
  struct Notes {
    int freq;
    int dur;
  };  
  // This sequence is designed to last approximately 10 seconds at the defined tempo.
  const struct Notes marioTheme[] = {
    // Phrase 1: The Opening Riff (Original part)
    { E5, EIGHTH }, { 0, SIXTEENTH }, { E5, EIGHTH }, { 0, SIXTEENTH }, { E5, EIGHTH }, { 0, SIXTEENTH }, { C5, EIGHTH }, 
    { E5, DOT_EIGHTH }, { G5, QUARTER }, { 0, QUARTER }, { G4, QUARTER }, { 0, QUARTER }, 
    
    // Phrase 2: Continuation
    { C5, QUARTER }, { 0, QUARTER }, { G4, QUARTER }, { 0, QUARTER }, { E4, QUARTER }, { 0, QUARTER }, { A4, DOT_EIGHTH }, 
    { B4, EIGHTH }, { A4, QUARTER }, { G4, EIGHTH }, { E5, EIGHTH }, { G5, EIGHTH }, { A5, EIGHTH }, { F5, EIGHTH }, { G5, EIGHTH },
    
    // Phrase 3: The Ascending Run
    { E5, SIXTEENTH }, { C5, SIXTEENTH }, { A4, SIXTEENTH }, { G4, SIXTEENTH }, // Quick run up/down
    { E5, EIGHTH }, { 0, SIXTEENTH }, { F5, EIGHTH }, { 0, SIXTEENTH }, { G5, EIGHTH }, { 0, SIXTEENTH }, { C6, QUARTER },
    
    // Phrase 4: The Final High Note (to extend duration)
    { G5, QUARTER }, { 0, QUARTER }, { E5, QUARTER }, { 0, QUARTER },
    { E5, EIGHTH }, { D5, EIGHTH }, { C5, QUARTER }, { 0, QUARTER }, // Ending cadence
    
    // Added rests/padding to ensure near 10 seconds
    { 0, QUARTER }, { 0, QUARTER }, { 0, QUARTER }, { 0, QUARTER } 
  };  

  int numNotes = sizeof(marioTheme) / sizeof(marioTheme[0]);

  for(i=0; i< numNotes; i++) {
      
      //Codec_PlayTone(marioTheme[i].freq, marioTheme[i].dur);
      //while(!Codec_IsSoundFinished()) {
      //    vTaskDelay(1 / portTICK_PERIOD_MS);
      //}
      
     Codec_PlayTone(marioTheme[i].freq, 0);
     vTaskDelay(marioTheme[i].dur / portTICK_PERIOD_MS);

  }
}
*/
