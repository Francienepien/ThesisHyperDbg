/**
 * @file VmxFootprints.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hide the debugger from VMX-footprints of anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.14
 * @date 2024-08-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		            Constants                   //
//////////////////////////////////////////////////
#define MSR_SMI_COUNT 0x00000034