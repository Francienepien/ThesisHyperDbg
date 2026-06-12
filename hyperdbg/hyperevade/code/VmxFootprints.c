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
        CpuInfo[2] &= ~HYPERV_HYPERVISOR_PRESENT_BIT;
    }
    else if (Regs->rax == CPUID_HV_VENDOR_AND_MAX_FUNCTIONS || Regs->rax == HYPERV_CPUID_INTERFACE)
    {
        //
        // When transparent, all CPUID leaves in the 0x40000000+ range should contain no usable data
        //
        CpuInfo[0] = CpuInfo[1] = CpuInfo[2] = CpuInfo[3] = 0x40000000;
    }
    else if (Regs->rax == CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS && Regs->rcx == 0)
    {
        // LogInfo("TransparentCheckAndModifyCpuid: CPUID.07H.00H ArchLBR bit: %d", (CpuInfo[3] >> 19) & 1);
    }
    else if (Regs->rax == 0x1c)
    {
        // LogInfo("TransparentCheckAndModifyCpuid: CPUID.1CH.00H ArchLBR depth mask: 0x%02X", CpuInfo[0] & 0xFF);
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
TransparentCheckAndModifyMsrRead(PGUEST_REGS Regs, UINT64 Rip, UINT32 TargetMsr)
{
    if ((g_TransparentEvadeMask & TRANSPARENT_EVADE_MASK_MSR) == 0)
    {
        UNREFERENCED_PARAMETER(Regs);
        UNREFERENCED_PARAMETER(TargetMsr);

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

    UINT64 MsrValue;
    const UINT32 LbrCapacity = g_LbrCapacity;

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
    case IA32_DEBUGCTL:
    case MSR_LBR_TOS:
    {
        return TRUE;
    }
    case MSR_LEGACY_LBR_SELECT:
    case IA32_LBR_CTL:
    {
        SaveMsrValueToRegisters(Regs, g_GuestLbrFilter);

        return TRUE;
    }
    }

    if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, LbrCapacity))
    {
        PDEBUGGER_SINGLE_CALLSTACK_FRAME frames = (PDEBUGGER_SINGLE_CALLSTACK_FRAME)ExAllocatePoolWithTag(
            NonPagedPool,
            1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME),
            'frms');
        if (!frames)
        {
            return TRUE;
        }
        RtlZeroMemory(frames, 1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME));
            UINT32                          OutCount     = 0;
            if (!g_Callbacks.DebuggingCallbackCallstackWalkthroughStack(frames, &OutCount, Regs->rsp, PAGE_SIZE * 2, FALSE))
            {
                LogInfo("Something is going wrong in the stack walk.\n");
                return TRUE;
            }

            for (UINT32 i = 0; i < OutCount; i++)
            {
                if (frames[i].IsValidAddress && frames[i].IsExecutable)
                {
                    LBR_STACK_ENTRY entry  = {0};
                    VOID *           buffer = ExAllocatePoolWithTag(NonPagedPool, 4096, 'lbrs');
                    if (!buffer)
                    {
                        ExFreePoolWithTag(frames, 'frms');
                        return TRUE;
                    }
                    g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(frames[i].Value, buffer, 4096);
                    if (!g_Callbacks.CallbackGenerateLbrEntry(frames[i].Value, buffer, Rip, &entry))
                    {
                        LogInfo("ATTEMPTING NEXT FRAME\n");
                        continue;
                    }
                    LogInfo("Saving LBR From: %llx\n", entry.BranchEntry[entry.Tos].From);
                    SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].From);
                    ExFreePoolWithTag(frames, 'frms');
                    ExFreePoolWithTag(buffer, 'lbrs');
                    return TRUE;
                }
            }

            // SaveMsrValueToRegisters(Regs, MsrValue);

            return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, LbrCapacity))
    {
        PDEBUGGER_SINGLE_CALLSTACK_FRAME frames = (PDEBUGGER_SINGLE_CALLSTACK_FRAME)ExAllocatePoolWithTag(
            NonPagedPool,
            1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME),
            'frms');
        if (!frames)
        {
            return TRUE;
        }
        RtlZeroMemory(frames, 1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME));
            UINT32                          OutCount     = 0;
            if (!g_Callbacks.DebuggingCallbackCallstackWalkthroughStack(frames, &OutCount, Regs->rsp, PAGE_SIZE * 2, FALSE))
            {
                LogInfo("Something is going wrong in the stack walk.\n");
                return TRUE;
            }

            for (UINT32 i = 0; i < OutCount; i++)
            {
                if (frames[i].IsValidAddress && frames[i].IsExecutable)
                {
                    LBR_STACK_ENTRY entry  = {0};
                    VOID *           buffer = ExAllocatePoolWithTag(NonPagedPool, 4096, 'lbrs');
                    if (!buffer)
                    {
                        ExFreePoolWithTag(frames, 'frms');
                        return TRUE;
                    }
                    g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(frames[i].Value, buffer, 4096);
                    if (!g_Callbacks.CallbackGenerateLbrEntry(frames[i].Value, buffer, Rip, &entry))
                    {
                        LogInfo("ATTEMPTING NEXT FRAME\n");
                        continue;
                    }
                    LogInfo("Saving LBR To: %llx\n", entry.BranchEntry[entry.Tos].To);
                    SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].To);
                    ExFreePoolWithTag(frames, 'frms');
                    ExFreePoolWithTag(buffer, 'lbrs');
                    return TRUE;
                }
            }

            // SaveMsrValueToRegisters(Regs, MsrValue);

            return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, LbrCapacity))
    {
        PDEBUGGER_SINGLE_CALLSTACK_FRAME frames = (PDEBUGGER_SINGLE_CALLSTACK_FRAME)ExAllocatePoolWithTag(
            NonPagedPool,
            1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME),
            'frms');
        if (!frames)
        {
            return TRUE;
        }
        RtlZeroMemory(frames, 1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME));
        UINT32                          OutCount     = 0;
        if (!g_Callbacks.DebuggingCallbackCallstackWalkthroughStack(frames, &OutCount, Regs->rsp, PAGE_SIZE * 2, FALSE))
        {
            LogInfo("Something is going wrong in the stack walk.\n");
            return TRUE;
        }


        for (UINT32 i = 0; i < OutCount; i++)
        {
            if (frames[i].IsValidAddress && frames[i].IsExecutable)
            {
                LBR_STACK_ENTRY entry  = {0};
                VOID *           buffer = ExAllocatePoolWithTag(NonPagedPool, 4096, 'lbrs');
                if (!buffer)
                {
                    ExFreePoolWithTag(frames, 'frms');
                    return TRUE;
                }
                g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(frames[i].Value, buffer, 4096);
                if (!g_Callbacks.CallbackGenerateLbrEntry(frames[i].Value, buffer, Rip, &entry))
                {
                    LogInfo("ATTEMPTING NEXT FRAME\n");
                    continue;
                }
                LogInfo("Saving LBR info: %llx\n", entry.LastBranchInfo[entry.Tos].AsUInt);
                SaveMsrValueToRegisters(Regs, entry.LastBranchInfo[entry.Tos].AsUInt);
                ExFreePoolWithTag(frames, 'frms');
                ExFreePoolWithTag(buffer, 'lbrs');
                return TRUE;
            }
        }

        return TRUE;
    }
      if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
          IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
          IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, MAXIMUM_LBR_CAPACITY))
      {
          //
          // Trying to access LBR stack past supported range.
          //
          g_Callbacks.EventInjectGeneralProtection();
          return TRUE;
      }
      if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, LbrCapacity) ||
          IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, LbrCapacity) ||
          IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, LbrCapacity))
      {
          MsrValue = CpuReadMsr(TargetMsr);

          SaveMsrValueToRegisters(Regs, MsrValue);

          return TRUE;
      }
      if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
          IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
          IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, MAXIMUM_LBR_CAPACITY))
      {
          //
          // Trying to access LBR stack past supported range.
          //
          g_Callbacks.EventInjectGeneralProtection();
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
        UNREFERENCED_PARAMETER(Regs);
        UNREFERENCED_PARAMETER(TargetMsr);

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

    UINT64            MsrValue;
    const UINT32      LbrCapacity = g_LbrCapacity;

    if (TargetMsr != 0x400000b1 && TargetMsr != 0x400000b0)
    {
        LogInfo("TransparentCheckAndModifyMsrWrite: MSR: %x", TargetMsr);
    }
    
    MsrValue = GetMsrValueFromRegisters(Regs);

    switch (TargetMsr)
    {
    case IA32_DEBUGCTL:
    {
        if (MsrValue & 1)
        {
            g_IsGuestLbrEnabled = TRUE;
        }
        else
        {
            g_IsGuestLbrEnabled = FALSE;
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

        g_GuestLbrFilter = MsrValue;

        return TRUE;
    }
    case MSR_LBR_TOS:
    {
        return TRUE;
    }
    case IA32_LBR_CTL:
    {
        if (MsrValue & 1)
        {
            g_IsGuestLbrEnabled = TRUE;
        }
        else
        {
            g_IsGuestLbrEnabled = FALSE;
        }

        //
        // TODO: add filter support
        //
        g_GuestLbrFilter = MsrValue;

        return TRUE;
    }
    }


        if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, LbrCapacity))
        {
            //
            // TODO: LBR MSRs can be written to in order to store an arbitrary value
            //

            return TRUE;
        }

        if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, MAXIMUM_LBR_CAPACITY))
        {
            //
            // Trying to access LBR stack past supported range.
            //
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
        //
        // Check whether TargetMSR is part of legacy LBR range.
        //
        if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, LbrCapacity) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, LbrCapacity))
        {
            //
            // TODO: LBR MSRs can be written to in order to store an arbitrary value
            //

            return TRUE;
        }

        if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
            IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, MAXIMUM_LBR_CAPACITY))
        {
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
