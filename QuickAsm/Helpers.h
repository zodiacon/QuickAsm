#pragma once

#include "RegisterInfo.h"

struct Helpers abstract final {
	static std::wstring GetTempFilePath(PCWSTR name);
	static std::vector<BYTE> ReadFileContents(PCWSTR path);
	static PCWSTR RegisterTypeToString(RegisterType type);
	static CString CPUFlagsToString(uint32_t value);
	static CString CR0ToString(uint64_t value);
	// where nasm.exe is looked for, in order: the executable's folder, then the user's QuickAsm folder (if that isn't writable)
	static std::vector<std::wstring> GetNasmLocations();
	// writes the embedded nasm.exe to the first location it can, unless one has it already; returns its path (empty on failure)
	static std::wstring ExtractNasm();
	// a number typed by the user: decimal, or hex (0x1F or 1Fh); no octal, no sign
	static bool ParseNumber(CString text, uint64_t& value);
};

