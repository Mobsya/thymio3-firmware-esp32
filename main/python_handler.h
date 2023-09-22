//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    python_handler.h
//! \brief   This module provides the ability to start and stop predefined python scripts or REPL
//!
//! \author  Stefano Morgani
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef PYTHON_HANDLER_H_
#define PYTHON_HANDLER_H_

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

//! \brief     Initialize the python handler mode
//! \pre       None
//! \param     None
//! \return    None
extern void PythonHandler_Init(void);

//! \brief     Start the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void PythonHandler_Start(void);

//! \brief     Stop the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void PythonHandler_Stop(void);

//! \brief     Run the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void PythonHandler_Run(uint8_t id);

#endif // PYTHON_HANDLER_H_
