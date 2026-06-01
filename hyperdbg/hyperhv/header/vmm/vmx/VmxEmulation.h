#pragma once

#define GETSEC_LEAF_CAPABILITES 0
#define GETSEC_LEAF_ENTERACCS   2
#define GETSEC_LEAF_EXITAC      3
#define GETSEC_LEAF_SENTER      4
#define GETSEC_LEAF_SEXIT       5
#define GETSEC_LEAF_PARAMETERS  6
#define GETSEC_LEAF_SMCTRL      7
#define GETSEC_LEAF_WAKEUP      8

typedef struct _VMEXIT_INFO
{
        UINT32 Scaling : 2;
        UINT32 Undefined1 : 5;
        UINT32 AddressSize : 3;
        UINT32 Cleared : 1;
        UINT32 Undefined2 : 4;
        UINT32 SegmentRegister : 3;
        UINT32 IndexRegister : 4;
        UINT32 IndexRegisterInvalid : 1;
        UINT32 BaseRegister : 4;
        UINT32 BaseRegisterInvalid : 1;

        union
        {
            UINT32 Reg2 : 4;
            UINT32 Undefined3 : 4;
        };
    
} VMEXIT_INFO, *P_VMEXIT_INFO;

extern long inline AsmGetsecCapabilities(void);

VOID
VmxEmulationVmclear(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmptrld(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmptrst(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmread(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmresume(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmwrite(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmxoff(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmlaunch(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationVmxon(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationInvept(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationInvvpid(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VmxEmulationGetsec(VIRTUAL_MACHINE_STATE * VCpu);

VOID
VMSucceed();

VOID
VMFailWithoutErrorCode();

VOID
VMFailWithErrorCode(UINT32 ErrorCode);

BOOLEAN
CheckRegistersForException(VIRTUAL_MACHINE_STATE * VCpu);

UINT64
GetRegister(VIRTUAL_MACHINE_STATE * VCpu, UINT32 EncodedRegister);

BOOLEAN
CalculateAddressFromExitInfo(VIRTUAL_MACHINE_STATE * VCpu,
                             VMEXIT_INFO             ExitInfo,
                             UINT64                  Displacement,
                             UINT64 *                Address);

BOOLEAN
VmexitFetchAndCheckVmcsPointerAlignment(VIRTUAL_MACHINE_STATE * VCpu, UINT64 * FetchedAddress);

BOOLEAN
VmexitFetchVMCSField(VIRTUAL_MACHINE_STATE * VCpu, UINT64 * FetchedAddress);
