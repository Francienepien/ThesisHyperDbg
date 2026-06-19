/**
 * @file VmxFootprints.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author jtaw5649
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
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_CPUID) == 0)
    {
        return;
    }

    if (Regs->rax == CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS)
    {
        //
        // Unset the Hypervisor Present-bit in RCX, which Intel and AMD have both
        // reserved for this indication
        //
        CpuInfo[2] &= ~(HYPERV_HYPERVISOR_PRESENT_BIT|CPUID_VMX_SUPPORT_BIT|CPUID_SMX_SUPPORT_BIT);
    }
    else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS || Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // When transparent, all CPUID leaves in the 0x40000000+ range should contain no usable data
        //
        CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0x40000000;
    }
    else if (Regs->rax == CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS && Regs->rcx == 0 && g_IsArchLbr)
    {
        //
        // Report Arch LBR support in CPUID.07H.00H ECX[bit 19] when the Transparent mode is enabled
        //
        CpuInfo[3] |= CPUID_ARCH_LBR_PRESENT_BIT;
    }
    else if (Regs->rax == 0x1c && g_IsArchLbr)
    {
        //
        // Report Arch LBR depth in CPUID.1CH.00H EAX[7:0] when the Transparent mode is enabled
        //
        CpuInfo[0] = g_LbrCapacity & 0xFF;
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
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_MSR) == 0)
    {
        return FALSE;
    }

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

    UINT64 MsrValue = 0;
    
    if (TargetMsr != 0x400000b1 && TargetMsr != 0x400000b0)
    {
        LogInfo("TransparentCheckAndModifyMsrRead: MSR: %x", TargetMsr);
    }

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
    case IA32_FEATURE_CONTROL:
    {
        //
        // Spoof VMX support being disabled in IA32_FEATURE_CONTROL value to hide hypervisor
        //
        MsrValue = CpuReadMsr(IA32_FEATURE_CONTROL);
        MsrValue |= IA32_FEATURE_CONTROL_LOCK_BIT_FLAG;
        MsrValue &= ~(IA32_FEATURE_CONTROL_ENABLE_VMX_INSIDE_SMX_FLAG  | IA32_FEATURE_CONTROL_ENABLE_VMX_OUTSIDE_SMX_FLAG);

        SaveMsrValueToRegisters(Regs, MsrValue);

        return TRUE;
    }
    }

    if (TransparentCheckAndModifyLbrMsrRead(Regs, TargetMsr))
    {
        return TRUE;
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
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_MSR) == 0)
    {
        return FALSE;
    }

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

    switch (TargetMsr)
    {
    case IA32_FEATURE_CONTROL:
    {
        //
        // Lock bit is set in transparent mdoe, so we #GP
        //
        g_Callbacks.EventInjectGeneralProtection();

        return TRUE;
    }
    }

    if (TransparentCheckAndModifyLbrMsrWrite(Regs, TargetMsr))
    {
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
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_TRAP_FLAG) == 0)
    {
        return;
    }

    //
    // If RIP is incremented, then we emulate an instruction, and then
    // we need to handle the trap flag if it is set in a guest
    //
    g_Callbacks.HvHandleTrapFlag();
}

/**
 * @brief Handle I/O write access VM exits when the Transparent mode is enabled
 *
 * @param Port that is being written to
 *
 * @return BOOLEAN Whether the emulation should be further continued or not
 */
BOOLEAN
TransparentCheckAndModifyIOWrite(UINT16 Port)
{
    switch(Port) 
    {
        case 0xB2 :
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
