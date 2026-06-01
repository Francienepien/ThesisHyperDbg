/**
 * @file VmxFootprint.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hide the debugger from VMX-footrpints of anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.14
 * @date 2024-08-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Some of the functions are exported in the module
//

#define IN_MSR_RANGE(msr, base, cap) ((msr) >= (base) && (msr) <= ((base) + (cap)))

#define MSR_SMI_COUNT 0x00000034

VOID
SaveMsrValueToRegisters(PGUEST_REGS Regs, UINT64 MsrValue);

UINT64
GetMsrValueFromRegisters(PGUEST_REGS Regs);