//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    line_tracker.h
//! \brief   This module provides the useful functions to use the line tracker mode
//!
//! \author  Vincent Gonet
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef LINE_TRACKER_H_
#define LINE_TRACKER_H_

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

//! \brief     Initialize the line tracker mode
//! \pre       None
//! \param     None
//! \return    None
extern void LineTracker_Init(void);

//! \brief     Start the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void LineTracker_Start(void);

//! \brief     Stop the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void LineTracker_Stop(void);

//! \brief     Run the line tracker mode
//! \pre       First initialize the line tracker mode
//! \param     None
//! \return    None
extern void LineTracker_Run(void);

#endif // LINE_TRACKER_H_
