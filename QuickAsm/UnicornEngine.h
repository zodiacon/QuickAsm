#pragma once

enum class CpuArch {
	Arm = UC_ARCH_ARM,   // ARM architecture (including Thumb, Thumb-2)
	Arm64 = UC_ARCH_ARM64,      // ARM-64, also called AArch64
	x86 = UC_ARCH_X86,        // X86 architecture (including x86 & x86-64)
};

enum class CpuMode {
	LittleEndian = UC_MODE_LITTLE_ENDIAN,    // little-endian mode (default mode)
	BigEngian = UC_MODE_BIG_ENDIAN, // big-endian mode

	// arm / arm64
	Arm = UC_MODE_ARM,              // ARM mode
	Thumb = UC_MODE_THUMB,       // THUMB mode (including Thumb-2)
	MClass = UC_MODE_MCLASS,      // ARM's Cortex-M series (currently unsupported)
	V8 = UC_MODE_V8,          // ARMv8 A32 encodings for ARM (currently unsupported)

	// arm (32bit) cpu types
	Arm926 = UC_MODE_ARM926,	  // ARM926 CPU type
	Arm946 = UC_MODE_ARM946,	  // ARM946 CPU type
	Arm1176 = UC_MODE_ARM1176,	  // ARM1176 CPU type

	// ARM BE8
	ArmBe8 = UC_MODE_ARMBE8,     // Big-endian data and Little-endian code

	// x86 / x64
	x86_16 = UC_MODE_16,          // 16-bit mode
	x86_32 = UC_MODE_32,          // 32-bit mode
	x86_64 = UC_MODE_64,          // 64-bit mode
};

enum class x86Register {
	_INVALID = 0,
	AH, AL, AX, BH, BL,
	BP, BPL, BX, CH, CL,
	CS, CX, DH, DI, DIL,
	DL, DS, DX, EAX, EBP,
	EBX, ECX, EDI, EDX, EFLAGS,
	EIP, EIZ, ES, ESI, ESP,
	FPSW, FS, GS, IP, RAX,
	RBP, RBX, RCX, RDI, RDX,
	RIP, RIZ, RSI, RSP, SI,
	SIL, SP, SPL, SS, CR0,
	CR1, CR2, CR3, CR4, CR5,
	CR6, CR7, CR8, CR9, CR10,
	CR11, CR12, CR13, CR14, CR15,
	DR0, DR1, DR2, DR3, DR4,
	DR5, DR6, DR7, DR8, DR9,
	DR10, DR11, DR12, DR13, DR14,
	DR15, FP0, FP1, FP2, FP3,
	FP4, FP5, FP6, FP7,
	K0, K1, K2, K3, K4,
	K5, K6, K7, MM0, MM1,
	MM2, MM3, MM4, MM5, MM6,
	MM7, R8, R9, R10, R11,
	R12, R13, R14, R15,
	ST0, ST1, ST2, ST3,
	ST4, ST5, ST6, ST7,
	XMM0, XMM1, XMM2, XMM3, XMM4,
	XMM5, XMM6, XMM7, XMM8, XMM9,
	XMM10, XMM11, XMM12, XMM13, XMM14,
	XMM15, XMM16, XMM17, XMM18, XMM19,
	XMM20, XMM21, XMM22, XMM23, XMM24,
	XMM25, XMM26, XMM27, XMM28, XMM29,
	XMM30, XMM31, YMM0, YMM1, YMM2,
	YMM3, YMM4, YMM5, YMM6, YMM7,
	YMM8, YMM9, YMM10, YMM11, YMM12,
	YMM13, YMM14, YMM15, YMM16, YMM17,
	YMM18, YMM19, YMM20, YMM21, YMM22,
	YMM23, YMM24, YMM25, YMM26, YMM27,
	YMM28, YMM29, YMM30, YMM31, ZMM0,
	ZMM1, ZMM2, ZMM3, ZMM4, ZMM5,
	ZMM6, ZMM7, ZMM8, ZMM9, ZMM10,
	ZMM11, ZMM12, ZMM13, ZMM14, ZMM15,
	ZMM16, ZMM17, ZMM18, ZMM19, ZMM20,
	ZMM21, ZMM22, ZMM23, ZMM24, ZMM25,
	ZMM26, ZMM27, ZMM28, ZMM29, ZMM30,
	ZMM31, R8B, R9B, R10B, R11B,
	R12B, R13B, R14B, R15B, R8D,
	R9D, R10D, R11D, R12D, R13D,
	R14D, R15D, R8W, R9W, R10W,
	R11W, R12W, R13W, R14W, R15W,
	IDTR, GDTR, LDTR, TR, FPCW,
	FPTAG,
	MSR, // Model-Specific Register
	MXCSR,
	FS_BASE, // Base regs for x86_64
	GS_BASE,
	_Count
};

enum class MemoryType {
	READ = 16,   // Memory is read from
	WRITE,       // Memory is written to
	FETCH,       // Memory is fetched
	READ_UNMAPPED,    // Unmapped memory is read from
	WRITE_UNMAPPED,   // Unmapped memory is written to
	FETCH_UNMAPPED,   // Unmapped memory is fetched
	WRITE_PROT,  // Write to write protected, but mapped, memory
	READ_PROT,   // Read from read protected, but mapped, memory
	FETCH_PROT,  // Fetch from non-executable, but mapped, memory
	READ_AFTER,   // Memory is read from (successful access)
};

// All type of hooks for uc_hook_add() API.
enum class HookType {
	// Hook all interrupt/syscall events
	INTR = 1 << 0,
	// Hook a particular instruction - only a very small subset of instructions supported here
	INSN = 1 << 1,
	// Hook a range of code
	CODE = 1 << 2,
	// Hook basic blocks
	BLOCK = 1 << 3,
	// Hook for memory read on unmapped memory
	MEM_READ_UNMAPPED = 1 << 4,
	// Hook for invalid memory write events
	MEM_WRITE_UNMAPPED = 1 << 5,
	// Hook for invalid memory fetch for execution events
	MEM_FETCH_UNMAPPED = 1 << 6,
	// Hook for memory read on read-protected memory
	MEM_READ_PROT = 1 << 7,
	// Hook for memory write on write-protected memory
	MEM_WRITE_PROT = 1 << 8,
	// Hook for memory fetch on non-executable memory
	MEM_FETCH_PROT = 1 << 9,
	// Hook memory read events.
	MEM_READ = 1 << 10,
	// Hook memory write events.
	MEM_WRITE = 1 << 11,
	// Hook memory fetch for execution events
	MEM_FETCH = 1 << 12,
	// Hook memory read events, but only successful access.
	// The callback will be triggered after successful read.
	MEM_READ_AFTER = 1 << 13,
	// Hook invalid instructions exceptions.
	INSN_INVALID = 1 << 14,
};

enum class MemProtection {
	None = UC_PROT_NONE,
	Read = UC_PROT_READ,
	Write = UC_PROT_WRITE,
	Execute = UC_PROT_EXEC,
	All = UC_PROT_ALL,
};
DEFINE_ENUM_FLAG_OPERATORS(MemProtection);

class UnicornEngine {
public:
	UnicornEngine() = default;
	~UnicornEngine();
	UnicornEngine(UnicornEngine const&) = delete;
	UnicornEngine& operator=(UnicornEngine const&) = delete;

	bool Open(CpuArch arch, CpuMode mode);
	void Close();
	bool IsOpen() const;

	uc_err LastError() const;

	CpuMode GetMode() const;

	template<typename T> requires std::is_trivially_copyable_v<T>
	bool WriteReg(x86Register reg, T value) {
		m_LastError = uc_reg_write(m_UC, (int)reg, &value);
		return m_LastError == UC_ERR_OK;
	}

	bool WriteReg(x86Register reg, void* value);
	bool ReadReg(x86Register reg, void* value) const;

	template<typename T> requires std::is_trivially_copyable_v<T>
	T ReadReg(x86Register reg) const {
		T value{};
		m_LastError = uc_reg_read(m_UC, (int)reg, &value);
		return m_LastError == UC_ERR_OK ? value : T{};
	}

	bool MapMemory(uint64_t address, uint64_t size, MemProtection protect);
	bool MapHostMemory(uint64_t address, uint64_t size, MemProtection protect, void* mem);
	bool UnmapMemory(uint64_t address, uint64_t size);
	bool ProtectMemory(uint64_t address, uint64_t size, MemProtection protect);
	bool WriteMemory(uint64_t address, const void* data, uint64_t size);
	bool ReadMemory(uint64_t address, void* data, uint64_t size);
	void UnmapAllMemory();

	bool Start(uint64_t address, uint64_t until, uint64_t timeout = 0, size_t count = 0);
	bool Stop();

	void* HookCode(HookType type, std::function<void(uint64_t, uint32_t)> cb, uint64_t begin, uint64_t end);
	void Unhook(void* hook);

	struct CpuContext {
		friend class UnicornEngine;
		~CpuContext();
		bool Save();
		bool Restore();

	private:
		CpuContext(uc_engine* uc);

	private:
		uc_context* m_ctx;
		uc_engine* m_uc;
	};

	CpuContext AllocContext();
	void EnableHooks(bool enable);

	void* SaveContext(void* ctx = nullptr);
	bool RestoreContext(void* ctx);
	const char* GetErrorText(uc_err error) const;

private:
	static void _CallbackHookCode(uc_engine* uc, uint64_t address, uint32_t size, void* ud);

	struct HookHandle {
		uc_hook m_Hook{ 0 };
		UnicornEngine* m_Engine;
		std::function<void(uint64_t, uint32_t)> m_CB;
	};
	uc_engine* m_UC{ nullptr };
	mutable uc_err m_LastError{ UC_ERR_OK };
	CpuMode m_Mode;
	bool m_HooksEnabled{ true };
};

