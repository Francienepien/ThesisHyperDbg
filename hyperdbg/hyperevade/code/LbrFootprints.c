#include "pch.h"

BOOLEAN
TransparentCheckAndModifyLbrMsrRead(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    const UINT32    LbrCapacity = g_LbrCapacity;
    LBR_STACK_ENTRY entry       = {0};
    UINT64          MsrValue;

    switch (TargetMsr)
    {
    case IA32_DEBUGCTL:
    {
        MsrValue = CpuReadMsr(IA32_DEBUGCTL);
        SaveMsrValueToRegisters(Regs, MsrValue);
        return TRUE;
    }
    case MSR_LBR_TOS:
    {
        if (!g_isArchLbr)
        {
            //
            // Return any valid value.
            //
            MsrValue = TransparentGetRand() % LbrCapacity;
        }
        else
        {
            //
            // Arch LBR does not populate this MSR
            //
            MsrValue = 0;
        }

        SaveMsrValueToRegisters(Regs, MsrValue);
        return TRUE;
    }
    case MSR_LEGACY_LBR_SELECT:
    {
        if (!g_isArchLbr)
        {
            SaveMsrValueToRegisters(Regs, g_GuestLbrFilter.AsUInt);
        }
        return TRUE;
    }
    case IA32_LBR_CTL:
    {
        if (g_isArchLbr)
        {
            SaveMsrValueToRegisters(Regs, g_GuestLbrFilter.AsUInt);
        }

        return TRUE;
    }
    }

    if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        else if (!g_isArchLbr)
        {
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
        if (!TransparentGenerateLbrEntry(&entry, 
                                         Regs->rsp, 
                                         (BOOLEAN)g_GuestLbrFilter.LBR_CTL.OS, 
                                         (BOOLEAN)g_GuestLbrFilter.LBR_CTL.USR))
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].From);
        return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        else if (!g_isArchLbr)
        {
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
        if (!TransparentGenerateLbrEntry(&entry,
                                         Regs->rsp,
                                         (BOOLEAN)g_GuestLbrFilter.LBR_CTL.OS,
                                         (BOOLEAN)g_GuestLbrFilter.LBR_CTL.USR))
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].To);
        return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        else if (!g_isArchLbr)
        {
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, TransparentGenerateLbrMetadata().AsUInt);
        return TRUE;
    }

    if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled || g_isArchLbr)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        if (!TransparentGenerateLbrEntry(&entry,
                                         Regs->rsp,
                                         !g_GuestLbrFilter.LEGACY_LBR_SELECT.OS,
                                         !g_GuestLbrFilter.LEGACY_LBR_SELECT.USR))
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].From);
        return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled || g_isArchLbr)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        if (!TransparentGenerateLbrEntry(&entry,
                                         Regs->rsp,
                                         !g_GuestLbrFilter.LEGACY_LBR_SELECT.OS,
                                         !g_GuestLbrFilter.LEGACY_LBR_SELECT.USR))
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, entry.BranchEntry[entry.Tos].To);
        return TRUE;
    }
    else if (IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, LbrCapacity))
    {
        if (!g_IsGuestLbrEnabled || g_isArchLbr)
        {
            SaveMsrValueToRegisters(Regs, 0);
            return TRUE;
        }
        SaveMsrValueToRegisters(Regs, TransparentGenerateLbrMetadata().AsUInt);
        return TRUE;
    }

    if (LbrCapacity < MAXIMUM_LBR_CAPACITY &&
        (IN_MSR_RANGE(TargetMsr, IA32_LBR_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
         IN_MSR_RANGE(TargetMsr, IA32_LBR_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
         IN_MSR_RANGE(TargetMsr, IA32_LBR_0_INFO, MAXIMUM_LBR_CAPACITY) ||
         IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_FROM_IP, MAXIMUM_LBR_CAPACITY) ||
         IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_0_TO_IP, MAXIMUM_LBR_CAPACITY) ||
         IN_MSR_RANGE(TargetMsr, MSR_LASTBRANCH_INFO_0, MAXIMUM_LBR_CAPACITY)))
    {
        //
        // Trying to access LBR stack past supported range.
        //
        g_Callbacks.EventInjectGeneralProtection();
        return TRUE;
    }

    return FALSE;
}

BOOLEAN
TransparentCheckAndModifyLbrMsrWrite(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    UINT64       MsrValue;
    const UINT32 LbrCapacity = g_LbrCapacity;

    if (TargetMsr != 0x400000b1 && TargetMsr != 0x400000b0)
    {
        LogInfo("TransparentCheckAndModifyMsrWrite: MSR: %x", TargetMsr);
    }

    MsrValue = GetMsrValueFromRegisters(Regs);

    switch (TargetMsr)
    {
    case IA32_DEBUGCTL:
    {
        //
        // Guest tries to enable Arch LBR
        // We dont check for disabling, as guest likely expects branches to be populated after lbr is disabled.
        //
        if (MsrValue & 1 && !g_isArchLbr)
        {
            g_IsGuestLbrEnabled = TRUE;
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

        if (!g_isArchLbr)
        {
            g_GuestLbrFilter.AsUInt = MsrValue;
        }

        return TRUE;
    }
    case MSR_LBR_TOS:
    {
        return TRUE;
    }
    case IA32_LBR_CTL:
    {
        //
        // Guest tries to enable Arch LBR
        // We dont check for disabling, as guest likely expects branches to be populated after lbr is disabled.
        //
        if (!g_isArchLbr)
        {
            //
            // IA32_LBR_CTL should not exist on legacy-lbr CPUs
            //
            g_Callbacks.EventInjectGeneralProtection();
            return TRUE;
        }
        if (MsrValue & 1)
        {
            g_IsGuestLbrEnabled = TRUE;
        }

        g_GuestLbrFilter.AsUInt = MsrValue;

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
 * @brief Save MSR value into relevant return registers
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentGenerateLbrEntry(PLBR_STACK_ENTRY Entry, UINT64 Rsp, BOOLEAN TraceKernelSpace, BOOLEAN TraceUserSpace)
{
    PDEBUGGER_SINGLE_CALLSTACK_FRAME frames;
    UINT32                           OutCount;
    UINT8 *                          buffer;

    if (!TraceKernelSpace && !TraceUserSpace)
    {
        return FALSE;
    }

    frames = (PDEBUGGER_SINGLE_CALLSTACK_FRAME)PlatformAllocateMemory(1024 * sizeof(DEBUGGER_SINGLE_CALLSTACK_FRAME));

    if (!frames)
    {
        return TRUE;
    }

    if (!g_Callbacks.DebuggingCallbackCallstackWalkthroughStack(frames, &OutCount, Rsp, PAGE_SIZE * 2, FALSE))
    {
        LogInfo("Something is going wrong in the stack walk.\n");
        return FALSE;
    }

    for (UINT32 i = 0; i < OutCount; i++)
    {
        if (frames[i].IsValidAddress && frames[i].IsExecutable &&
            ((TraceKernelSpace && frames[i].Value >= 0xFFFF080000000000) ||
             (TraceUserSpace && frames[i].Value <= 0x00007FFFFFFFFFFF)))
        {
            buffer = PlatformAllocateMemory(4096);

            if (!buffer)
            {
                PlatformFreeMemory(frames);
                return FALSE;
            }

            g_Callbacks.MemoryMapperReadMemorySafeOnTargetProcess(frames[i].Value, buffer, 4096);

            if (!g_Callbacks.CallbackDisassemblerFindGuestBranch(frames[i].Value, buffer, Entry, TransparentGetLbrFilterForBranchGeneration()))
            {
                LogInfo("ATTEMPTING NEXT FRAME\n");
                continue;
            }

            PlatformFreeMemory(frames);
            PlatformFreeMemory(buffer);
            return TRUE;
        }
    }

    return FALSE;
}

MSR_LBR_INFO
TransparentGenerateLbrMetadata()
{
    MSR_LBR_INFO Out = {0};

    Out.CycleCount = (UINT64)(5 + (TransparentGetRand() % 75));
    Out.Mispred    = (UINT64)(TransparentGetRand() % 2);
    
    if (g_isArchLbr)
    {
        Out.CycCntValid_OnlyArchLbr = 1;
        Out.BrType_OnlyArchLbr      = TransparentGetRandomAllowedBranchType();
    }

    return Out;
}

UINT32
TransparentGetLbrFilterForBranchGeneration()
{
    UINT32 Out = 0;
    GUEST_LBR_FILTER RawFilter = g_GuestLbrFilter;

    if (g_isArchLbr)
    {
        if (RawFilter.LBR_CTL.NearRelCall)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_REL_CALL;
        if (RawFilter.LBR_CTL.NearRet)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_RET;
        if (RawFilter.LBR_CTL.JCC)
            Out |= LBR_HYPEREVADE_FILTER_JCC;
        if (RawFilter.LBR_CTL.NearIndCall)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_IND_CALL;
        if (RawFilter.LBR_CTL.NearRelJmp)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_REL_JMP;
        if (RawFilter.LBR_CTL.NearIndJmp)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_IND_JMP;
        if (RawFilter.LBR_CTL.OtherBranch)
            Out |= LBR_HYPEREVADE_FILTER_FAR_BRANCH;
    }
    else
    {
        if (!RawFilter.LEGACY_LBR_SELECT.NearRelCall)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_REL_CALL;
        if (!RawFilter.LEGACY_LBR_SELECT.NearRet)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_RET;
        if (!RawFilter.LEGACY_LBR_SELECT.JCC)
            Out |= LBR_HYPEREVADE_FILTER_JCC;
        if (!RawFilter.LEGACY_LBR_SELECT.NearIndCall)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_IND_CALL;
        if (!RawFilter.LEGACY_LBR_SELECT.NearRelJmp)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_REL_JMP;
        if (!RawFilter.LEGACY_LBR_SELECT.NearIndJmp)
            Out |= LBR_HYPEREVADE_FILTER_NEAR_IND_JMP;
        if (!RawFilter.LEGACY_LBR_SELECT.FarBranch)
            Out |= LBR_HYPEREVADE_FILTER_FAR_BRANCH;
    }

    return Out;
}

UINT64
TransparentGetRandomAllowedBranchType()
{
    //
    // Collect all allowed branch types into a small array,
    // then pick one at random.
    //
    UINT32 Filter                               = TransparentGetLbrFilterForBranchGeneration();
    UINT32 Allowed[8]                           = {0};
    UINT32 Count                                = 0;

    if (!(Filter & LBR_HYPEREVADE_FILTER_NEAR_REL_CALL))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_NEAR_REL_CALL;
    if (!(Filter & LBR_HYPEREVADE_FILTER_NEAR_RET))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_NEAR_RET;
    if (!(Filter & LBR_HYPEREVADE_FILTER_JCC))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_JCC;
    if (!(Filter & LBR_HYPEREVADE_FILTER_NEAR_IND_CALL))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_NEAR_IND_CALL;
    if (!(Filter & LBR_HYPEREVADE_FILTER_NEAR_REL_JMP))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_NEAR_REL_JMP;
    if (!(Filter & LBR_HYPEREVADE_FILTER_NEAR_IND_JMP))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_NEAR_IND_JMP;
    if (!(Filter & LBR_HYPEREVADE_FILTER_FAR_BRANCH))
        Allowed[Count++] = LBR_HYPEREVADE_FILTER_FAR_BRANCH;

    if (Count == 0)
    {
        // All branches filtered out — fall back to a safe default
        return LBR_HYPEREVADE_FILTER_NEAR_REL_CALL;
    }

    return Allowed[TransparentGetRand() % Count];
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
