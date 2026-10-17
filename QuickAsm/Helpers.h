#pragma once

#include "RegisterInfo.h"

struct Helpers abstract final {
	static std::wstring GetTempFilePath(PCWSTR name);
	static std::vector<BYTE> ReadFileContents(PCWSTR path);
	static PCWSTR RegisterTypeToString(RegisterType type);
	static CString CPUFlagsToString(uint32_t value);
	static CString CR0ToString(uint64_t value);
};

