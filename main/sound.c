//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
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

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "esp_log.h"

#include "sound.h"

#include "adc.h"
#include "cosine_generator.h"
#include "dsp.h"
#include "fifo.h"
#include "i2s.h"
#include "mp3.h"
#include "timer_hw.h"

#include "melody.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//#define ACQUISITION_TASK_PERIOD_us  200u  //!< Acquisition task frequency = 8 [kHz]

#define MICROPHONE_BUFFER_SIZE      800u

#define NUMBER_OF_SAMPLES           200u

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef enum
{
  E_DacStatus_Disable,  //!< DAC disable
  E_DacStatus_I2S,      //!< DAC enabled by I2S
  E_DacStatus_Cosine    //!< DAC enabled by the cosine generator
} T_DacStatus;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

TaskHandle_t AcquisitionTask;

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "sound";

static T_TimerHw* GeneratorTimer = NULL;  //!< Used to generate the sound

static bool NoteInProgress = false;
static bool MelodyIsFinished = false;

static uint16_t NextNote = 0u;
static uint16_t NextLoop = 0u;

uint16_t Number = 0;
uint16_t Loop = 0;

static T_DacStatus DacStatus = E_DacStatus_Disable;

static float MicrophoneBuffer[MICROPHONE_BUFFER_SIZE];  // RECV_QUEUE_SIZE

T_FifoFloat* MicrophoneFifo = NULL;

float input[NUMBER_OF_SAMPLES];
float outputRe[NUMBER_OF_SAMPLES / 2];
float outputIm[NUMBER_OF_SAMPLES / 2];
float outputMag[NUMBER_OF_SAMPLES / 2];

float mean;
float max;

static bool AcquiredDataAreReady = false;

//static TaskHandle_t ProcessingTask = NULL;

static T_TimerHw* AcquisitionTaskTimer = NULL;  //!< Used to schedule the Acquisition task

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Run the task to play a sound
//! \pre       First initialize the sound
//! \param     pvParameter - Task parameter
//! \return    None
static void RunPlayingTask(T_Melody* arg);

static void RunAcquisitionTask(void* arg);

//! \brief     Run the task to process a sound
//! \pre       First initialize the sound
//! \param     pvParameter - Task parameter
//! \return    None
static void RunProcessingTask(void* arg);

//! \brief     Calculate the duration of the note
//! \pre       First initialize the sound
//! \param     noteValue - Value of the note
//! \param     tempo - Tempo of the melody
//! \return    Duration of the note in [ms]
static int16_t CalculateNoteDuration(T_NoteValue noteValue, T_Tempo tempo);

static void FeedWatchdog(void);

//! \brief     Callback function called at the end of each note played
//! \pre       First initialize the sound
//! \param     arg - Argument
//! \return    None
static void Callback_TimerGenerator(void* arg);

static void Callback_TimerAcquisitionTask(void* arg);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Sound_Init(void)
{
  //I2S_Init();

  //MP3_Init();

  CosineGenerator_Init();

  GeneratorTimer = TimerHw_Create(1000, Callback_TimerGenerator);

  DacStatus = E_DacStatus_Cosine;

  MicrophoneFifo = FifoFloat_Create(MicrophoneBuffer, MICROPHONE_BUFFER_SIZE);

  //AcquisitionTaskTimer = TimerHw_Create(ACQUISITION_TASK_PERIOD_us, Callback_TimerAcquisitionTask);

  ESP_LOGI(Tag, "Sound is initialized");
}

//_____________________________________________________________________________

void Sound_StartPlaying(T_Melody* melody)
{
  xTaskCreatePinnedToCore(
    RunPlayingTask,  // Function to implement the task
    "play",          // Name of the task
    2048,            // Stack size in words
    melody,          // Task input parameter
    2,               // Priority of the task
    NULL,            // Task handle
    0);              // Core where the task should run
}

//_____________________________________________________________________________

void Sound_StartAcquisition(void)
{
  xTaskCreatePinnedToCore(
    RunAcquisitionTask,  // Function to implement the task
    "acquisition",       // Name of the task
    4096,                // Stack size in words
    NULL,                // Task input parameter
    1,                   // Priority of the task
    &AcquisitionTask,    // Task handle
    0);                  // Core where the task should run
}

//_____________________________________________________________________________

void Sound_StartProcessing(void)
{
  xTaskCreatePinnedToCore(
    RunProcessingTask,  // Function to implement the task
    "sound",            // Name of the task
    4096,               // Stack size in words
    NULL,               // Task input parameter
    3,                  // Priority of the task
    NULL,               // Task handle
    1);                 // Core where the task should run
}

//_____________________________________________________________________________

void Sound_RunReplayerTask(void* pvParameter)
{
  while (1)
  {
    Sound_Replay();
    vTaskDelete(NULL);
  }
}

//_____________________________________________________________________________

void Sound_RunRecordingTask(void* pvParameter)
{
  while (1)
  {
    Sound_Record();
    //vTaskDelete(NULL);
  }
}

//_____________________________________________________________________________
#if 0
void Sound_Process(void)
{
  float outputRe[50];
  float outputIm[50];

  DSP_CalculateDFT(InputSignal_ech1kHz_20Hz, outputRe, outputIm, 100);

  DSP_CalculateDFTOutputMag(outputRe, outputIm, outputMag, 50);

  //for (uint16_t index = 0; index < 160; index++)
  {
    //ESP_LOGI(Tag, "index = %d, out = %f", 140, outputMag[140]);
  }
}
#endif
#if 0
void Sound_Process(uint16_t* data, uint16_t size)
{
  uint16_t input[100];
  uint16_t outputRe[50];
  uint16_t outputIm[50];
  //uint16_t outputMag[10];
  //uint16_t max;
  //uint16_t mean;

  Fifo16bits_Write(MicrophoneFifo, data, size);

  if (Fifo16bits_GetNumberOfElements(MicrophoneFifo) == 100)
  {
    Fifo16bits_Read(MicrophoneFifo, input, 100);

    //max = DSP_GetMaxValue(input, 10);

    //DSP_CalculateDFT(input, outputRe, outputIm, 100);

    //DSP_CalculateDFTOutputMag(outputRe, outputIm, outputMag, 50);

    mean = DSP_CalculateMeanValue(input, 100);
  }
}
#endif
#if 0
void Sound_Process(float* data, uint16_t size)
{
  FifoFloat_Write(MicrophoneFifo, data, size);
}
#endif

//_____________________________________________________________________________

void Sound_PlayNote(T_Note note, int16_t duration_ms)
{
  if (DacStatus == E_DacStatus_I2S)
  {
    I2S_DisableDAC();
  }

  DacStatus = E_DacStatus_Cosine;

  CosineGenerator_Enable();

  if (!NoteInProgress)
  {
    NoteInProgress = true;

    TimerHw_StartTimerOnce(GeneratorTimer, (duration_ms * 1000));

    CosineGenerator_ConfigureSignal(note.Name, note.Dynamics);
  }
}

//_____________________________________________________________________________

void Sound_PlayMelody(const T_Note* melody, T_Tempo tempo, uint16_t loop, uint16_t size)
{
  int16_t duration_ms = 0;

  Number = size;
  Loop = loop;

  if (!MelodyIsFinished)
  {
    duration_ms = CalculateNoteDuration(melody[NextNote].Value, tempo);

    Sound_PlayNote(melody[NextNote], duration_ms);
  }
}

//_____________________________________________________________________________

void Sound_Record(void)
{
  I2S_Record();
}

//_____________________________________________________________________________

void Sound_Replay(void)
{
  if (DacStatus == E_DacStatus_Cosine)
  {
    CosineGenerator_Disable();
  }

  DacStatus = E_DacStatus_I2S;

  I2S_EnableDAC();
  I2S_ReadFromFlash();
}

//_____________________________________________________________________________

static void RunPlayingTask(T_Melody* arg)
{
  while (1)
  {
    Sound_PlayMelody(arg->Melody, arg->Tempo, arg->Loop, arg->Size);

    if (MelodyIsFinished)
    {
      MelodyIsFinished = false;
      vTaskDelete(NULL);
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

//_____________________________________________________________________________

static void RunAcquisitionTask(void* arg)
{
  uint32_t result = 0;
  uint16_t micro = 0u;
  float val = 0.0;

  //float table[NUMBER_OF_SAMPLES];
  //static uint16_t count = 0u;

  //TimerHw_StartTimerPeriodically(AcquisitionTaskTimer, ACQUISITION_TASK_PERIOD_us);

  ESP_LOGI(Tag, "Start Sound Acquisition Task");

  while (1)
  {
	// The task being notified should wait for the notification using the ulTaskNotifyTake()
	// API function rather than the xTaskNotifyWait() API function
	result = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    if (result != 0u)
    {
      ADC_AcquireMicrophoneValues(&micro);

      val = ((micro * 1.1) / 4095) * 3.6;

      FifoFloat_Write(MicrophoneFifo, &val, 1);
#if 0
      table[count] = ((micro * 1.1) / 4095) * 3.6;

      count++;

      if (count == NUMBER_OF_SAMPLES)
      {
        //ESP_LOGI(Tag, "val = %d", count);
    	//printf("SOUND Hello world from core %d!\n", xPortGetCoreID());
        count = 0;
        FifoFloat_Write(MicrophoneFifo, table, NUMBER_OF_SAMPLES);
        AcquiredDataAreReady = true;
      }

      //FifoFloat_Write(MicrophoneFifo, &val, 1);
//#if 0
      //if (FifoFloat_GetNumberOfElements(MicrophoneFifo) >= NUMBER_OF_SAMPLES)
      {
        //ESP_LOGI(Tag, "count = %d", count);
        //FifoFloat_Read(MicrophoneFifo, input, NUMBER_OF_SAMPLES);

        //xTaskNotifyGive(ProcessingTask);
        //count++;
        //AcquiredDataAreReady = true;
      }
#endif
    }
    else
    {
      ESP_LOGI(Tag, "OUPS");
    }

    FeedWatchdog();
  }
}

//_____________________________________________________________________________

//#if 0
static void RunProcessingTask(void* arg)
{
  T_Max maxOutput;
  static uint16_t count = 0u;
  uint32_t result = 0;

//#if 0
  ESP_LOGI(Tag, "Start Sound Processing Task");

  //I2S_EnableADC();
#if 0
  while (1)
  {
    I2S_Process();
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
#endif
//#if 0
  while (1)
  {
    //if (xTaskNotifyWait(0, 0, &result, portMAX_DELAY) == pdTRUE)
    if (FifoFloat_GetNumberOfElements(MicrophoneFifo) >= NUMBER_OF_SAMPLES)
    //if (AcquiredDataAreReady)
    {
      FifoFloat_Read(MicrophoneFifo, input, NUMBER_OF_SAMPLES);
      AcquiredDataAreReady = false;

      // Get the max value of the raw data
      DSP_GetMaxValue(input, NUMBER_OF_SAMPLES, &maxOutput);

      if (maxOutput.Value >= 2.0)
      {
        ESP_LOGI(Tag, "Max = %f", maxOutput.Value);
      }

#if 0
      DSP_CalculateDFT(input, outputRe, outputIm, NUMBER_OF_SAMPLES);
      DSP_CalculateDFTOutputMag(outputRe, outputIm, outputMag, (NUMBER_OF_SAMPLES / 2));

      DSP_GetMaxValue(&outputMag[1], ((NUMBER_OF_SAMPLES / 2) - 1), &maxOutput);

      if (maxOutput.Value > 5.0)
      {
        //ESP_LOGI(Tag, "MY count = %f", maxOutput.Value);
        ESP_LOGI(Tag, "Amp = %f, freq = %d [Hz]", maxOutput.Value, (maxOutput.Index + 1) * 40);
      }
#endif
    }

    vTaskDelay(12 / portTICK_PERIOD_MS);
  }
//#endif
}
//#endif
//_____________________________________________________________________________

static int16_t CalculateNoteDuration(T_NoteValue noteValue, T_Tempo tempo)
{
  int16_t tmp = ((60 * 1000) / tempo);
  int16_t duration_ms = 0;

  switch (noteValue)
  {
    case E_NoteValue_TripleCroche:
      duration_ms = (tmp / 8);
      break;

    case E_NoteValue_DoubleCroche:
      duration_ms = (tmp / 4);
      break;

    case E_NoteValue_Croche:
      duration_ms = (tmp / 2);
      break;

    case E_NoteValue_CrochePointee:
      duration_ms = ((tmp * 3) / 4);
      break;

    case E_NoteValue_Noire:
      duration_ms = tmp;
      break;

    case E_NoteValue_NoirePointee:
      duration_ms = ((tmp * 3) / 2);
      break;

    case E_NoteValue_Blanche:
      duration_ms = (tmp * 2);
      break;

    case E_NoteValue_BlancheCroche:
      duration_ms = ((tmp * 5) / 2);
      break;

    case E_NoteValue_BlanchePointee:
      duration_ms = (tmp * 3);
      break;

    case E_NoteValue_Ronde:
      duration_ms = (tmp * 4);
      break;

    default:
      // Do nothing
      break;
  }

  return duration_ms;
}

//_____________________________________________________________________________

static void FeedWatchdog(void)
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed     = 1u;
  TIMERG0.wdt_wprotect = 0u;
}

//_____________________________________________________________________________

static void Callback_TimerGenerator(void* arg)
{
  CosineGenerator_Disable();

  NoteInProgress = false;
  NextNote++;

  if (NextNote == Number)
  {
    NextNote = 0u;
    NextLoop++;

    if (NextLoop == Loop)  // Stop the melody
    {
      NextLoop = 0u;
      MelodyIsFinished = true;
    }
  }
}

//_____________________________________________________________________________
#if 0
static void Callback_TimerAcquisitionTask(void* arg)
{
  BaseType_t higherPriorityTaskWoken = pdFALSE;
  //BaseType_t result;

  vTaskNotifyGiveFromISR(AcquisitionTask, &higherPriorityTaskWoken);
  //result = xTaskNotifyFromISR(TaskToNotify, 0, eIncrement, &higherPriorityTaskWoken);

  // If the call to xTaskNotifyFromISR() returns pdFAIL then the task
  // is not keeping up with the rate at which the timer elapsed.
  //configASSERT(result == pdTRUE);

  //if (higherPriorityTaskWoken != pdFALSE)
  {
    portYIELD_FROM_ISR();
  }
}
#endif
