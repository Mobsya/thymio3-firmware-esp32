//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    audio.c
//! \brief   This module provides the useful functions to play sounds
//!
//! \author  Vincent Gonet
//!
//! \version $Id: audio.c 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <math.h>

#include "esp_log.h"

#include "driver/dac.h"

#include "audio.h"

#include "board.h"
#include "timer_hw.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define BUFFER_SIZE 600

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef struct
{
  uint32_t* PreviousItem;							// used in linked list of wavs
  uint32_t* NextItem;								// used in linked list of wavs
  T_Wav* Wav;
} T_PlayListItem;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "audio";

volatile uint32_t NextFillPos=0;								// position in buffer of next byte to fill
volatile int32_t NextPlayPos=0;									// position in buffer of next byte to play
volatile int32_t EndFillPos=BUFFER_SIZE;						// position in buffer of last byte+1 that can be filled
volatile uint8_t LastDacValue;									// Next Idx pos in buffer to send to DAC
volatile uint8_t Buffer[BUFFER_SIZE];							// The buffer to store the data that will be sent to the
volatile uint8_t DacPin;                              			// pin to send DAC data to, presumably one of the DAC pins!
volatile T_PlayListItem* FirstPlayListItem=0;          	// first play list item to play in linked list
volatile uint16_t BufferUsedCount=0;							// how much buffer used since last buffer fill

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

static uint8_t MixBytesToPlay(void);

static uint8_t IRAM_ATTR NextByte(T_Wav* wav);

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Audio_Init(T_Wav* wav, uint8_t* wavData)
{
  wav->SampleRate   = (wavData[25] * 256) + wavData[24];
  wav->DataSize     = (wavData[42] * 65536) + (wavData[41] * 256) + wavData[40] + 44;
  wav->IncreaseBy   = (float)(wav->SampleRate) / 50000; //float(Wav->SampleRate) / 50000;
  wav->Data         = wavData;
  wav->Count        = 0;
  wav->LastIntCount = 0;
  wav->DataIdx      = 44;
  wav->Completed    = true;

  FirstPlayListItem = 0;
  LastDacValue = 0x7F;

  TimerHw_StartAudioTimer20us();
  dac_output_enable(DAC_CHANNEL_1);
}

#if 0
void Audio_Init(T_Wav* wav, uint8_t* wavData)
{
  wav->SampleRate   = (wavData[25] * 256) + wavData[24];
  wav->DataSize     = (wavData[42] * 65536) + (wavData[41] * 256) + wavData[40] + 44;
  wav->IncreaseBy   = float32(Wav->SampleRate) / 50000;
  wav->Data         = wavData;
  wav->Count        = 0;
  wav->LastIntCount = 0;
  wav->DataIdx      = 44;
  wav->Completed    = true;

  TimerHw_StartAudioTimer20us();
  dac_output_enable(DAC_CHANNEL_1);

#if 0
  Buffer[0] = 255;
  Buffer[1] = 150;
  Buffer[2] = 200;
  Buffer[3] = 100;
  Buffer[4] = 200;
  Buffer[5] = 100;
  Buffer[6] = 200;
  Buffer[7] = 100;
  Buffer[8] = 200;
  Buffer[9] = 100;
  Buffer[10] = 200;
  Buffer[11] = 100;
  Buffer[12] = 200;
  Buffer[13] = 100;
  Buffer[14] = 200;
  Buffer[15] = 100;
  Buffer[16] = 200;
  Buffer[17] = 20;
  Buffer[18] = 120;
  Buffer[19] = 100;
#endif
}
#endif

//_____________________________________________________________________________

void Audio_FillBuffer(void)
{
  // Fill buffer with the sound to output

  if ((NextFillPos == BUFFER_SIZE) & (EndFillPos != 0) & (NextPlayPos != 0))
  {
    NextFillPos = 0;
  }

  //ESP_LOGI(Tag, "SALUT %p, %d, %d", FirstPlayListItem, NextFillPos, EndFillPos);

  // If there are items that need to be played & room for more in buffer
  while ((FirstPlayListItem != 0) & (NextFillPos != EndFillPos) & (NextFillPos != BUFFER_SIZE))
  {
	//ESP_LOGI(Tag, "COUCOU");
    Buffer[NextFillPos] = MixBytesToPlay();  // Get the byte to play and put into buffer
    NextFillPos++;	// move to next buffer position

    if (NextFillPos != EndFillPos)
	{
	  if (NextFillPos == BUFFER_SIZE)
	  {
        NextFillPos = 0;
	  }
	}
  }
}

//_____________________________________________________________________________

void Audio_PlayWav(T_Wav* wav)
{
  //if(Mix==false)  // stop all currently playing sounds and just have this one
  //	StopAllSounds();
  //Serial.println("hh");
  //return;
  // set up this wav to play
  wav->LastIntCount = 0;
  wav->DataIdx = 44;
  wav->Count = 0;

  // fine to here
  // add to list of currently playing wavs
  if (FirstPlayListItem == 0) // no items to play in list yet
  {
    //FirstPlayListItem=new XT_PlayListItem_Class();  // FIXME
	FirstPlayListItem = malloc(sizeof(FirstPlayListItem));
	//ESP_LOGI(Tag, "Hello %p", FirstPlayListItem);
	FirstPlayListItem->Wav = wav;  // FIXME
    //wav->ParentPlayListItem = FirstPlayListItem;  // FIXME
  }
  else
  {
    // add to end of list
  }

  wav->Completed = false; 					// Will start it playing
}

#if 0
void Audio_PlayWav(T_Wav* wav, bool mix)
{
  //if(Mix==false)  // stop all currently playing sounds and just have this one
  //	StopAllSounds();
  //Serial.println("hh");
  //return;
  // set up this wav to play
  wav->LastIntCount=0;
  wav->DataIdx=44;
  wav->Count=0;

  // fine to here
  // add to list of currently playing wavs
  if (FirstPlayListItem == 0) // no items to play in list yet
  {
    //FirstPlayListItem=new XT_PlayListItem_Class();  // FIXME
	FirstPlayListItem->Wav = wav;
    //wav->ParentPlayListItem = FirstPlayListItem;  // FIXME
  }
  else
  {
    // add to end of list
  }

  wav->Completed = false; 					// Will start it playing
}
#endif
//_____________________________________________________________________________

static uint8_t MixBytesToPlay(void)
{
  // goes through sounds, gets the bytes from each and mixes them together, returning a final byte

  // mix all sounds together in the list of sounds to play and then output that one mixed value
  // To mix waves you add the waves together.... That is presuming they are "correct" waves
  // that have both positive and negative peaks and troughs. However these wav files are saved
  // as 8bit unsigned, so therefore there is no negative, we need to adjust them back to having
  // correct positive and ngative peaks and troughs first before doing the "mixing", this is a
  // trivial task

  volatile T_PlayListItem* playItem;
  uint8_t byteToPlay = 0;

  playItem = FirstPlayListItem;
  //while(PlayItem!=0)
  {
    if (playItem->Wav->Completed == false)
	//if (Wav->Completed == false)
    {
	  byteToPlay = NextByte(playItem->Wav);
    }

    playItem = playItem->NextItem;
  }

  return byteToPlay;
}

//_____________________________________________________________________________

static uint8_t IRAM_ATTR NextByte(T_Wav* wav)
{
  // Returns the next byte to be played, note that this routine will return values suitable to
  // be played back at 50,000Hz. Even if this sample is at a lesser rate than that it will be
  // padded out as required so that it will appear to have a 50Khz sample rate

  // Note it is up to the calling routine to check if this WAV file has NOT completed playing
  // before calling. If you call it and it has completed playing then it will always return
  // 0x7F (speaker mid point).
  uint16_t IntPartOfCount;
  uint8_t ReturnValue;

  if (wav->Completed)
  {
	return 0x7f;
  }

  // increase the counter, if it goes to a new integer digit then write to DAC
  wav->Count += wav->IncreaseBy;
  IntPartOfCount = floor(wav->Count);
  ReturnValue = wav->Data[wav->DataIdx];				// by default we return previous value;

  if (IntPartOfCount > wav->LastIntCount)
  {
	// gone to a new integer of count, we need to send a new value to the DAC
	wav->LastIntCount = IntPartOfCount; // crashes on this line with panic
	ReturnValue = wav->Data[wav->DataIdx];
	wav->DataIdx++;

	if (wav->DataIdx >= wav->DataSize)  				// end of data, flag end
	{
	  wav->Count = 0;						// reset frequency counter
	  wav->DataIdx = 44;						// reset data pointer back to beginning of WAV data
	  // remove wav from play list
	  wav->Completed=true;  				// mark as completed
	  //XTDacAudioClassGlobalObject->RemoveFromPlayList(ParentPlayListItem); FIXME
	}
  }

  return ReturnValue;
}

//_____________________________________________________________________________

void TimerHw_CallbackAudio20us(void* arg)
{
  //if (NextPlayPos != NextFillPos)  // If not up against the next fill position then valid data to play
  {
    if (LastDacValue != Buffer[NextPlayPos])  // Send value to DAC only of changed since last value else no need
    {
	  // Value to DAC has changed, send to actual hardware, else we just leave setting as is as it's not changed
	  LastDacValue = Buffer[NextPlayPos];
	  dac_output_voltage(DAC_CHANNEL_1, LastDacValue);	// Write out the data
    }

    NextPlayPos++;  // Move play position to next byte in buffer

    if (NextPlayPos == BUFFER_SIZE)  // If gone past end of buffer,
    {
      NextPlayPos = 0;	// Set back to beginning
    }

    EndFillPos = (NextPlayPos - 1);  // Move area where we can fill up to to NextPlayPos

    if (EndFillPos < 0)  // check if less than zero, if so then 1 less is
    {
      EndFillPos = BUFFER_SIZE;  // BUFFER_SIZE (we can fill up to one less than this)
	}
  }

  if (FirstPlayListItem != 0)  // sounds in Q
  {
    BufferUsedCount++;
  }
}
