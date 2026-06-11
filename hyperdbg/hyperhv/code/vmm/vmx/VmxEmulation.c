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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    UINT64                  FetchedField;
    UINT64                  FieldValue;
    UINT64                  DestAddress;
    VMEXIT_INFO             ExitInfo = {0};

    //
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    FetchedField = GetRegister(VCpu, ExitInfo.Reg2);
    VmxVmread64P(FetchedField, &FieldValue);

    //
    // If not in transparency mode, we pass the VMCS field contents to the guest.
    //
    if (ExitInfo.VmReadWrite.MemReg)
    {
        SetRegister(VCpu, ExitInfo.VmReadWrite.Reg1, FieldValue);\
        LogInfo("Guest executed VMREAD: Field=0x%llx, Value=0x%llx\n", FetchedField, GetRegister(VCpu, ExitInfo.VmReadWrite.Reg1));
    }
    else
    {
        if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, VCpu->ExitQualification, &DestAddress))
        {
            EventInjectUndefinedOpcode(VCpu);
            return;
        }
        *(UINT64 *)DestAddress = FieldValue;
        LogInfo("Guest executed VMREAD: Field=0x%llx, Value=0x%llx\n", FetchedField, DestAddress);
    }

    VMSucceed();
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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    VMEXIT_INFO ExitInfo = {0};

    //
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    FieldValue   = GetRegister(VCpu, ExitInfo.VmReadWrite.Reg1);
    FetchedField = GetRegister(VCpu, ExitInfo.VmReadWrite.Reg2);

    //
    // We do not perform the write, as they are likely unrecoverable, and may cause deadlocks.
    // Instead log the attempt to notify the user in case they want to investigate further.
    //
    LogInfo("Guest tried to execute VMWRITE on field: 0x%llx, with value: 0x%llx\n", FetchedField, FieldValue);

    VMSucceed();
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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    // Emulate Windows VBS behaviour which locks CR4.VMXE at 0.
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
    //
    // We do not care about execution mode, as we already know it is VMX-root.
    // We also dont keep track of a guests IA32_FEATURE_CONTROL, so we assume it is set.
    // Either way we do not support SMX, so the feature doesn't hold much value.
    //

    UINT64 FetchedAddress;

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
    if (CheckRegistersForException(VCpu))
    {
        return;
    }
    else if (!(GetGuestCr4(VCpu) & (REG_CR4_VMXE)))
    {
        EventInjectUndefinedOpcode(VCpu);
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

    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, VCpu->ExitQualification, &Address))
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
    long capabilities;
    long GetsecLeaf;
    //
    // CR4.SMXE is never set in transparent mode.
    //
    if (g_CheckForFootprints)
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    //
    // Inject #UD is the guest does not have CR4.SMXE set.
    //
    if (!(GetGuestCr4(VCpu) & (1ULL << CR4_SMX_ENABLE_BIT)))
    {
        EventInjectUndefinedOpcode(VCpu);
        return;
    }

    GetsecLeaf = VCpu->Regs->rax & 0xFFFFFFFF;
    switch (GetsecLeaf)
    {
    case GETSEC_LEAF_CAPABILITES:
    {
        if ((VCpu->Regs->rbx & 0xFFFFFFFF) == 0)
        {
            //
            // verify the host wont #UD when executing GETSEC
            //
            if (!(CpuReadCr4() & (1ULL << CR4_SMX_ENABLE_BIT)))
            {
                EventInjectUndefinedOpcode(VCpu);
                return;
            }

            capabilities = AsmGetsecCapabilities();
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
 * @brief Get the value of the VMCS exit-info register fields
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
 * @brief Set the value of the VMCS exit-info register fields
 *
 * @param VCpu The virtual processor's state
 * @param Register index
 * @param Value to be written to register
 * @return VOID
 */
VOID
SetRegister(VIRTUAL_MACHINE_STATE * VCpu, UINT32 EncodedRegister, UINT64 Value)
{
    switch (EncodedRegister)
    {
    case 0:
        VCpu->Regs->rax = Value;
        break;
    case 1:
        VCpu->Regs->rcx = Value;
        break;
    case 2:
        VCpu->Regs->rdx = Value;
        break;
    case 3:
        VCpu->Regs->rbx = Value;
        break;
    case 4:
        VCpu->Regs->rsp = Value;
        break;
    case 5:
        VCpu->Regs->rbp = Value;
        break;
    case 6:
        VCpu->Regs->rsi = Value;
        break;
    case 7:
        VCpu->Regs->rdi = Value;
        break;
    case 8:
        VCpu->Regs->r8 = Value;
        break;
    case 9:
        VCpu->Regs->r9 = Value;
        break;
    case 10:
        VCpu->Regs->r10 = Value;
        break;
    case 11:
        VCpu->Regs->r11 = Value;
        break;
    case 12:
        VCpu->Regs->r12 = Value;
        break;
    case 13:
        VCpu->Regs->r13 = Value;
        break;
    case 14:
        VCpu->Regs->r14 = Value;
        break;
    case 15:
        VCpu->Regs->r15 = Value;
        break;
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
    VMX_SEGMENT_SELECTOR Segment;

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
        Segment = GetGuestEs();
        break;
    case 1:
        Segment = GetGuestCs();
        break;
    case 2:
        Segment = GetGuestSs();
        break;
    case 3:
        Segment = GetGuestDs();
        break;
    case 4:
        Segment = GetGuestFs();
        break;
    case 5:
        Segment = GetGuestGs();
        break;
    default:
        //
        // No valid segment register
        //
        return FALSE;
    }

    if (Segment.Attributes.Unusable)
        return FALSE;

    *Address = Segment.Base + Offset;
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
    UINT64      Address;

    VmxVmread32P(VMCS_VMEXIT_INSTRUCTION_INFO, (UINT32 *)&ExitInfo);
    if (!CalculateAddressFromExitInfo(VCpu, ExitInfo, VCpu->ExitQualification, &Address))
    {
        EventInjectUndefinedOpcode(VCpu);
        return FALSE;
    }

    MemoryMapperReadMemorySafeOnTargetProcess(Address, FetchedAddress, 8);

    if (CheckAccessValidityAndSafety(*FetchedAddress, 8) ||
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
