//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    fifo.h
//! \brief   This module provides the useful functions to use the FIFO
//!
//! \author  Vincent Gonet
//!
//! \version $Id: fifo.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <stdlib.h>

#include "esp_log.h"

#include "fifo.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define MAX_FIFO_BYTES_ALLOWED  4U  //!< Maximum number of FIFO for bytes allowed

#define MAX_FIFO_WORDS_ALLOWED  4U  //!< Maximum number of FIFO for words allowed

#define MAX_FIFO_FLOAT_ALLOWED  4U  //!< Maximum number of FIFO for float allowed

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//! \details The description of the FIFO for bytes
struct PrivateFifoBytes
{
  uint16_t Size;     //!< Size of the buffer
  uint16_t Insert;   //!< Index of the buffer used for writing
  uint16_t Consume;  //!< Index of the buffer used for reading
  uint8_t* Buffer;   //!< Buffer used to store the data
  bool     IsUsed;   //!< Flag that indicates if the FIFO is used
};

//! \details The description of the FIFO for words
struct PrivateFifoWords
{
  uint16_t  Size;     //!< Size of the buffer
  uint16_t  Insert;   //!< Index of the buffer used for writing
  uint16_t  Consume;  //!< Index of the buffer used for reading
  uint16_t* Buffer;   //!< Buffer used to store the data
  bool      IsUsed;   //!< Flag that indicates if the FIFO is used
};

//! \details The description of the FIFO for words
struct PrivateFifoFloat
{
  uint16_t Size;     //!< Size of the buffer
  uint16_t Insert;   //!< Index of the buffer used for writing
  uint16_t Consume;  //!< Index of the buffer used for reading
  float*  Buffer;   //!< Buffer used to store the data
  bool     IsUsed;   //!< Flag that indicates if the FIFO is used
};

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Private Data
//-----------------------------------------------------------------------------

static const char* Tag = "fifo";

static T_FifoBytes TableFifoBytes[MAX_FIFO_BYTES_ALLOWED];  //!< Table containing the FIFO for bytes created

static T_FifoWords TableFifoWords[MAX_FIFO_BYTES_ALLOWED];  //!< Table containing the FIFO for words created

static T_FifoFloat TableFifoFloat[MAX_FIFO_FLOAT_ALLOWED];  //!< Table containing the FIFO for float created

//-----------------------------------------------------------------------------
// Private Functions Prototypes
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Functions Implementation
//-----------------------------------------------------------------------------

void Fifo8bits_Init(void)
{
  // Initialize the table timers
  for (uint16_t index = 0u; index < MAX_FIFO_BYTES_ALLOWED; index++)
  {
    TableFifoBytes[index].Size    = 0u;
    TableFifoBytes[index].Insert  = 0u;
    TableFifoBytes[index].Consume = 0u;
    TableFifoBytes[index].Size    = false;
  }

  ESP_LOGI(Tag, "FIFO 8-bits is initialized");
}

//_____________________________________________________________________________

void Fifo16bits_Init(void)
{
  // Initialize the table timers
  for (uint16_t index = 0u; index < MAX_FIFO_WORDS_ALLOWED; index++)
  {
    TableFifoWords[index].Size    = 0u;
    TableFifoWords[index].Insert  = 0u;
    TableFifoWords[index].Consume = 0u;
    TableFifoWords[index].Size    = false;
  }

  ESP_LOGI(Tag, "FIFO 16-bits is initialized");
}

//_____________________________________________________________________________

void FifoFloat_Init(void)
{
  // Initialize the table timers
  for (uint16_t index = 0u; index < MAX_FIFO_FLOAT_ALLOWED; index++)
  {
    TableFifoFloat[index].Size    = 0u;
    TableFifoFloat[index].Insert  = 0u;
    TableFifoFloat[index].Consume = 0u;
    TableFifoFloat[index].Size    = false;
  }

  ESP_LOGI(Tag, "FIFO Float is initialized");
}

//_____________________________________________________________________________

T_FifoBytes* Fifo8bits_Create(uint8_t* buffer, uint16_t bufferSize)
{
  T_FifoBytes* fifo = NULL;

  for (uint16_t index = 0u; index < MAX_FIFO_BYTES_ALLOWED; index++)
  {
    if (!TableFifoBytes[index].IsUsed)
    {
      fifo = &TableFifoBytes[index];

      fifo->IsUsed    = true;
      fifo->Size      = bufferSize;
      fifo->Insert    = 0u;
      fifo->Consume   = 0u;
      fifo->Buffer    = buffer;
      break;
    }
  }

  ESP_LOGI(Tag, "New FIFO 8-bits is created");

  return fifo;
}

//_____________________________________________________________________________

T_FifoWords* Fifo16bits_Create(uint16_t* buffer, uint16_t bufferSize)
{
  T_FifoWords* fifo = NULL;

  for (uint16_t index = 0u; index < MAX_FIFO_WORDS_ALLOWED; index++)
  {
    if (!TableFifoWords[index].IsUsed)
    {
      fifo = &TableFifoWords[index];

      fifo->IsUsed    = true;
      fifo->Size      = bufferSize;
      fifo->Insert    = 0u;
      fifo->Consume   = 0u;
      fifo->Buffer    = buffer;
      break;
    }
  }

  ESP_LOGI(Tag, "New FIFO 16-bits is created");

  return fifo;
}

//_____________________________________________________________________________

T_FifoFloat* FifoFloat_Create(float* buffer, uint16_t bufferSize)
{
  T_FifoFloat* fifo = NULL;

  for (uint16_t index = 0u; index < MAX_FIFO_FLOAT_ALLOWED; index++)
  {
    if (!TableFifoFloat[index].IsUsed)
    {
      fifo = &TableFifoFloat[index];

      fifo->IsUsed    = true;
      fifo->Size      = bufferSize;
      fifo->Insert    = 0u;
      fifo->Consume   = 0u;
      fifo->Buffer    = buffer;
      break;
    }
  }

  ESP_LOGI(Tag, "New FIFO Float is created");

  return fifo;
}

//_____________________________________________________________________________

void Fifo8bits_Reset(T_FifoBytes* fifo)
{
  fifo->Insert  = 0u;
  fifo->Consume = 0u;
}

//_____________________________________________________________________________

void Fifo16bits_Reset(T_FifoWords* fifo)
{
  fifo->Insert  = 0u;
  fifo->Consume = 0u;
}

//_____________________________________________________________________________

void FifoFloat_Reset(T_FifoFloat* fifo)
{
  fifo->Insert  = 0u;
  fifo->Consume = 0u;
}

//_____________________________________________________________________________

void Fifo8bits_Peek(T_FifoBytes* fifo, uint8_t* data, uint16_t size)
{
  uint16_t consume = fifo->Consume;

  while (size > 0u)
  {
    *data = fifo->Buffer[consume];
    data++;

    if (consume != fifo->Size)
    {
      consume++;
    }
    else
    {
      consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void Fifo16bits_Peek(T_FifoWords* fifo, uint16_t* data, uint16_t size)
{
  uint16_t consume = fifo->Consume;

  while (size > 0u)
  {
    *data = fifo->Buffer[consume];
    data++;

    if (consume != fifo->Size)
    {
      consume++;
    }
    else
    {
      consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void FifoFloat_Peek(T_FifoFloat* fifo, float* data, uint16_t size)
{
  uint16_t consume = fifo->Consume;

  while (size > 0u)
  {
    *data = fifo->Buffer[consume];
    data++;

    if (consume != fifo->Size)
    {
      consume++;
    }
    else
    {
      consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void Fifo8bits_Write(T_FifoBytes* fifo, uint8_t* src, uint16_t size)
{
  while (size > 0u)
  {
    fifo->Buffer[fifo->Insert] = *src;
    src++;
    fifo->Insert++;

    if (fifo->Insert == fifo->Size)
    {
      fifo->Insert = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void Fifo16bits_Write(T_FifoWords* fifo, uint16_t* src, uint16_t size)
{
  while (size > 0u)
  {
    fifo->Buffer[fifo->Insert] = *src;
    src++;
    fifo->Insert++;

    if (fifo->Insert == fifo->Size)
    {
      fifo->Insert = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void FifoFloat_Write(T_FifoFloat* fifo, float* src, uint16_t size)
{
  while (size > 0u)
  {
    fifo->Buffer[fifo->Insert] = *src;
    src++;
    fifo->Insert++;

    if (fifo->Insert == fifo->Size)
    {
      fifo->Insert = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void Fifo8bits_Read(T_FifoBytes* fifo, uint8_t* dest, uint16_t size)
{
  while (size > 0u)
  {
    *dest = fifo->Buffer[fifo->Consume];
    dest++;
    fifo->Consume++;

    if (fifo->Consume == fifo->Size)
    {
      fifo->Consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void Fifo16bits_Read(T_FifoWords* fifo, uint16_t* dest, uint16_t size)
{
  while (size > 0u)
  {
    *dest = fifo->Buffer[fifo->Consume];
    dest++;
    fifo->Consume++;

    if (fifo->Consume == fifo->Size)
    {
      fifo->Consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

void FifoFloat_Read(T_FifoFloat* fifo, float* dest, uint16_t size)
{
  while (size > 0u)
  {
    *dest = fifo->Buffer[fifo->Consume];
    dest++;
    fifo->Consume++;

    if (fifo->Consume == fifo->Size)
    {
      fifo->Consume = 0u;
    }

    size--;
  }
}

//_____________________________________________________________________________

uint16_t Fifo8bits_GetNumberOfElements(T_FifoBytes* fifo)
{
  uint16_t numberOfElements;

  // ipos and cpos are created to avoid the fifo->Insert and fifo->Consume
  // values change during the execution of the function
  uint16_t ipos = fifo->Insert;
  uint16_t cpos = fifo->Consume;

  if (ipos >= cpos)
  {
    numberOfElements = (ipos - cpos);
  }
  else
  {
    numberOfElements = ((fifo->Size - cpos) + ipos);
  }

  //ESP_LOGI(Tag, "Insert = %d, Consume = %d", ipos, cpos);

  return numberOfElements;
}

//_____________________________________________________________________________

uint16_t Fifo16bits_GetNumberOfElements(T_FifoWords* fifo)
{
  uint16_t numberOfElements;

  // ipos and cpos are created to avoid the fifo->Insert and fifo->Consume
  // values change during the execution of the function
  uint16_t ipos = fifo->Insert;
  uint16_t cpos = fifo->Consume;

  if (ipos >= cpos)
  {
    numberOfElements = (ipos - cpos);
  }
  else
  {
    numberOfElements = ((fifo->Size - cpos) + ipos);
  }

  //ESP_LOGI(Tag, "Insert = %d, Consume = %d", ipos, cpos);

  return numberOfElements;
}

//_____________________________________________________________________________

uint16_t FifoFloat_GetNumberOfElements(T_FifoFloat* fifo)
{
  uint16_t numberOfElements;

  // ipos and cpos are created to avoid the fifo->Insert and fifo->Consume
  // values change during the execution of the function
  uint16_t ipos = fifo->Insert;
  uint16_t cpos = fifo->Consume;

  if (ipos >= cpos)
  {
    numberOfElements = (ipos - cpos);
  }
  else
  {
    numberOfElements = ((fifo->Size - cpos) + ipos);
  }

  //ESP_LOGI(Tag, "Insert = %d, Consume = %d", ipos, cpos);

  return numberOfElements;
}

//_____________________________________________________________________________

uint16_t Fifo8bits_GetFreeSpace(T_FifoBytes* fifo)
{
  return ((fifo->Size - 1u) - Fifo8bits_GetNumberOfElements(fifo));
}

//_____________________________________________________________________________

uint16_t Fifo16bits_GetFreeSpace(T_FifoWords* fifo)
{
  return ((fifo->Size - 1u) - Fifo16bits_GetNumberOfElements(fifo));
}

//_____________________________________________________________________________

uint16_t FifoFloat_GetFreeSpace(T_FifoFloat* fifo)
{
  return ((fifo->Size - 1u) - FifoFloat_GetNumberOfElements(fifo));
}

//_____________________________________________________________________________

bool Fifo8bits_IsFull(T_FifoBytes* fifo)
{
  return (Fifo8bits_GetFreeSpace(fifo) == 0u);
}

//_____________________________________________________________________________

bool Fifo16bits_IsFull(T_FifoWords* fifo)
{
  return (Fifo16bits_GetFreeSpace(fifo) == 0u);
}

//_____________________________________________________________________________

bool FifoFloat_IsFull(T_FifoFloat* fifo)
{
  return (FifoFloat_GetFreeSpace(fifo) == 0u);
}

//_____________________________________________________________________________

bool Fifo8bits_IsEmpty(T_FifoBytes* fifo)
{
  return (fifo->Insert == fifo->Consume);
}

//_____________________________________________________________________________

bool Fifo16bits_IsEmpty(T_FifoWords* fifo)
{
  return (fifo->Insert == fifo->Consume);
}

//_____________________________________________________________________________

bool FifoFloat_IsEmpty(T_FifoFloat* fifo)
{
  return (fifo->Insert == fifo->Consume);
}

//_____________________________________________________________________________

void Fifo8bits_SetConsumePosition(T_FifoBytes* fifo, uint16_t position)
{
  fifo->Consume = position;
}

//_____________________________________________________________________________

void Fifo16bits_SetConsumePosition(T_FifoWords* fifo, uint16_t position)
{
  fifo->Consume = position;
}

//_____________________________________________________________________________

void FifoFloat_SetConsumePosition(T_FifoFloat* fifo, uint16_t position)
{
  fifo->Consume = position;
}

//_____________________________________________________________________________

uint16_t Fifo8bits_GetConsumePosition(T_FifoBytes* fifo)
{
  return fifo->Consume;
}

//_____________________________________________________________________________

uint16_t Fifo16bits_GetConsumePosition(T_FifoWords* fifo)
{
  return fifo->Consume;
}

//_____________________________________________________________________________

uint16_t FifoFloat_GetConsumePosition(T_FifoFloat* fifo)
{
  return fifo->Consume;
}
