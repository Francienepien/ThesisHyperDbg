#pragma once

//////////////////////////////////////////////////
//		            Constants                   //
//////////////////////////////////////////////////
#define IN_MSR_RANGE(msr, base, cap) ((msr) >= (base) && (msr) <= ((base) + (cap)))

#define MSR_SMI_COUNT 0x00000034

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////
BOOLEAN
TransparentCheckAndModifyLbrMsrRead(PGUEST_REGS Regs, UINT32 TargetMsr);

BOOLEAN
TransparentCheckAndModifyLbrMsrWrite(PGUEST_REGS Regs, UINT32 TargetMsr);

BOOLEAN
TransparentGenerateLbrEntry(PLBR_STACK_ENTRY Entry, UINT64 Rsp, BOOLEAN TraceKernelSpace, BOOLEAN TraceUserSpace);

MSR_LBR_INFO
TransparentGenerateLbrMetadata();

UINT32
TransparentGetLbrFilterForBranchGeneration();

UINT64
TransparentGetRandomAllowedBranchType();

VOID
SaveMsrValueToRegisters(PGUEST_REGS Regs, UINT64 MsrValue);

UINT64
GetMsrValueFromRegisters(PGUEST_REGS Regs);

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////
/**
 * @brief The structure to hold a the current guests set filters
 *
 */
typedef union _GUEST_LBR_FILTER
{
    ULONG64 AsUInt;

    struct
    {
        ULONG64 LBREn : 1;       // [0]     When set, enables LBR recording
        ULONG64 OS : 1;          // [1]     When set, allows LBR recording when CPL == 0
        ULONG64 USR : 1;         // [2]     When set, allows LBR recording when CPL != 0
        ULONG64 CallStack : 1;   // [3]     When set, records branches in call-stack mode (See Section 7.1.2.4)
        ULONG64 Reserved0 : 12;  // [15:4]  Reserved (must be zero)
        ULONG64 JCC : 1;         // [16]    When set, records taken conditional branches (See Section 7.1.2.3)
        ULONG64 NearRelJmp : 1;  // [17]    When set, records near relative JMPs (See Section 7.1.2.3)
        ULONG64 NearIndJmp : 1;  // [18]    When set, records near indirect JMPs (See Section 7.1.2.3)
        ULONG64 NearRelCall : 1; // [19]    When set, records near relative CALLs (See Section 7.1.2.3)
        ULONG64 NearIndCall : 1; // [20]    When set, records near indirect CALLs (See Section 7.1.2.3)
        ULONG64 NearRet : 1;     // [21]    When set, records near RETs (See Section 7.1.2.3)
        ULONG64 OtherBranch : 1; // [22]    When set, records other branches (See Section 7.1.2.3)
        ULONG64 Reserved1 : 41;  // [63:23] Reserved (must be zero)
    } LBR_CTL;

    struct
    {
        ULONG64 OS : 1;          // [0]    When set, allows LBR recording when CPL == 0
        ULONG64 USR : 1;         // [1]    When set, allows LBR recording when CPL != 0
        ULONG64 JCC : 1;         // [2]    When set, records taken conditional branches (See Section 7.1.2.3)
        ULONG64 NearRelCall : 1; // [3]    When set, records near relative CALLs (See Section 7.1.2.3)
        ULONG64 NearIndCall : 1; // [4]    When set, records near indirect CALLs (See Section 7.1.2.3)
        ULONG64 NearRet : 1;     // [5]    When set, records near RETs (See Section 7.1.2.3)
        ULONG64 NearIndJmp : 1;  // [6]    When set, records near indirect JMPs (See Section 7.1.2.3)
        ULONG64 NearRelJmp : 1;  // [7]    When set, records near relative JMPs (See Section 7.1.2.3)
        ULONG64 FarBranch : 1;   // [8]    When set, records far branches (See Section 7.1.2.3)
        ULONG64 Reserved1 : 45;  // [63:23] Reserved (must be zero)
    } LEGACY_LBR_SELECT;
} GUEST_LBR_FILTER, *PGUEST_LBR_FILTER;
