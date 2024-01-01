// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"

union CR0 {
    size_t All;
    struct {
        unsigned PE : 1;          //!< [0] Protected Mode Enabled
        unsigned MP : 1;          //!< [1] Monitor Coprocessor FLAG
        unsigned EM : 1;          //!< [2] Emulate FLAG
        unsigned TS : 1;          //!< [3] Task Switched FLAG
        unsigned ET : 1;          //!< [4] Extension Type FLAG
        unsigned NE : 1;          //!< [5] Numeric Error
        unsigned reserved1 : 10;  //!< [6:15]
        unsigned WP : 1;          //!< [16] Write Protect
        unsigned reserved2 : 1;   //!< [17]
        unsigned AM : 1;          //!< [18] Alignment Mask
        unsigned reserved3 : 10;  //!< [19:28]
        unsigned NW : 1;          //!< [29] Not Write-Through
        unsigned CD : 1;          //!< [30] Cache Disable
        unsigned PG : 1;          //!< [31] Paging Enabled
    };
};
static_assert(sizeof(CR0) == sizeof(void*), "Size check");

union CR4 {
    size_t All;
    struct {
        unsigned VME : 1;        //!< [0] Virtual Mode Extensions
        unsigned PVI : 1;        //!< [1] Protected-Mode Virtual Interrupts
        unsigned TSD : 1;        //!< [2] Time Stamp Disable
        unsigned DE : 1;         //!< [3] Debugging Extensions
        unsigned PSE : 1;        //!< [4] Page Size Extensions
        unsigned PAE : 1;        //!< [5] Physical Address Extension
        unsigned MCE : 1;        //!< [6] Machine-Check Enable
        unsigned PGE : 1;        //!< [7] Page Global Enable
        unsigned PCE : 1;        //!< [8] Performance-Monitoring Counter Enable
        unsigned OSFXSR : 1;     //!< [9] OS Support for FXSAVE/FXRSTOR
        unsigned OSXMMEXCPT : 1; //!< [10] OS Support for Unmasked SIMD Exceptions
        unsigned reserved1 : 2;  //!< [11:12]
        unsigned VMXE : 1;       //!< [13] Virtual Machine Extensions Enabled
        unsigned SMXE : 1;       //!< [14] SMX-Enable Bit
        unsigned reserved2 : 2;  //!< [15:16]
        unsigned PCIDE : 1;      //!< [17] PCID Enable
        unsigned OSXSAVE : 1;    //!< [18] XSAVE and Processor Extended States-Enable
        unsigned reserved3 : 1;  //!< [19]
        unsigned SMEP : 1;       //!< [20] Supervisor Mode Execution Protection Enable
        unsigned SMAP : 1;       //!< [21] Supervisor Mode Access Protection Enable
    };
};
static_assert(sizeof(CR4) == sizeof(void*), "Size check");

union DR6 {
    size_t All;
    struct {
        unsigned B0 : 1;          //!< [0] Breakpoint Condition Detected 0
        unsigned B1 : 1;          //!< [1] Breakpoint Condition Detected 1
        unsigned B2 : 1;          //!< [2] Breakpoint Condition Detected 2
        unsigned B3 : 1;          //!< [3] Breakpoint Condition Detected 3
        unsigned reserved1 : 8;   //!< [4:11] Always 1
        unsigned reserved2 : 1;   //!< [12] Always 0
        unsigned BD : 1;          //!< [13] Debug Register Access Detected
        unsigned BS : 1;          //!< [14] Single Step
        unsigned BT : 1;          //!< [15] Task Switch
        unsigned RTM : 1;         //!< [16] Restricted Transactional Memory
        unsigned reserved3 : 15;  //!< [17:31] Always 1
    };
};
static_assert(sizeof(DR6) == sizeof(void*), "Size check");

/// See: Debug Control Register (DR7)
union DR7 {
    size_t All;
    struct {
        unsigned L0 : 1;         //!< [0] Local Breakpoint Enable 0
        unsigned G0 : 1;         //!< [1] Global Breakpoint Enable 0
        unsigned L1 : 1;         //!< [2] Local Breakpoint Enable 1
        unsigned G1 : 1;         //!< [3] Global Breakpoint Enable 1
        unsigned L2 : 1;         //!< [4] Local Breakpoint Enable 2
        unsigned G2 : 1;         //!< [5] Global Breakpoint Enable 2
        unsigned L3 : 1;         //!< [6] Local Breakpoint Enable 3
        unsigned G3 : 1;         //!< [7] Global Breakpoint Enable 3
        unsigned LE : 1;         //!< [8] Local Exact Breakpoint Enable
        unsigned GE : 1;         //!< [9] Global Exact Breakpoint Enable
        unsigned reserved1 : 1;  //!< [10] Always 1
        unsigned RTM : 1;        //!< [11] Restricted Transactional Memory
        unsigned reserved2 : 1;  //!< [12] Always 0
        unsigned GD : 1;         //!< [13] General Detect Enable
        unsigned reserved3 : 2;  //!< [14:15] Always 0
        unsigned RW0 : 2;        //!< [16:17] Read / Write 0
        unsigned LEN0 : 2;       //!< [18:19] Length 0
        unsigned RW1 : 2;        //!< [20:21] Read / Write 1
        unsigned LEN1 : 2;       //!< [22:23] Length 1
        unsigned RW2 : 2;        //!< [24:25] Read / Write 2
        unsigned LEN2 : 2;       //!< [26:27] Length 2
        unsigned RW3 : 2;        //!< [28:29] Read / Write 3
        unsigned LEN3 : 2;       //!< [30:31] Length 3
    };
};
static_assert(sizeof(DR7) == sizeof(void*), "Size check");

#include <pshpack1.h>
struct IDTR {
    unsigned short Limit;
    size_t Base;
};

struct IDTR32 {
    unsigned short Limit;
    unsigned Base;
};
static_assert(sizeof(IDTR32) == 6, "Size check");

union SegmentSelector {
    unsigned short All;
    struct {
        unsigned short RPL : 2;  //!< Requested Privilege Level
        unsigned short TI : 1;   //!< Table Indicator
        unsigned short Index : 13;
    };
};
static_assert(sizeof(SegmentSelector) == 2, "Size check");

union SegmentDescriptor {
    unsigned long long All;
    struct {
        unsigned long long LimitLow : 16;
        unsigned long long BaseLow : 16;
        unsigned long long BaseMid : 8;
        unsigned long long Type : 4;
        unsigned long long System : 1;
        unsigned long long DPL : 2;
        unsigned long long Present : 1;
        unsigned long long LimitHigh : 4;
        unsigned long long AVL : 1;
        unsigned long long L : 1;  //!< 64-bit code segment (IA-32e mode only)
        unsigned long long DB : 1;
        unsigned long long Gran : 1;
        unsigned long long BaseHigh : 8;
    };
};
static_assert(sizeof(SegmentDescriptor) == 8, "Size check");

union CpuFeaturesEcx {
    unsigned All;
    struct {
        unsigned SSE3 : 1;       //!< [0] Streaming SIMD Extensions 3 (SSE3)
        unsigned PCLMULQDQ : 1;  //!< [1] PCLMULQDQ
        unsigned DTES64 : 1;     //!< [2] 64-bit DS Area
        unsigned MONITOR : 1;    //!< [3] MONITOR/WAIT
        unsigned DS_CPL : 1;     //!< [4] CPL qualified Debug Store
        unsigned VMX : 1;        //!< [5] Virtual Machine Technology
        unsigned SMX : 1;        //!< [6] Safer Mode Extensions
        unsigned EST : 1;        //!< [7] Enhanced Intel Speedstep Technology
        unsigned TM2 : 1;        //!< [8] Thermal monitor 2
        unsigned SSSE3 : 1;      //!< [9] Supplemental Streaming SIMD Extensions 3
        unsigned CID : 1;        //!< [10] L1 context ID
        unsigned SDBG : 1;       //!< [11] IA32_DEBUG_INTERFACE MSR
        unsigned FMA : 1;        //!< [12] FMA extensions using YMM state
        unsigned CX16 : 1;       //!< [13] CMPXCHG16B
        unsigned XTPR : 1;       //!< [14] xTPR Update Control
        unsigned PDCM : 1;       //!< [15] Performance/Debug capability MSR
        unsigned reserved : 1;   //!< [16] Reserved
        unsigned PCID : 1;       //!< [17] Process-context identifiers
        unsigned DCA : 1;        //!< [18] prefetch from a memory mapped device
        unsigned SSE4_1 : 1;     //!< [19] SSE4.1
        unsigned SSE4_2 : 1;     //!< [20] SSE4.2
        unsigned X2_APIC : 1;    //!< [21] x2APIC feature
        unsigned MOVBE : 1;      //!< [22] MOVBE instruction
        unsigned POPCNT : 1;     //!< [23] POPCNT instruction
        unsigned reserved2 : 1;  //!< [24] one-shot operation using a TSC deadline
        unsigned AES : 1;        //!< [25] AESNI instruction
        unsigned XSAVE : 1;      //!< [26] XSAVE/XRSTOR feature
        unsigned OSXSAVE : 1;    //!< [27] enable XSETBV/XGETBV instructions
        unsigned AVX : 1;        //!< [28] AVX instruction extensions
        unsigned F16C : 1;       //!< [29] 16-bit floating-point conversion
        unsigned RDRAND : 1;     //!< [30] RDRAND instruction
        unsigned unused : 1;   //!< [31] Always 0 (a.k.a. HypervisorPresent)
    };
};
static_assert(sizeof(CpuFeaturesEcx) == 4, "Size check");

union CpuFeaturesEdx {
    unsigned All;
    struct {
        unsigned FPU : 1;        //!< [0] Floating Point Unit On-Chip
        unsigned VME : 1;        //!< [1] Virtual 8086 Mode Enhancements
        unsigned DE : 1;         //!< [2] Debugging Extensions
        unsigned PSE : 1;        //!< [3] Page Size Extension
        unsigned TSC : 1;        //!< [4] Time Stamp Counter
        unsigned MSR : 1;        //!< [5] RDMSR and WRMSR Instructions
        unsigned MCE : 1;        //!< [7] Machine Check Exception
        unsigned CX8 : 1;        //!< [8] Thermal monitor 2
        unsigned APIC : 1;       //!< [9] APIC On-Chip
        unsigned reserved1 : 1;  //!< [10] Reserved
        unsigned SEP : 1;        //!< [11] SYSENTER and SYSEXIT Instructions
        unsigned MTRR : 1;       //!< [12] Memory Type Range Registers
        unsigned PGE : 1;        //!< [13] Page Global Bit
        unsigned MCA : 1;        //!< [14] Machine Check Architecture
        unsigned CMOV : 1;       //!< [15] Conditional Move Instructions
        unsigned PAT : 1;        //!< [16] Page Attribute Table
        unsigned PSE36 : 1;      //!< [17] 36-Bit Page Size Extension
        unsigned PSN : 1;        //!< [18] Processor Serial Number
        unsigned CLFSH : 1;      //!< [19] CLFLUSH Instruction
        unsigned reserved2 : 1;  //!< [20] Reserved
        unsigned DS : 1;         //!< [21] Debug Store
        unsigned ACPI : 1;       //!< [22] TM and Software Controlled Clock
        unsigned MMX : 1;        //!< [23] Intel MMX Technology
        unsigned FXSR : 1;       //!< [24] FXSAVE and FXRSTOR Instructions
        unsigned SSE : 1;        //!< [25] SSE
        unsigned SSE2 : 1;       //!< [26] SSE2
        unsigned SS : 1;         //!< [27] Self Snoop
        unsigned HTT : 1;        //!< [28] Max APIC IDs reserved field is Valid
        unsigned TM : 1;         //!< [29] Thermal Monitor
        unsigned reserved3 : 1;  //!< [30] Reserved
        unsigned PBE : 1;        //!< [31] Pending Break Enable
    };
};
static_assert(sizeof(CpuFeaturesEdx) == 4, "Size check");

struct HardwarePteX86 {
    unsigned Valid : 1;               //!< [0]
    unsigned Write : 1;               //!< [1]
    unsigned Owner : 1;               //!< [2]
    unsigned WriteThrough : 1;       //!< [3]
    unsigned CacheDisable : 1;       //!< [4]
    unsigned Accessed : 1;            //!< [5]
    unsigned Dirty : 1;               //!< [6]
    unsigned LargePage : 1;          //!< [7]
    unsigned Global : 1;              //!< [8]
    unsigned CopyOnWrite : 1;       //!< [9]
    unsigned Prototype : 1;           //!< [10]
    unsigned reserved : 1;           //!< [11]
    unsigned PageFrameNumber : 20;  //!< [12:31]
};
static_assert(sizeof(HardwarePteX86) == 4, "Size check");

/// nt!_HARDWARE_PTE on x86 PAE-enabled Windows
struct HardwarePteX86PAE {
    unsigned long long Valid : 1;               //!< [0]
    unsigned long long Write : 1;               //!< [1]
    unsigned long long Owner : 1;               //!< [2]
    unsigned long long WriteThrough : 1;       //!< [3]     PWT
    unsigned long long CacheDisable : 1;       //!< [4]     PCD
    unsigned long long Accessed : 1;            //!< [5]
    unsigned long long Dirty : 1;               //!< [6]
    unsigned long long LargePage : 1;          //!< [7]     PAT
    unsigned long long Global : 1;              //!< [8]
    unsigned long long CopyOnWrite : 1;       //!< [9]
    unsigned long long Prototype : 1;           //!< [10]
    unsigned long long reserved1 : 1;           //!< [11]
    unsigned long long PageFrameNumber : 26;  //!< [12:37]
    unsigned long long Reserved2 : 25;          //!< [38:62]
    unsigned long long NoExecute : 1;          //!< [63]
};
static_assert(sizeof(HardwarePteX86PAE) == 8, "Size check");

/// nt!_HARDWARE_PTE on x64 Windows
struct HardwarePteX64 {
    unsigned long long Valid : 1;               //!< [0]
    unsigned long long Write : 1;               //!< [1]
    unsigned long long Owner : 1;               //!< [2]
    unsigned long long WriteThrough : 1;       //!< [3]     PWT
    unsigned long long CacheDisable : 1;       //!< [4]     PCD
    unsigned long long Accessed : 1;            //!< [5]
    unsigned long long Dirty : 1;               //!< [6]
    unsigned long long LargePage : 1;          //!< [7]     PAT
    unsigned long long Global : 1;              //!< [8]
    unsigned long long CopyOnWrite : 1;       //!< [9]
    unsigned long long Prototype : 1;           //!< [10]
    unsigned long long reserved1 : 1;           //!< [11]
    unsigned long long PageFrameNumber : 36;  //!< [12:47]
    unsigned long long reserved2 : 4;           //!< [48:51]
    unsigned long long SoftwareWsIndex : 11;  //!< [52:62]
    unsigned long long NoExecute : 1;          //!< [63]
};
static_assert(sizeof(HardwarePteX64) == 8, "Size check");

enum class MSR : unsigned int {
    ApicBase = 0x01B,

    FeatureControl = 0x03A,

    SysenterCs = 0x174,
    SysenterEsp = 0x175,
    SysenterEip = 0x176,

    Debugctl = 0x1D9,

    MtrrCap = 0xFE,
    MtrrDefType = 0x2FF,
    MtrrPhysBaseN = 0x200,
    MtrrPhysMaskN = 0x201,
    MtrrFix64k00000 = 0x250,
    MtrrFix16k80000 = 0x258,
    MtrrFix16kA0000 = 0x259,
    MtrrFix4kC0000 = 0x268,
    MtrrFix4kC8000 = 0x269,
    MtrrFix4kD0000 = 0x26A,
    MtrrFix4kD8000 = 0x26B,
    MtrrFix4kE0000 = 0x26C,
    MtrrFix4kE8000 = 0x26D,
    MtrrFix4kF0000 = 0x26E,
    MtrrFix4kF8000 = 0x26F,

    VmxBasic = 0x480,
    VmxPinbasedCtls = 0x481,
    VmxProcBasedCtls = 0x482,
    VmxExitCtls = 0x483,
    VmxEntryCtls = 0x484,
    VmxMisc = 0x485,
    VmxCr0Fixed0 = 0x486,
    VmxCr0Fixed1 = 0x487,
    VmxCr4Fixed0 = 0x488,
    VmxCr4Fixed1 = 0x489,
    VmxVmcsEnum = 0x48A,
    VmxProcBasedCtls2 = 0x48B,
    VmxEptVpidCap = 0x48C,
    VmxTruePinbasedCtls = 0x48D,
    VmxTrueProcBasedCtls = 0x48E,
    VmxTrueExitCtls = 0x48F,
    VmxTrueEntryCtls = 0x490,
    VmxVmfunc = 0x491,

    Efer = 0xC0000080,
    Star = 0xC0000081,
    Lstar = 0xC0000082,

    Fmask = 0xC0000084,

    FsBase = 0xC0000100,
    GsBase = 0xC0000101,
    KernelGsBase = 0xC0000102,
    TscAux = 0xC0000103,
};
