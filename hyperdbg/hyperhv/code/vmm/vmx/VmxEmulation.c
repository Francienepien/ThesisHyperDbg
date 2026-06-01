#include "pch.h"

/**
 * @brief Handling VM exit related to VMCLEAR instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmclear(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 FetchedAddress;

    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    //
    // Fetch VMCS pointer and check its validity
    //
    if (!VmexitFetchAndCheckVmcsPointerAlignment(VCpu, &FetchedAddress))
    {
        return;
    }

    LogInfo("Guest executed VMCLEAR\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMPTRLD instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmptrld(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 FetchedAddress;

    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    if (!VmexitFetchAndCheckVmcsPointerAlignment(VCpu, &FetchedAddress))
    {
        return;
    }

    LogInfo("Guest executed VMPTRLD\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMPTRST instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmptrst(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    LogInfo("Guest executed VMPTRST\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMREAD instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmread(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 FetchedField;
    UINT64 FieldValue;
    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    VmexitFetchVMCSField(VCpu, &FetchedField);

    VmxVmread64P(FetchedField, &FieldValue);

    //
    // TODO: Pass read value to guest.
    //
}

/**
 * @brief Handling VM exit related to VMRESUME instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmresume(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    LogInfo("Guest executed VMRESUME\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMWRITE instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmwrite(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64 FetchedField;
    UINT64 FieldValue;

    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    VmexitFetchVMCSField(VCpu, &FetchedField);

    FieldValue = 0;

    //
    // TODO: Firgure out written value and write it to the relevant field in the VMCS
    //
}

/**
 * @brief Handling VM exit related to VMXOFF instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmxoff(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    LogInfo("Guest executed VMXOFF\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMLAUNCH instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmlaunch(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // Emulate Windows VBS behaviour which locks CR4.VMSE at 0.
    // This means we can never execute VMLAUNCH and thus we can never have an active VMCS
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    if (CheckRegistersForException(VCpu))
    {
        return;
    }

    LogInfo("Guest executed VMLAUNCH\n");

    VMSucceed();
}

/**
 * @brief Handling VM exit related to VMXON instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationVmxon(VIRTUAL_MACHINE_STATE * VCpu)
{
    MSR    Msr_IA32_FEATURE_CONTROL = {0};
    UINT64 FetchedAddress;

    //
    // TODO: Check control flow (order of branches mostly)
    //

    //
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    ///
    /// Check whether safety features allow execution
    ///
    if ( // Register operand ||
        CheckRegistersForException(VCpu))
    {
        return;
    }
    else if (!(GetGuestCr4(VCpu) & (1ULL << 13)))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    VmxVmread64P(IA32_FEATURE_CONTROL, &Msr_IA32_FEATURE_CONTROL.Flags);

    if (VmxGetCurrentExecutionMode() ||
        !(Msr_IA32_FEATURE_CONTROL.Flags & 1ULL) ||
        !(Msr_IA32_FEATURE_CONTROL.Flags & (1ULL << 2)) // && OUTSIDE OF SMX OP
    )
    {
        EventInjectGeneralProtection();
        return;
    }

    //
    //  Fetch VMCS pointer and check its validity
    //
    if (!VmexitFetchAndCheckVmcsPointerAlignment(VCpu, &FetchedAddress))
    {
        return;
    }

    LogInfo("Guest executed VMXON\n");

    //
    //  Return VMSucceed to indicate that we did enter VMX mode
    //
    VMSucceed();
}

/**
 * @brief Handling VM exit related to INVEPT instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationInvept(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMX_SEGMENT_SELECTOR Cs;
    VMEXIT_INFO          ExitInfo = {0};
    UINT64               InveptType;
    UINT64               Displacement;
    UINT64               Address;
    INVEPT_DESCRIPTOR    Descriptor = {0};

    //
    // SDM 33.3, insert #UD if not in VMX.
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    Cs = GetGuestCs();

    if (Cs.Attributes.DescriptorPrivilegeLevel > 0)
    {
        EventInjectGeneralProtection();
        return;
    }

    //
    //  Find the type of INVVPID from the exit qualification
    //
    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    InveptType = GetRegister(VCpu, ExitInfo.Reg2);

    VmxVmread64P(VMCS_EXIT_QUALIFICATION, &Displacement);
    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, Displacement, &Address))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Descriptor, 16);

    switch (InveptType)
    {
    case InvvpidSingleContext:
    case InvvpidAllContext:
        EptInveptSingleContext(Descriptor.EptPointer);
        return;
    default:
        EventInjectUndefinedOpcode(VCpu);
        return;
    }
}

/**
 * @brief Handling VM exit related to INVVPID instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationInvvpid(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMX_SEGMENT_SELECTOR Cs;
    VMEXIT_INFO          ExitInfo = {0};
    UINT64               InvvpidType;
    UINT64               Displacement;
    UINT64               Address;
    INVVPID_DESCRIPTOR   Descriptor = {0};

    //
    // SDM 33.3, insert #UD if not in VMX.
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    Cs = GetGuestCs();

    if (Cs.Attributes.DescriptorPrivilegeLevel > 0)
    {
        EventInjectGeneralProtection();
        return;
    }

    //
    //  Find the type of INVVPID from the exit qualification
    //
    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    InvvpidType = GetRegister(VCpu, ExitInfo.Reg2);

    //
    //  Read the descriptor, the vmcs info field does not contain the displacement, so we need to
    //  disassemble the instruction to find it. After this we can use the exit info to calculate
    //  the operand address using the formula specified in chapter 3.7.5 of vol 1 of the SDM.
    //
    // Displacement = GetDescriptorDisplacementFromOperandByIndex(VCpu->LastVmexitRip, CommonIsGuestOnUsermode32Bit(), 1);
    VmxVmread64P(VMCS_EXIT_QUALIFICATION, &Displacement);
    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, Displacement, &Address))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }
    MemoryMapperReadMemorySafeOnTargetProcess(Address, &Descriptor, 16);

    switch (InvvpidType)
    {
    case InvvpidSingleContext:
    case InvvpidAllContext:
        if (Descriptor.Vpid == 0)
        {
            VMFailWithErrorCode(VMX_ERROR_INVEPT_INVVPID_INVALID_OPERAND);
            return;
        }
        VpidInvvpidSingleContext(Descriptor.Vpid);
        return;
    case InvvpidSingleContextRetainingGlobals:
        if (Descriptor.Vpid == 0)
        {
            VMFailWithErrorCode(VMX_ERROR_INVEPT_INVVPID_INVALID_OPERAND);
            return;
        }
        VpidInvvpidSingleContextRetainingGlobals(Descriptor.Vpid);
        VMSucceed();
        return;
    case InvvpidIndividualAddress:
        if (Descriptor.Vpid == 0)
        {
            VMFailWithErrorCode(VMX_ERROR_INVEPT_INVVPID_INVALID_OPERAND);
            return;
        }
        VpidInvvpidIndividualAddress(Descriptor.Vpid, Descriptor.LinearAddress);
        VMSucceed();
        return;
    default:
        EventInjectUndefinedOpcode(VCpu);
        return;
    }
}

/**
 * @brief Handling VM exit related to GETSEC instruction
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
VmxEmulationGetsec(VIRTUAL_MACHINE_STATE * VCpu)
{
    //
    // GETSEC only works if CPUID.01H:ECX[6] = 1.
    // Should likely check
    //

    //
    // Inject #UD if CR4.SMXE is disabled, meaning that GETSEC is disabled.
    //
    if (!(GetGuestCr4(VCpu) & (1ULL << 14)))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    //
    // Inject #UD if host has CR4.SMXE cleared to prevent host crash.
    // Guest should mimic host on startup, I am not sure if guest can ever be set while host is cleared
    //
    if (!(__readcr4() & (1ULL << 14)))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    long GetsecLeaf = VCpu->Regs->rax & 0xFFFFFFFF;
    switch (GetsecLeaf)
    {
    case GETSEC_LEAF_CAPABILITES:
    {
        if ((VCpu->Regs->rbx & 0xFFFFFFFF) == 0)
        {
            long capabilities = AsmGetsecCapabilities();
            VCpu->Regs->rax &= ~0xFFFFFFFF;
            VCpu->Regs->rax |= capabilities;
        }
        else
        {
            VCpu->Regs->rax &= ~0xFFFFFFFF;
        }

        LogInfo("Guest executed GETSEC[capabilities]\n");

        break;
    }
    case GETSEC_LEAF_ENTERACCS:
    case GETSEC_LEAF_EXITAC:
    case GETSEC_LEAF_SENTER:
    case GETSEC_LEAF_SEXIT:
    case GETSEC_LEAF_PARAMETERS:
    case GETSEC_LEAF_SMCTRL:
    case GETSEC_LEAF_WAKEUP:
    {
        LogInfo("Guest executed other GETSEC leaf, injecting #GP\n");

        //
        //  Inject #GP to spoof lack of TPM/TXT support.
        //
        EventInjectGeneralProtection();
        break;
    }
    default:
    {
        LogInfo("Guest executed reserved GETSEC leaf, injecting #UD\n");

        //
        //  Reserved leaf was called, inject #UD
        //
        EventInjectUndefinedOpcode(VCpu);
        break;
    }
    }

    return;
}

/**
 * @brief Set RFLAGS to indicate VMSucceed
 *
 * @return VOID
 */
VOID
VMSucceed()
{
    UINT64 Rflags = HvGetRflags();
    Rflags &= ~(X86_FLAGS_CF | X86_FLAGS_PF | X86_FLAGS_AF | X86_FLAGS_ZF | X86_FLAGS_SF | X86_FLAGS_OF);
    HvSetRflags(Rflags);
}

/**
 * @brief Set RFLAGS to indicate VMFail
 *
 * @return VOID
 */
VOID
VMFailWithoutErrorCode()
{
    UINT64 Rflags = HvGetRflags();
    Rflags &= ~(X86_FLAGS_PF | X86_FLAGS_AF | X86_FLAGS_ZF | X86_FLAGS_SF | X86_FLAGS_OF);
    HvSetRflags(Rflags | X86_FLAGS_CF);
}

/**
 * @brief Set RFLAGS to indicate VMFail
 *
 * @param VMfail errorcode as defined in chapter 33.4, vol 3c of the SDM
 * @return VOID
 */
VOID
VMFailWithErrorCode(UINT32 ErrorCode)
{
    UINT64 Rflags = HvGetRflags();
    Rflags &= ~(X86_FLAGS_CF | X86_FLAGS_PF | X86_FLAGS_AF | X86_FLAGS_SF | X86_FLAGS_OF);
    HvSetRflags(Rflags | X86_FLAGS_ZF);
    VmxVmwrite32(VMCS_VM_INSTRUCTION_ERROR, ErrorCode);
}

/**
 * @brief Check guest registers if exception should be raised
 *
 * @param VCpu The virtual processor's state
 * @return BOOLEAN
 */
BOOLEAN
CheckRegistersForException(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMX_SEGMENT_SELECTOR Cs;
    MSR                  Msr_IA32_EFER = {0};

    Cs = GetGuestCs();
    VmxVmread64P(IA32_EFER, &Msr_IA32_EFER.Flags);

    if (!(GetGuestCr0(VCpu) & 1ULL) ||
        !VmFuncVmxGetCurrentExecutionMode() ||
        (GetGuestRFlags(VCpu) & (1ULL << 17)) ||
        (Msr_IA32_EFER.Flags & (1ULL << 10) && !Cs.Attributes.LongMode))
    {
        EventInjectUndefinedOpcode(VCpu);
        return TRUE;
    }

    if (Cs.Attributes.DescriptorPrivilegeLevel != 0)
    {
        EventInjectGeneralProtection();
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Get the value of the VMCS info register fields
 *
 * @param VCpu The virtual processor's state
 * @param Register index
 * @return UINT64
 */
UINT64
GetRegister(VIRTUAL_MACHINE_STATE * VCpu, UINT32 EncodedRegister)
{
    switch (EncodedRegister)
    {
    case 0:
        return VCpu->Regs->rax;
    case 1:
        return VCpu->Regs->rcx;
    case 2:
        return VCpu->Regs->rdx;
    case 3:
        return VCpu->Regs->rbx;
    case 4:
        return VCpu->Regs->rsp;
    case 5:
        return VCpu->Regs->rbp;
    case 6:
        return VCpu->Regs->rsi;
    case 7:
        return VCpu->Regs->rdi;
    case 8:
        return VCpu->Regs->r8;
    case 9:
        return VCpu->Regs->r9;
    case 10:
        return VCpu->Regs->r10;
    case 11:
        return VCpu->Regs->r11;
    case 12:
        return VCpu->Regs->r12;
    case 13:
        return VCpu->Regs->r13;
    case 14:
        return VCpu->Regs->r14;
    case 15:
        return VCpu->Regs->r15;
    default:
        return 0;
    }
}

/**
 * @brief Calculate descriptor address
 *
 * @param Guest VCpu state
 * @param VMCS VM exit info structure
 * @param Displacement of memory operand
 * @param Address that will point to descriptor
 * @return BOOLEAN
 */
BOOLEAN
CalculateAddressFromExitInfo(VIRTUAL_MACHINE_STATE * VCpu,
                             VMEXIT_INFO             ExitInfo,
                             UINT64                  Displacement,
                             UINT64 *                Address)
{
    UINT64               BaseVal  = 0;
    UINT64               IndexVal = 0;
    UINT32               Scale    = 0;
    UINT64               Offset;
    VMX_SEGMENT_SELECTOR Seg;

    //
    // Check exit info validity bits, and fetch base and index values
    //
    if (!ExitInfo.BaseRegisterInvalid)
    {
        BaseVal = GetRegister(VCpu, ExitInfo.BaseRegister);
    }

    if (!ExitInfo.IndexRegisterInvalid)
    {
        IndexVal = GetRegister(VCpu, ExitInfo.IndexRegister);
        Scale    = 1ULL << ExitInfo.Scaling;
    }

    //
    // Calculate offset
    //
    Offset = BaseVal + IndexVal * Scale + Displacement;

    switch (ExitInfo.AddressSize)
    {
    case 0:
        Offset &= 0xFFFFULL;
        break;
    case 1:
        Offset &= 0xFFFFFFFFULL;
        break;
    case 2:
    default:
        break;
    }

    //
    // Fetch the segment base
    //
    switch (ExitInfo.SegmentRegister)
    {
    case 0:
        Seg = GetGuestEs();
        break;
    case 1:
        Seg = GetGuestCs();
        break;
    case 2:
        Seg = GetGuestSs();
        break;
    case 3:
        Seg = GetGuestDs();
        break;
    case 4:
        Seg = GetGuestFs();
        break;
    case 5:
        Seg = GetGuestGs();
        break;
    default:
        //
        // No valid segment register
        //
        return FALSE;
    }

    if (Seg.Attributes.Unusable)
        return FALSE;

    *Address = Seg.Base + Offset;
    return TRUE;
}

/**
 * @brief Fetch and calculate VMCS pointer from vmexit info, then check validity
 *
 * @param Guest VCpu state
 * @param Address pointing to VMCS
 * @return BOOLEAN
 */
BOOLEAN
VmexitFetchAndCheckVmcsPointerAlignment(VIRTUAL_MACHINE_STATE * VCpu, UINT64 * FetchedAddress)
{
    VMEXIT_INFO ExitInfo = {0};
    UINT64      Displacement;
    UINT64      Address;

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    VmxVmread64P(VMCS_EXIT_QUALIFICATION, &Displacement);
    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, Displacement, &Address))
    {
        EventInjectUndefinedOpcode(VCpu);
        return FALSE;
    }

    MemoryMapperReadMemorySafeOnTargetProcess(Address, FetchedAddress, 8);

    if (CheckAccessValidityAndSafety(Address, 8) ||
        *FetchedAddress % 4096 != 0)
    {
        VMFailWithErrorCode(VMX_ERROR_VMCLEAR_INVALID_PHYSICAL_ADDRESS);
        return FALSE;
    }
    else if (*FetchedAddress == VCpu->VmxonRegionPhysicalAddress)
    {
        VMFailWithErrorCode(VMX_ERROR_VMCLEAR_INVALID_VMXON_POINTER);
        return FALSE;
    }
    else if (*FetchedAddress == VCpu->VmcsRegionPhysicalAddress)
    {
        VMFailWithErrorCode(VMX_ERROR_VMCLEAR_INVALID_PHYSICAL_ADDRESS);
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
VmexitFetchVMCSField(VIRTUAL_MACHINE_STATE* VCpu, UINT64* FetchedAddress)
{
    VMEXIT_INFO ExitInfo = {0};
    UINT64      Displacement;
    UINT64      Address;

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    VmxVmread64P(VMCS_EXIT_QUALIFICATION, &Displacement);
    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, Displacement, &Address))
    {
        EventInjectUndefinedOpcode(VCpu);
        return FALSE;
    }

    MemoryMapperReadMemorySafeOnTargetProcess(Address, FetchedAddress, 8);

    return TRUE;
}
