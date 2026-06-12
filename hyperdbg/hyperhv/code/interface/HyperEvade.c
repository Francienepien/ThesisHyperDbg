/**
 * @file HyperEvade.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author jtaw5649
 * @brief Hyperevade function wrappers
 * @details
 *
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Wrapper for hiding debugger on transparent-mode (activate transparent-mode)
 *
 * @param HyperevadeCallbacks
 * @param TransparentModeRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentHideDebuggerWrapper(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest)
{
    HYPEREVADE_CALLBACKS HyperevadeCallbacks = {0};
    UINT32               EvadeMask           = TransparentModeRequest->EvadeMask;

    if (EvadeMask == 0)
    {
        EvadeMask = TRANSPARENT_EVADE_MASK_DEFAULT;
    }

    if ((EvadeMask & ~TRANSPARENT_EVADE_MASK_ALL) != 0)
    {
        TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
        return FALSE;
    }

    TransparentModeRequest->EvadeMask = EvadeMask;

    //
    // *** Fill the callbacks ***
    //

    //
    // Fill the callbacks for using hyperlog in hyperevade
    // We use the callbacks directly to avoid two calls to the same function
    //
    HyperevadeCallbacks.LogCallbackPrepareAndSendMessageToQueueWrapper = g_Callbacks.LogCallbackPrepareAndSendMessageToQueueWrapper;
    HyperevadeCallbacks.LogCallbackSendMessageToQueue                  = g_Callbacks.LogCallbackSendMessageToQueue;
    HyperevadeCallbacks.LogCallbackSendBuffer                          = g_Callbacks.LogCallbackSendBuffer;
    HyperevadeCallbacks.LogCallbackCheckIfBufferIsFull                 = g_Callbacks.LogCallbackCheckIfBufferIsFull;

    //
    // HyperTrace callback(s)
    //
    HyperevadeCallbacks.HyperTraceLbrIsSupported = HyperTraceCallbackLbrIsSupported;

    //
    // Memory callbacks
    //
    HyperevadeCallbacks.CheckAccessValidityAndSafety               = CheckAccessValidityAndSafety;
    HyperevadeCallbacks.MemoryMapperReadMemorySafeOnTargetProcess  = MemoryMapperReadMemorySafeOnTargetProcess;
    HyperevadeCallbacks.MemoryMapperWriteMemorySafeOnTargetProcess = MemoryMapperWriteMemorySafeOnTargetProcess;

    //
    // Common callbacks
    //
    HyperevadeCallbacks.CommonGetProcessNameFromProcessControlBlock = CommonGetProcessNameFromProcessControlBlock;

    //
    // System call callbacks
    //
    HyperevadeCallbacks.SyscallCallbackSetTrapFlagAfterSyscall = SyscallCallbackSetTrapFlagAfterSyscall;

    //
    // VMX callbacks
    //
    HyperevadeCallbacks.HvHandleTrapFlag             = HvHandleTrapFlag;
    HyperevadeCallbacks.EventInjectGeneralProtection = EventInjectGeneralProtection;
    HyperevadeCallbacks.CallbackGenerateLbrEntry = GenerateLbrEntry;

    //
    // Debugging callbacks
    //
    HyperevadeCallbacks.DebuggingCallbackCallstackWalkthroughStack = g_Callbacks.DebuggingCallbackCallstackWalkthroughStack;

    //
    // Call the hyperevade hide debugger function
    //
    if (TransparentHideDebugger(&HyperevadeCallbacks, TransparentModeRequest))
    {
        //
        // Initialize the syscall callback mechanism from hypervisor after
        // transparent-mode accepts the request as the active state.
        //
        if ((EvadeMask & TRANSPARENT_EVADE_MASK_SYSCALL_HOOK) != 0 && !SyscallCallbackInitialize())
        {
            TransparentUnhideDebugger();

            TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
            g_CheckForFootprints                 = FALSE;
            return FALSE;
        }

        //
        // Status is set within the transparent mode (hyperevade) module
        //
        g_CheckForFootprints = TRUE;

        //
        // Set MSR bitmaps used in transparent mode.
        //
        TransparentSetMSRBitmap();

        //
        // Set I/O bitmap used in transparent mode
        //
        TransparentSetIOBitmap();

        return TRUE;
    }
    else
    {
        //
        // Status is set within the transparent mode (hyperevade) module
        //
        g_CheckForFootprints = FALSE;
        return FALSE;
    }
}

/**
 * @brief Deactivate transparent-mode
 * @param TransparentModeRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
TransparentUnhideDebuggerWrapper(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest)
{
    if (SyscallCallbackIsInitialized() && !SyscallCallbackUninitialize())
    {
        if (TransparentModeRequest != NULL)
        {
            TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER;
        }

        return FALSE;
    }

    if (TransparentUnhideDebugger())
    {
        //
        // Unset MSR bitmaps used in transparent mode.
        //
        TransparentUnSetMSRBitmap();

        //
        // Unset I/O bitmap used in transparent mode
        //
        TransparentUnSetIOBitmap();

        //
        // Unset transparent mode for the VMM module
        //
        g_CheckForFootprints = FALSE;

        if (TransparentModeRequest != NULL)
        {
            TransparentModeRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
        }

        return TRUE;
    }
    else
    {
        if (TransparentModeRequest != NULL)
        {
            TransparentModeRequest->KernelStatus = DEBUGGER_ERROR_DEBUGGER_ALREADY_UNHIDE;
        }
        return FALSE;
    }
}

/**
 * @brief Update the MSR bitmap when enabling transparent mode
 *
 * @return VOID
 */
VOID
TransparentSetMSRBitmap()
{
    ULONG ProcessorCount;
    UINT32 capacity;
    BOOLEAN IsArchLbr;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    g_Callbacks.HyperTraceCallbackLbrIsSupported(&capacity, &IsArchLbr);

    //
    // Enable bitmaps to intercept MSR reads/writes related to LBR and SMI count as they can be used for VM detection
    // Only intercept writes to save on performance.
    //
    for (size_t ProcessorID = 0; ProcessorID < ProcessorCount; ProcessorID++)
    {
        MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], MSR_SMI_COUNT, TRUE, FALSE);
        MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], MSR_LEGACY_LBR_SELECT, TRUE, TRUE);
        MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], MSR_LBR_TOS, TRUE, TRUE);
        MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], IA32_LBR_CTL, TRUE, TRUE);
        MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], IA32_DEBUGCTL, TRUE, TRUE);

        for (size_t BranchIndex = 0; BranchIndex < MAXIMUM_LBR_CAPACITY; BranchIndex++)
        {
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_0_FROM_IP + BranchIndex), TRUE, TRUE);
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_0_TO_IP + BranchIndex), TRUE, TRUE);
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_INFO_0 + BranchIndex), TRUE, TRUE);
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_FROM_IP + BranchIndex), TRUE, TRUE);
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_TO_IP + BranchIndex), TRUE, TRUE);
            MsrHandleSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_INFO + BranchIndex), TRUE, TRUE);
        }
    }
}

/**
 * @brief Update the MSR bitmap when disabling transparent mode
 *
 * @return VOID
 */
VOID
TransparentUnSetMSRBitmap()
{
    ULONG   ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);
    
    //
    // Enable bitmaps to intercept MSR reads/writes related to LBR and SMI count as they can be used for VM detection
    // LBR MSRs support r/w access, so we should intercept both. SMI count is read-only.
    //
    for (size_t ProcessorID = 0; ProcessorID < ProcessorCount; ProcessorID++)
    {
        MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], MSR_SMI_COUNT, TRUE, FALSE);
        MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], MSR_LEGACY_LBR_SELECT, TRUE, TRUE);
        MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], MSR_LBR_TOS, TRUE, TRUE);
        MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], IA32_LBR_CTL, TRUE, TRUE);
        MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], IA32_DEBUGCTL, TRUE, TRUE);

        for (size_t BranchIndex = 0; BranchIndex < MAXIMUM_LBR_CAPACITY; BranchIndex++)
        {
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_0_FROM_IP + BranchIndex), TRUE, TRUE);
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_0_TO_IP + BranchIndex), TRUE, TRUE);
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(MSR_LASTBRANCH_INFO_0 + BranchIndex), TRUE, TRUE);
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_FROM_IP + BranchIndex), TRUE, TRUE);
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_TO_IP + BranchIndex), TRUE, TRUE);
            MsrHandleUnSetMsrBitmap(&g_GuestState[ProcessorID], (UINT32)(IA32_LBR_0_INFO + BranchIndex), TRUE, TRUE);
        }
    }
}

/**
 * @brief Update the I/O bitmap when enabling transparent mode
 *
 * @return VOID
 */
VOID
TransparentSetIOBitmap()
{
    ULONG   ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    for (size_t ProcessorID = 0; ProcessorID < ProcessorCount; ProcessorID++)
    {
        IoHandleSetIoBitmap(&g_GuestState[ProcessorID], 0xb2);
    }
}

/**
 * @brief Update the I/O bitmap when Disabling transparent mode
 *
 * @return VOID
 */
VOID
TransparentUnSetIOBitmap()
{
    ULONG ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCount(0);

    for (size_t ProcessorID = 0; ProcessorID < ProcessorCount; ProcessorID++)
    {
        IoHandleUnSetIoBitmap(&g_GuestState[ProcessorID], 0xb2);
    }
}
