//_____________________________________________________________________________
//
// Copyright (C) 2019                   Mobsya                   CH-1020 Renens
//_____________________________________________________________________________
//
// PROJECT   Thymio-III
//_____________________________________________________________________________
//
//! \file    efuse_id.h
//! \brief   This module provides the useful functions to read and program the
//!          Thymio3's ID in the ESP32 eFuse.
//!
//!          An ID is based on a lot (2 chars) of prototype or production and a
//!          number (2 bytes) between 0 and 65535.
//!          The lot is used to identify the production batch of the Thymio3.
//!          The number is a 16 bits number (0..65535) that is incremented for
//!          each Thymio3 produced in the same lot.
//!
//!          The lot is a 16 bits number (2 chars) that is defined by the
//!          production or prototype team:
//!          A prototype lot start with "@" followed by a letter (A..Z) for the
//!          prototype version.
//!          A production lot is 2 letters from AA to AZ, BA
//!          to BZ, ..., ZA to ZZ.
//!
//!          This ID is stored in the ESP32 eFuse in 3 entries (DATA2, DATA6,
//!          DATA7) of 32 bits each. As the eFuse can only be programed from 0
//!          to 1, the entries being initially 0x00000000, the first entry is
//!          used to store the ID.
//!          If actual ID must be changed, if the new ID doesn't need change of
//!          any bit from 1 to 0, the new ID can be written in the same entry.
//!          If the new ID needs to change a bit from 0 to 1, the current entry
//!          must be killed (set to 0xFFFFFFFF) and the new ID is written in the
//!          next free entry.
//!          If all entries are used, the ID cannot be changed anymore. Then
//!          killing an entry must a light user choice!!
//!
//! \author  Daniel Burnier
//!
//! \license This project is released under the GNU Lesser General Public License
//_____________________________________________________________________________

#ifndef EFUSE_ID_H_
#define EFUSE_ID_H_

//-----------------------------------------------------------------------------
// Include Section
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Constants/Macros Definitions
//-----------------------------------------------------------------------------

#define NO_ID 0x00000000 // Entry not used and free for use OR no change (lot = 0x0000, pcb_id = 0x0000)
#define INVALID_ID 0xFFFFFFFF // Entry not allowed or killed (lot = 0xFFFF, pcb_id = 0xFFFF)

//-----------------------------------------------------------------------------
// Types Definitions
//-----------------------------------------------------------------------------

typedef union {
    uint32_t id;

    struct {
        char lot[2];
        uint16_t number;
    } parts;
} t3_id_t;

//-----------------------------------------------------------------------------
// Exported Global Data
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Inline Code Definition
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Exported Functions Prototypes
//-----------------------------------------------------------------------------

/**
 * @brief Get id of Thymio3.
 * * This function returns the id of the Thymio3 based on the actual entry in the
 * eFuse.
 * 
 * @return The id of the Thymio3 in the current entry
 * Else 0x00000000 if the first available entry is not used (no id programmed yet)
 * Else 0xFFFFFFFF if all entries are killed
 */
uint32_t getCurrentEfuseID(void);

/**
 * @brief Sets id of Thymio3
 * * This function tries to write the id in the actual entry of the eFuse only if
 * it is possible.
 * 
 * It will not kill the entry if it is already used and cannot be modified with the
 * desired id. The user must check then decide to kill the actual entry to allow the
 * new id to be written in the next entry.
 * 
 * @param id The new id to write in the eFuse.
 * @return code of result
 * 0 if the id was written successfully
 * -1 if the new id is invalid
 * -2 if the entry is already used and cannot be written
 * -3 if there is not anymore free entry
 * -5 if programming the eFuse failed
 * -10 else
 */
int8_t setCurrentEfuseID(uint32_t new_id);

/**
 * @brief Kills the current entry in the eFuse to allow a new id to be written in
 * the next entry.
 * 
 * @return 0 if the entry was killed successfully and there is a free entry to write
 * a new id, -1 if the entry has not been killed because there is no more free entry
 */
int8_t killCurrentEfuseID(void);

/**
 * @brief Returns the available id entries number for the Thymio3 id in the eFuse,
 * actual included, based on the efuse_entries array.
 * 
 * There is a maximum of 8 entries in the eFuse BLK3, but only 3 are usable for the
 * Thymio3 id. Then available entries are DATA2, DATA6 and DATA7, the other entries
 * are used by the ESP32 for other purposes.
 * But effective available entries are defined in the efuse_entries array configura-
 * tion and based on the eFuse contents.
 * 
 * @return The available entries number for the Thymio3 id in the eFuse (0..3)
 */
uint8_t getAvailableEfuseEntries(void);

#endif // EFUSE_ID_H_