/**
 * @file Disassembler.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for disassembler in kernel
 * @details
 *
 * @version 0.3
 * @date 2023-06-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

//
// Most of the functions are defined and exported
//

BOOLEAN
GenerateLbrEntry(UINT64 CodeBaseVa, UINT8 * GuestCode, UINT64 GuestRip, PLBR_STACK_ENTRY OutEntries, UINT8 * OutCount);
