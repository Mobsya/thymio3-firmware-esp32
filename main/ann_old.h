//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    ann.h
//! \brief   This module provides the useful functions to use the artificial neural network mode
//!
//! \author  Stefano Morgani, based on the python script developed by Valentina Ferraioli
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef ANN_H_
#define ANN_H_

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

//! \brief     Initialize the ANN mode
//! \pre       None
//! \param     None
//! \return    None
extern void ANN_Init(void);

//! \brief     Start the ANN mode
//! \pre       First initialize the ANN mode
//! \param     None
//! \return    None
extern void ANN_Start(void);

//! \brief     Stop the ANN mode
//! \pre       First initialize the ANN mode
//! \param     None
//! \return    None
extern void ANN_Stop(void);

//! \brief     Run the ANN mode
//! \pre       First initialize the ANN mode
//! \param     None
//! \return    None
extern void ANN_Run(void);

#endif // ANN_H_
