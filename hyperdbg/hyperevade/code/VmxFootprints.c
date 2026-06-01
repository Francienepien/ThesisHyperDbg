/**
 * @file VmxFootprints.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Try to hide VMX methods from anti-debugging and anti-hypervisor
 * @details
 * @version 0.14
 * @date 2025-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handle Cpuid Vmexits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param CpuInfo The temporary logical processor registers
 *
 * @return VOID
 */
VOID
TransparentCheckAndModifyCpuid(PGUEST_REGS Regs, INT32 CpuInfo[])
{
    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        //
        // Unset the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        CpuInfo[2] &= ~HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS || Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // When transparent, all CPUID leaves in the 0x40000000+ range should contain no usable data
        //
        CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0x40000000;
    }
}

/**
 * @brief Handle RDMSR VM exits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param TargetMsr Target MSR in ECX register
 *
 * @return BOOLEAN Whether the emulation should be further continued or not
 */
BOOLEAN
TransparentCheckAndModifyMsrRead(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    //
    // The MSR range between 40000000H and 400000F0H is reserved and usually used by hypervisors
    // when the guest operating system is Windows to indicate the OS identifier
    //
    // Sina: Needs more investigation since injecting #GP on Nested-virtualization environments
    // will crash the VM on Meteor Lake processors since the OS expects to use synthetic timers
    // (HV_REGISTER_STIMER0_CONFIG and HV_REGISTER_STIMER0_COUNT) to receive interrupts
    // Ref: https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/tlfs/timers
    //
    // if (TargetMsr >= RESERVED_MSR_RANGE_LOW && TargetMsr <= RESERVED_MSR_RANGE_HI)
    // {
    //     LogInfo("RDMSR attempts to write to a reserved MSR range. MSR: %x",
    //             TargetMsr);
    //
    //     g_Callbacks.EventInjectGeneralProtection();
    //     return TRUE; // Should not emulate further
    // }
    // else
    // {
    //     //
    //     // Not handled in the transparent-mode
    //     //
    //     return FALSE;
    // }

    switch (TargetMsr)
    {
    case MSR_SMI_COUNT:
    {
        //
        // Spoof SMI value to hide hypervisor
        //
        SaveMsrValueToRegisters(Regs, g_TransparentSmiCount);

        return TRUE;
    }
    }

    return FALSE;
}

/**
 * @brief Handle WRMSR VM exits when the Transparent mode is enabled
 *
 * @param Regs The virtual processor's state of registers
 * @param TargetMsr Target MSR in ECX register
 *
 * @return BOOLEAN Whether the emulation should be further continued or not
 */
BOOLEAN
TransparentCheckAndModifyMsrWrite(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    // if (TargetMsr >= RESERVED_MSR_RANGE_LOW && TargetMsr <= RESERVED_MSR_RANGE_HI)
    // {
    //     //
    //     // The MSR range between 40000000H and 400000F0H is reserved and usually used by hypervisors
    //     // when the guest operating system is Windows to indicate the OS identifier
    //     //
    //
    //     LogInfo("WRMSR attempts to write to a reserved MSR range. MSR: %x, rax: %llx, rdx: %llx",
    //             TargetMsr,
    //             Regs->rax,
    //             Regs->rdx);
    //
    //     g_Callbacks.EventInjectGeneralProtection();
    //
    //     return TRUE; // Should not emulate further
    // }
    // else
    // {
    //     //
    //     // Not handled in the transparent-mode
    //     //
    //     return FALSE;
    // }

    UINT64            MsrValue;
    UINT64            DebugCtlValue;

    MsrValue = GetMsrValueFromRegisters(Regs);

    switch (TargetMsr)
    {
    case IA32_DEBUGCTL:
    {
        //
        // Only allow setting bit 0
        //
        if (MsrValue & 1)
        {
            DebugCtlValue = CpuReadMsr(IA32_DEBUGCTL);
            CpuWriteMsr(IA32_DEBUGCTL, DebugCtlValue |= 1);
        }
        else
        {
            DebugCtlValue = CpuReadMsr(IA32_DEBUGCTL);
            CpuWriteMsr(IA32_DEBUGCTL, DebugCtlValue & ~1);
        }

        return TRUE;
    }
    case MSR_LEGACY_LBR_SELECT:
    {
        //
        // Guest tries to set illegal filter
        //
        if ((MsrValue & UPPER_56_BITS) != 0)
        {
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }

        CpuWriteMsr(TargetMsr, MsrValue);
    }
    case MSR_LBR_TOS:
    case IA32_LBR_CTL:
    {
        CpuWriteMsr(TargetMsr, MsrValue);

        return TRUE;
    }
    }

    if (!g_IsLbrSupported)
    {
        return FALSE;
    }

    if (g_isArchLbr)
    {
        if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, MAXIMUM_LBR_CAPACITY))
        {
            if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, g_LbrCapacity) ||
                IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, g_LbrCapacity) ||
                IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, g_LbrCapacity))
            {
                CpuWriteMsr(TargetMsr, MsrValue);

                return TRUE;
            }

            //
            // Trying to access LBR stack past supported range.
            //
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
    }

    //
    // Check whether TargetMSR is part of legacy LBR range.
    //
    if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
        IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
        IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, MAXIMUM_LBR_CAPACITY))
    {
        if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, g_LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, g_LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, g_LbrCapacity))
        {
            CpuWriteMsr(TargetMsr, MsrValue);

            return TRUE;
        }

        //
        // Trying to access LBR stack past supported range.
        //
        g_Callbacks.EventInjectGeneralProtection();
        return TRUE;
    }
    
    return FALSE;
}

/**
 * @brief Handle anti-debugging method of a trap flag after a VM exit
 *
 * @return VOID
 */
VOID
TransparentCheckAndTrapFlagAfterVmexit()
{
    //
    // If RIP is incremented, then we emulate an instruction, and then
    // we need to handle the trap flag if it is set in a guest
    //
    g_Callbacks.HvHandleTrapFlag();
}

BOOLEAN
TransparentCheckAndModifyIO(UINT16 Port)
{
    switch(Port) 
    {
        case 0x2B :
        {
            //
            // Guest attempts to trigger an SMI
            // Emulate success by incrementing the spoofed SMI count
            //
            g_TransparentSmiCount++;
            return TRUE;
        }
    }

    return FALSE;
}

/**
 * @brief Save MSR value into relevant return registers
 *
 * @return VOID
 */
VOID
SaveMsrValueToRegisters(PGUEST_REGS Regs, UINT64 MsrValue)
{
    Regs->rax = (UINT32)(MsrValue & 0xffffffff);
    Regs->rdx = (UINT32)(MsrValue >> 32);
}

/**
 * @brief Compile WRMSR operand from registers
 *
 * @return UINT64
 */
UINT64
GetMsrValueFromRegisters(PGUEST_REGS Regs) 
{
    return ((UINT64)Regs->rdx << 32) | Regs->rax;
}
