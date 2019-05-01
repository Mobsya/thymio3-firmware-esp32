//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    comm.h
//! \brief   This module provides the useful functions to communicate
//!
//! \author  Vincent Gonet
//!
//! \version $Id: comm.h 18076 2017-04-20 12:28:12Z v.gonet $
//_____________________________________________________________________________

#ifndef COMM_H_
#define COMM_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

//! \brief     Initialize the communication
//! \pre       None
//! \param     None
//! \return    None
extern void Comm_Init(void);

//! \brief     Start the communication task
//! \pre       First initialize the communication
//! \param     None
//! \return    None
extern void Comm_Start(void);

//! \brief     Check that the I2C bus is available
//! \pre       First initialize the communication
//! \param     None
//! \return    True if the I2C bus is available, false otherwise
extern bool Comm_IsBusAvailable(void);

#endif // COMM_H_
