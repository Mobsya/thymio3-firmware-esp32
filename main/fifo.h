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
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef FIFO_H_
#define FIFO_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

#include <stdbool.h>
#include <stdint.h>

#include "error.h"

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

struct PrivateFifoBytes;
typedef struct PrivateFifoBytes T_FifoBytes;  //!< Definition of T_FifoBytes type

struct PrivateFifoWords;
typedef struct PrivateFifoWords T_FifoWords;  //!< Definition of T_FifoWords type

struct PrivateFifoFloat;
typedef struct PrivateFifoFloat T_FifoFloat;  //!< Definition of T_FifoFloat type

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the FIFO
//! \pre       None
//! \param     None
//! \return    None
extern void Fifo8bits_Init(void);

//! \brief     Initialize the FIFO
//! \pre       None
//! \param     None
//! \return    None
extern void Fifo16bits_Init(void);

//! \brief     Initialize the FIFO
//! \pre       None
//! \param     None
//! \return    None
extern void FifoFloat_Init(void);

//! \brief     Create a FIFO
//! \pre       First initialize and create the FIFO
//! \param     None
//! \return    None
extern T_FifoBytes* Fifo8bits_Create(uint8_t* buffer, uint16_t bufferSize);

//! \brief     Create a FIFO
//! \pre       First initialize and create the FIFO
//! \param     None
//! \return    None
extern T_FifoWords* Fifo16bits_Create(uint16_t* buffer, uint16_t bufferSize);

//! \brief     Create a FIFO
//! \pre       First initialize and create the FIFO
//! \param     None
//! \return    None
extern T_FifoFloat* FifoFloat_Create(float* buffer, uint16_t bufferSize);

//! \brief     Reset the FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO to reset
//! \return    None
extern void Fifo8bits_Reset(T_FifoBytes* fifo);

//! \brief     Reset the FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO to reset
//! \return    None
extern void Fifo16bit_Reset(T_FifoWords* fifo);

//! \brief     Reset the FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO to reset
//! \return    None
extern void FifoFloat_Reset(T_FifoFloat* fifo);

//! \brief     Peek data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are peeked
//! \param     data - Data to be peeked
//! \param     size - Size of the data
//! \return    None
extern void Fifo8bits_Peek(T_FifoBytes* fifo, uint8_t* data, uint16_t size);

//! \brief     Peek data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are peeked
//! \param     data - Data to be peeked
//! \param     size - Size of the data
//! \return    None
extern void Fifo16bits_Peek(T_FifoWords* fifo, uint16_t* data, uint16_t size);

//! \brief     Peek data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are peeked
//! \param     data - Data to be peeked
//! \param     size - Size of the data
//! \return    None
extern void FifoFloat_Peek(T_FifoFloat* fifo, float* data, uint16_t size);

//! \brief     Write data bytes to FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are written
//! \param     src - Data to be written
//! \param     size - Size of the data
//! \return    None
extern void Fifo8bits_Write(T_FifoBytes* fifo, uint8_t* src, uint16_t size);

//! \brief     Write data bytes to FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are written
//! \param     src - Data to be written
//! \param     size - Size of the data
//! \return    None
extern void Fifo16bits_Write(T_FifoWords* fifo, uint16_t* src, uint16_t size);

//! \brief     Write data bytes to FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are written
//! \param     src - Data to be written
//! \param     size - Size of the data
//! \return    None
extern void FifoFloat_Write(T_FifoFloat* fifo, float* src, uint16_t size);

//! \brief     Read data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are read
//! \param     dest - Data to be read
//! \param     size - Size of the data
//! \return    None
extern void Fifo8bits_Read(T_FifoBytes* fifo, uint8_t* dest, uint16_t size);

//! \brief     Read data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are read
//! \param     dest - Data to be read
//! \param     size - Size of the data
//! \return    None
extern void Fifo16bits_Read(T_FifoWords* fifo, uint16_t* dest, uint16_t size);

//! \brief     Read data from FIFO
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the data are read
//! \param     dest - Data to be read
//! \param     size - Size of the data
//! \return    None
extern void FifoFloat_Read(T_FifoFloat* fifo, float* dest, uint16_t size);

//! \brief     Return the number of elements that are in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of element is calculated
//! \return    Number of elements that are in the FIFO
extern uint16_t Fifo8bits_GetNumberOfElements(T_FifoBytes* fifo);

//! \brief     Return the number of elements that are in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of element is calculated
//! \return    Number of elements that are in the FIFO
extern uint16_t Fifo16bits_GetNumberOfElements(T_FifoWords* fifo);

//! \brief     Return the number of elements that are in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of element is calculated
//! \return    Number of elements that are in the FIFO
extern uint16_t FifoFloat_GetNumberOfElements(T_FifoFloat* fifo);

//! \brief     Return the number of free space in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of frees space is calculated
//! \return    Number of free space that are in the FIFO
extern uint16_t Fifo8bits_GetFreeSpace(T_FifoBytes* fifo);

//! \brief     Return the number of free space in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of frees space is calculated
//! \return    Number of free space that are in the FIFO
extern uint16_t Fifo16bits_GetFreeSpace(T_FifoWords* fifo);

//! \brief     Return the number of free space in the FIFO queue
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the number of frees space is calculated
//! \return    Number of free space that are in the FIFO
extern uint16_t FifoFloat_GetFreeSpace(T_FifoFloat* fifo);

//! \brief     Check if the buffer it is full
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is full, false otherwise
extern bool Fifo8bits_IsFull(T_FifoBytes* fifo);

//! \brief     Check if the buffer it is full
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is full, false otherwise
extern bool Fifo16bits_IsFull(T_FifoWords* fifo);

//! \brief     Check if the buffer it is full
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is full, false otherwise
extern bool FifoFloat_IsFull(T_FifoFloat* fifo);

//! \brief     Check if the buffer it is empty
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is empty, false otherwise
extern bool Fifo8bits_IsEmpty(T_FifoBytes* fifo);

//! \brief     Check if the buffer it is empty
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is empty, false otherwise
extern bool Fifo16bits_IsEmpty(T_FifoWords* fifo);

//! \brief     Check if the buffer it is empty
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the test is performed
//! \return    True if the FIFO is empty, false otherwise
extern bool FifoFloat_IsEmpty(T_FifoFloat* fifo);

//! \brief     Set the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    None
extern void Fifo8bits_SetConsumePosition(T_FifoBytes* fifo, uint16_t position);

//! \brief     Set the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    None
extern void Fifo16bits_SetConsumePosition(T_FifoWords* fifo, uint16_t position);

//! \brief     Set the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    None
extern void FifoFloat_SetConsumePosition(T_FifoFloat* fifo, uint16_t position);

//! \brief     Get the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    The consume position
extern uint16_t Fifo8bits_GetConsumePosition(T_FifoBytes* fifo);

//! \brief     Get the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    The consume position
extern uint16_t Fifo16bits_GetConsumePosition(T_FifoWords* fifo);

//! \brief     Get the consume position
//! \pre       First initialize and create the FIFO
//! \param     fifo - FIFO on which the action is performed
//! \return    The consume position
extern uint16_t FifoFloat_GetConsumePosition(T_FifoFloat* fifo);

#endif // FIFO_H_
