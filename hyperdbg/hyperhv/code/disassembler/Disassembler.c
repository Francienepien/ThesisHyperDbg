/**
 * @file Diassembler.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Disassembler functions in kernel-mode
 * @details
 *
 * @version 0.3
 * @date 2023-06-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Disassembler show the instructions
 * @details This function should not be called from VMX-root mode
 *
 * @param Address
 * @param Length
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
DisassemblerShowInstructionsInVmxNonRootMode(PVOID Address, UINT32 Length, BOOLEAN Is32Bit)
{
    ZydisDecoder            Decoder;
    ZydisFormatter          Formatter;
    SIZE_T                  ReadOffset = 0;
    ZydisDecodedInstruction Instruction;
    ZydisDecodedOperand     Operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              Status;
    CHAR                    PrintBuffer[128];

    PAGED_CODE();

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return FALSE;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return FALSE;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return FALSE;
        }
    }

    //
    // Initialize Zydis formatter
    //
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&Formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
    {
        return FALSE;
    }

    //
    // Start the decode loop
    //
    while ((Status = ZydisDecoderDecodeFull(&Decoder,
                                            (PVOID)((UINT64)Address + ReadOffset),
                                            Length - ReadOffset,
                                            &Instruction,
                                            Operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(Status));
        if (!ZYAN_SUCCESS(Status))
        {
            ReadOffset++;
            continue;
        }

        // Format and print the instruction
        const ZyanU64 InstrAddress = (ZyanU64)((UINT64)Address + ReadOffset);
        ZydisFormatterFormatInstruction(
            &Formatter,
            &Instruction,
            Operands,
            Instruction.operand_count_visible,
            PrintBuffer,
            sizeof(PrintBuffer),
            InstrAddress,
            NULL);

        LogInfo("+%-4X 0x%-16llX\t\t%hs\n", (UINT32)ReadOffset, InstrAddress, PrintBuffer);

        ReadOffset += Instruction.length;
    }

    //
    // return an error status so that the driver does not have to be unloaded after running
    //
    return TRUE;
}

/**
 * @brief Disassembler show only one instruction
 * @details This function should not be directly called from VMX-root mode
 * if the caller is sure that the target buffer is safe to be access, then it's okay
 *
 * @param Address
 * @param ActualRip
 * @param Is32Bit
 *
 * @return BOOLEAN
 */
BOOLEAN
DisassemblerShowOneInstructionInVmxNonRootMode(PVOID Address, UINT64 ActualRip, BOOLEAN Is32Bit)
{
    ZydisDecoder            Decoder;
    ZydisFormatter          Formatter;
    SIZE_T                  ReadOffset = 0;
    ZydisDecodedInstruction Instruction;
    ZydisDecodedOperand     Operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              Status;
    CHAR                    PrintBuffer[128];

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return FALSE;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return FALSE;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return FALSE;
        }
    }

    //
    // Initialize Zydis formatter
    //
    if (!ZYAN_SUCCESS(ZydisFormatterInit(&Formatter, ZYDIS_FORMATTER_STYLE_INTEL)))
    {
        return FALSE;
    }

    //
    // Start the decode loop
    //
    while ((Status = ZydisDecoderDecodeFull(&Decoder,
                                            (PVOID)((UINT64)Address + ReadOffset),
                                            MAXIMUM_INSTR_SIZE - ReadOffset,
                                            &Instruction,
                                            Operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(Status));
        if (!ZYAN_SUCCESS(Status))
        {
            ReadOffset++;
            continue;
        }

        // Format and print the instruction
        const ZyanU64 InstrAddress = (ZyanU64)((UINT64)ActualRip + ReadOffset);
        ZydisFormatterFormatInstruction(
            &Formatter,
            &Instruction,
            Operands,
            Instruction.operand_count_visible,
            PrintBuffer,
            sizeof(PrintBuffer),
            InstrAddress,
            NULL);

        // LogInfo("+%-4X 0x%-16llX\t\t%hs\n", (UINT32)ReadOffset, InstrAddress, PrintBuffer);

        Log("core: %x | pid: %x - tid: %x,\t %llx \t\t\t\t%hs\n",
            KeGetCurrentProcessorNumberEx(NULL),
            PsGetCurrentProcessId(),
            PsGetCurrentThreadId(),
            ActualRip,
            PrintBuffer);

        // ReadOffset += Instruction.length;

        //
        // Only one instruction is enough
        //
        break;
    }

    //
    // return an error status so that the driver does not have to be unloaded after running
    //
    return TRUE;
}

/**
 * @brief Disassembler length disassemble engine
 * @details if you want to call it directly, shouldn't not be in VMX-root mode, otherwise, you can
 * call DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess to access memory safely
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerLengthDisassembleEngine(PVOID Address, BOOLEAN Is32Bit)
{
    ZydisDecoder            Decoder;
    ZydisDecodedInstruction Instruction;
    ZydisDecodedOperand     Operands[ZYDIS_MAX_OPERAND_COUNT];
    ZyanStatus              Status;

    if (ZydisGetVersion() != ZYDIS_VERSION)
    {
        LogError("Err, invalid zydis version");
        return NULL_ZERO;
    }

    //
    // Initialize Zydis decoder
    //
    if (Is32Bit)
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_COMPAT_32, ZYDIS_STACK_WIDTH_32)))
        {
            return NULL_ZERO;
        }
    }
    else
    {
        if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
        {
            return NULL_ZERO;
        }
    }

    //
    // Start the decode loop
    //
    while ((Status = ZydisDecoderDecodeFull(&Decoder,
                                            (PVOID)((UINT64)Address),
                                            MAXIMUM_INSTR_SIZE,
                                            &Instruction,
                                            Operands)) != ZYDIS_STATUS_NO_MORE_DATA)
    {
        NT_ASSERT(ZYAN_SUCCESS(Status));

        if (!ZYAN_SUCCESS(Status))
        {
            //
            // Probably invalid instruction
            //
            return NULL_ZERO;
        }

        //
        // This is a length disassembler, so we just return the length
        //
        return Instruction.length;
    }

    //
    // return an error status
    //
    return NULL_ZERO;
}

/**
 * @brief Disassembler length disassembler engine
 * @details Should be called in VMX-root mode
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerLengthDisassembleEngineInVmxRootOnTargetProcess(PVOID Address, BOOLEAN Is32Bit)
{
    BYTE   SafeMemoryToRead[MAXIMUM_INSTR_SIZE] = {0};
    UINT64 SizeOfSafeBufferToRead               = 0;

    //
    // Read the maximum number of instruction that is valid to be read in the
    // target address
    //
    SizeOfSafeBufferToRead = CheckAddressMaximumInstructionLength(Address);

    //
    // Find the current instruction
    //
    MemoryMapperReadMemorySafeOnTargetProcess((UINT64)Address,
                                              SafeMemoryToRead,
                                              SizeOfSafeBufferToRead);

    return DisassemblerLengthDisassembleEngine(SafeMemoryToRead, Is32Bit);
}

/**
 * @brief Disassembler length disassembler engine
 * @details Can be called from VMX non-root mode
 *
 * @param Address
 * @param Is32Bit
 * @param ProcessId
 *
 * @return UINT32
 */
UINT32
DisassemblerLengthDisassembleEngineByProcessId(PVOID Address, BOOLEAN Is32Bit, UINT32 ProcessId)
{
    UINT64   SizeOfBufferToRead               = 0;
    BYTE     MemoryToRead[MAXIMUM_INSTR_SIZE] = {0};
    CR3_TYPE CurrentProcessCr3                = {0};

    //
    // Switch to the target process memory layout by using the process id
    //
    CurrentProcessCr3 = SwitchToProcessMemoryLayout(ProcessId);

    //
    // Read the maximum number of instruction that is valid to be read in the
    // target address
    //
    SizeOfBufferToRead = CheckAddressMaximumInstructionLength(Address);

    //
    // Restore the original process
    //
    SwitchToPreviousProcess(CurrentProcessCr3);

    //
    // Find the current instruction
    //
    MemoryMapperReadMemoryUnsafe((UINT64)Address,
                                 MemoryToRead,
                                 SizeOfBufferToRead,
                                 ProcessId);

    return DisassemblerLengthDisassembleEngine(MemoryToRead, Is32Bit);
}

/**
 * @brief Shows the disassembly of only one instruction
 * @details Should be called in VMX-root mode
 *
 * @param Address
 * @param Is32Bit
 *
 * @return UINT32
 */
UINT32
DisassemblerShowOneInstructionInVmxRootMode(PVOID Address, BOOLEAN Is32Bit)
{
    BYTE   SafeMemoryToRead[MAXIMUM_INSTR_SIZE] = {0};
    UINT64 SizeOfSafeBufferToRead               = 0;

    //
    // Read the maximum number of instruction that is valid to be read in the
    // target address
    //
    SizeOfSafeBufferToRead = CheckAddressMaximumInstructionLength(Address);

    //
    // Find the current instruction
    //
    MemoryMapperReadMemorySafeOnTargetProcess((UINT64)Address,
                                              SafeMemoryToRead,
                                              SizeOfSafeBufferToRead);

    return DisassemblerShowOneInstructionInVmxNonRootMode(SafeMemoryToRead, (UINT64)Address, Is32Bit);
}

BOOLEAN
DisassemblerFindGuestBranch(UINT64 CodeBaseVa, UINT8 * GuestCode, PLBR_STACK_ENTRY OutEntries, UINT32 Filter)
{
    UINT8               found     = 0;
    UINT64              Remaining = 4096;
    ZydisDecoder        Decoder;

    if (!ZYAN_SUCCESS(ZydisDecoderInit(&Decoder, ZYDIS_MACHINE_MODE_LONG_64, ZYDIS_STACK_WIDTH_64)))
    {
        return FALSE;
    }

    while (Remaining > 0)
    {
        ZydisDecodedInstruction Instr;
        ZydisDecodedOperand     Operands[ZYDIS_MAX_OPERAND_COUNT];

        if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&Decoder, GuestCode, Remaining, &Instr, Operands)))
            break;

        if (Instr.meta.branch_type != ZYDIS_BRANCH_TYPE_NONE &&
            DissassemblerLbrFilterAllows(Filter, &Instr, Operands))
        {
            UINT64 target = 0;
            if (ZYAN_SUCCESS(ZydisCalcAbsoluteAddress(&Instr, &Operands[0], CodeBaseVa, &target)))
            {
                if (found < 256)
                {
                    if (target != 0)
                    {
                        OutEntries->BranchEntry[found % 32].From      = CodeBaseVa;
                        OutEntries->BranchEntry[found % 32].To        = target;
                        OutEntries->LastBranchInfo[found % 32].AsUInt     = 0xD00BEED00;
                        found++;
                    }
                }
            }
        }

        CodeBaseVa += Instr.length;
        GuestCode += Instr.length;
        Remaining -= Instr.length;
    }

    if (found > 0)
    {
        OutEntries->Tos = (found - 1) % 32;
        return TRUE;
    }
    
    return FALSE;
}

BOOLEAN
DissassemblerLbrFilterAllows(UINT32 Filter, ZydisDecodedInstruction * Instr, ZydisDecodedOperand * Operands)
{
    if (Filter == 0xFFFFFFFF)
        return TRUE;

    ZydisMnemonic m = Instr->mnemonic;

    if (m == ZYDIS_MNEMONIC_CALL)
    {
        if (Operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            return Filter & LBR_HYPEREVADE_FILTER_NEAR_REL_CALL;
        else
            return Filter & LBR_HYPEREVADE_FILTER_NEAR_IND_CALL;
    }

    if (m == ZYDIS_MNEMONIC_RET)
        return Filter & LBR_HYPEREVADE_FILTER_NEAR_RET;

    if (m == ZYDIS_MNEMONIC_JMP)
    {
        if (Instr->meta.branch_type == ZYDIS_BRANCH_TYPE_FAR)
            return Filter & LBR_HYPEREVADE_FILTER_FAR_BRANCH;
        if (Operands[0].type == ZYDIS_OPERAND_TYPE_IMMEDIATE)
            return Filter & LBR_HYPEREVADE_FILTER_NEAR_REL_JMP;
        else
            return Filter & LBR_HYPEREVADE_FILTER_NEAR_IND_JMP;
    }

    if (Instr->meta.branch_type != ZYDIS_BRANCH_TYPE_NONE)
        return Filter & LBR_HYPEREVADE_FILTER_JCC;

    return TRUE;
}
