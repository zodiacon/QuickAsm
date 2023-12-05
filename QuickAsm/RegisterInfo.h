#pragma once

#include "UnicornEngine.h"


enum class RegisterType {
	None = 0,
	General =		0x00001000,
	Segment =		0x00002000,
	FloatingPoint = 0x00004000,
	SSE =			0x00008000,
	Special =		0x00010000,
	Control =		0x00020000,
	Index =			0x00040000,
	Descriptor =	0x00080000,

	Bits16 = CS_MODE_16,
	Bits32 = CS_MODE_32,
	Bits64 = CS_MODE_64,

	AllBits = Bits16 | Bits32 | Bits64,
};
DEFINE_ENUM_FLAG_OPERATORS(RegisterType);

struct RegisterInfo {
	std::string Name;
	int Size;	// bits
	x86Register Id;
	RegisterType Category;
	x86Register SubReg;
	int StartBit;
};

extern const std::vector<RegisterInfo> AllRegisters;
