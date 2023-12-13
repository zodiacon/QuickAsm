#include "pch.h"
#include "RegisterInfo.h"

const std::vector<RegisterInfo> AllRegisters {
	{ "IP", 16, x86Register::IP, RegisterType::Special | RegisterType::Bits16 },
	{ "EIP", 32, x86Register::EIP, RegisterType::Special | RegisterType::Bits32 },
	{ "RIP", 64, x86Register::RIP, RegisterType::Special | RegisterType::Bits64 },

	{ "AX", 16, x86Register::AX, RegisterType::General | RegisterType::AllBits , x86Register::EAX, 0 },
	{ "AL", 8,  x86Register::AL, RegisterType::General | RegisterType::AllBits , x86Register::AX, 0 },
	{ "AH", 8,  x86Register::AH, RegisterType::General | RegisterType::AllBits , x86Register::AX, 8 },

	{ "BX", 16, x86Register::BX, RegisterType::General | RegisterType::AllBits, x86Register::EBX, 0 },
	{ "BL", 8,  x86Register::BL, RegisterType::General | RegisterType::AllBits, x86Register::BX, 0 },
	{ "BH", 8,  x86Register::BH, RegisterType::General | RegisterType::AllBits, x86Register::BX, 8 },

	{ "CX", 16, x86Register::CX, RegisterType::General | RegisterType::AllBits, x86Register::ECX, 0 },
	{ "CL", 8,  x86Register::CL, RegisterType::General | RegisterType::AllBits, x86Register::CX, 0 },
	{ "CH", 8,  x86Register::CH, RegisterType::General | RegisterType::AllBits, x86Register::CX, 8 },

	{ "DX", 16, x86Register::DX, RegisterType::General | RegisterType::AllBits, x86Register::EDX, 0 },
	{ "DL", 8,  x86Register::DL, RegisterType::General | RegisterType::AllBits, x86Register::DX, 0 },
	{ "DH", 8,  x86Register::DH, RegisterType::General | RegisterType::AllBits, x86Register::DX, 8 },

	{ "EAX", 32, x86Register::EAX, RegisterType::General | RegisterType::Bits32 | RegisterType::Bits64, x86Register::RAX },
	{ "EBX", 32, x86Register::EBX, RegisterType::General | RegisterType::Bits32 | RegisterType::Bits64, x86Register::RBX },
	{ "ECX", 32, x86Register::ECX, RegisterType::General | RegisterType::Bits32 | RegisterType::Bits64, x86Register::RCX },
	{ "EDX", 32, x86Register::EDX, RegisterType::General | RegisterType::Bits32 | RegisterType::Bits64, x86Register::RDX },

	{ "RAX", 64, x86Register::RAX, RegisterType::General | RegisterType::Bits64 },
	{ "RBX", 64, x86Register::RBX, RegisterType::General | RegisterType::Bits64 },
	{ "RCX", 64, x86Register::RCX, RegisterType::General | RegisterType::Bits64 },
	{ "RDX", 64, x86Register::RDX, RegisterType::General | RegisterType::Bits64 },

	{ "SP", 16, x86Register::SP, RegisterType::General | RegisterType::Bits16 },
	{ "ESP", 32, x86Register::ESP, RegisterType::General | RegisterType::Bits32 },
	{ "RSP", 64, x86Register::RSP, RegisterType::General | RegisterType::Bits64 },

	{ "BP", 16, x86Register::BP, RegisterType::General | RegisterType::Bits16 },
	{ "EBP", 32, x86Register::EBP, RegisterType::General | RegisterType::Bits32 },
	{ "RBP", 64, x86Register::RBP, RegisterType::General | RegisterType::Bits64 },

	{ "EFLAGS", 32, x86Register::EFLAGS, RegisterType::Special | RegisterType::Bits32 },
	{ "FLAGS", 16, x86Register::EFLAGS, RegisterType::Special | RegisterType::Bits16 },
	{ "RFLAGS", 64, x86Register::EFLAGS, RegisterType::Special | RegisterType::Bits64 },

	{ "CS", 16, x86Register::CS, RegisterType::Segment | RegisterType::AllBits },
	{ "DS", 16, x86Register::DS, RegisterType::Segment | RegisterType::AllBits },
	{ "ES", 16, x86Register::ES, RegisterType::Segment | RegisterType::AllBits },
	{ "SS", 16, x86Register::SS, RegisterType::Segment | RegisterType::AllBits },
	{ "FS", 16, x86Register::FS, RegisterType::Segment | RegisterType::Bits32 | RegisterType::Bits64 },
	{ "GS", 16, x86Register::GS, RegisterType::Segment | RegisterType::Bits32 | RegisterType::Bits64 },

	{ "SI", 16,   x86Register::SI, RegisterType::General | RegisterType::AllBits, x86Register::ESI, 0 },
	{ "ESI", 32,  x86Register::ESI, RegisterType::General | RegisterType::Bits64 | RegisterType::Bits32, x86Register::RSI, 0 },
	{ "RSI", 64,  x86Register::RSI, RegisterType::General | RegisterType::Bits64 },

	{ "DI", 16,   x86Register::DI, RegisterType::General | RegisterType::AllBits, x86Register::ESI, 0 },
	{ "EDI", 32,  x86Register::EDI, RegisterType::General | RegisterType::Bits64 | RegisterType::Bits32, x86Register::RDI, 0 },
	{ "RDI", 64,  x86Register::RDI, RegisterType::General | RegisterType::Bits64 },

	{ "R8", 64,  x86Register::R8, RegisterType::General | RegisterType::Bits64 },
	{ "R9", 64,  x86Register::R9, RegisterType::General | RegisterType::Bits64 },
	{ "R10", 64,  x86Register::R10, RegisterType::General | RegisterType::Bits64 },
	{ "R11", 64,  x86Register::R11, RegisterType::General | RegisterType::Bits64 },
	{ "R12", 64,  x86Register::R12, RegisterType::General | RegisterType::Bits64 },
	{ "R13", 64,  x86Register::R13, RegisterType::General | RegisterType::Bits64 },
	{ "R14", 64,  x86Register::R14, RegisterType::General | RegisterType::Bits64 },
	{ "R15", 64,  x86Register::R15, RegisterType::General | RegisterType::Bits64 },

	{ "R8D", 32,  x86Register::R8D, RegisterType::General | RegisterType::Bits64, x86Register::R8 },
	{ "R9D", 32,  x86Register::R9D, RegisterType::General | RegisterType::Bits64, x86Register::R9 },
	{ "R10D", 32,  x86Register::R10D, RegisterType::General | RegisterType::Bits64, x86Register::R10 },
	{ "R11D", 32,  x86Register::R11D, RegisterType::General | RegisterType::Bits64, x86Register::R11 },
	{ "R12D", 32,  x86Register::R12D, RegisterType::General | RegisterType::Bits64, x86Register::R12 },
	{ "R13D", 32,  x86Register::R13D, RegisterType::General | RegisterType::Bits64, x86Register::R13 },
	{ "R14D", 32,  x86Register::R14D, RegisterType::General | RegisterType::Bits64, x86Register::R14 },
	{ "R15D", 32,  x86Register::R15D, RegisterType::General | RegisterType::Bits64, x86Register::R15 },

	{ "R8W", 16,  x86Register::R8W, RegisterType::General | RegisterType::Bits64, x86Register::R8 },
	{ "R9W", 16,  x86Register::R9W, RegisterType::General | RegisterType::Bits64, x86Register::R9 },
	{ "R10W", 16,  x86Register::R10W, RegisterType::General | RegisterType::Bits64, x86Register::R10 },
	{ "R11W", 16,  x86Register::R11W, RegisterType::General | RegisterType::Bits64, x86Register::R11 },
	{ "R12W", 16,  x86Register::R12W, RegisterType::General | RegisterType::Bits64, x86Register::R12 },
	{ "R13W", 16,  x86Register::R13W, RegisterType::General | RegisterType::Bits64, x86Register::R13 },
	{ "R14W", 16,  x86Register::R14W, RegisterType::General | RegisterType::Bits64, x86Register::R14 },
	{ "R15W", 16,  x86Register::R15W, RegisterType::General | RegisterType::Bits64, x86Register::R15 },

	{ "R8B", 8,  x86Register::R8B, RegisterType::General | RegisterType::Bits64, x86Register::R8 },
	{ "R9B", 8,  x86Register::R9B, RegisterType::General | RegisterType::Bits64, x86Register::R9 },
	{ "R10B", 8,  x86Register::R10B, RegisterType::General | RegisterType::Bits64, x86Register::R10 },
	{ "R11B", 8,  x86Register::R11B, RegisterType::General | RegisterType::Bits64, x86Register::R11 },
	{ "R12B", 8,  x86Register::R12B, RegisterType::General | RegisterType::Bits64, x86Register::R12 },
	{ "R13B", 8,  x86Register::R13B, RegisterType::General | RegisterType::Bits64, x86Register::R13 },
	{ "R14B", 8,  x86Register::R14B, RegisterType::General | RegisterType::Bits64, x86Register::R14 },
	{ "R15B", 8,  x86Register::R15B, RegisterType::General | RegisterType::Bits64, x86Register::R15 },

	{ "CR0", 64,  x86Register::CR0, RegisterType::Control | RegisterType::Bits64 | RegisterType::Bits32 },
	{ "CR2", 64,  x86Register::CR2, RegisterType::Control | RegisterType::Bits64 | RegisterType::Bits32 },
	{ "CR3", 64,  x86Register::CR3, RegisterType::Control | RegisterType::Bits64 | RegisterType::Bits32 },
	{ "CR4", 64,  x86Register::CR4, RegisterType::Control | RegisterType::Bits64 | RegisterType::Bits32 },
	{ "CR8", 64,  x86Register::CR8, RegisterType::Control | RegisterType::Bits64 },

	{ "GDTR", 48,  x86Register::GDTR, RegisterType::Descriptor | RegisterType::Bits32 },
	{ "GDTR", 80,  x86Register::GDTR, RegisterType::Descriptor | RegisterType::Bits64 },
	{ "LDTR", 48,  x86Register::LDTR, RegisterType::Descriptor | RegisterType::Bits32 },
	{ "LDTR", 80,  x86Register::LDTR, RegisterType::Descriptor | RegisterType::Bits64 },
	{ "IDTR", 48,  x86Register::IDTR, RegisterType::Descriptor | RegisterType::Bits32 },
	{ "IDTR", 80,  x86Register::IDTR, RegisterType::Descriptor | RegisterType::Bits64 },
	{ "TR", 48,  x86Register::TR, RegisterType::Descriptor | RegisterType::Bits32 },
	{ "TR", 80,  x86Register::TR, RegisterType::Descriptor | RegisterType::Bits64 },

	//{ "ST0", 80,  x86Register::ST0, RegisterType::FloatingPoint },
	//{ "ST1", 80,  x86Register::ST1, RegisterType::FloatingPoint },
	//{ "ST2", 80,  x86Register::ST2, RegisterType::FloatingPoint },
	//{ "ST3", 80,  x86Register::ST3, RegisterType::FloatingPoint },
	//{ "ST4", 80,  x86Register::ST4, RegisterType::FloatingPoint },
	//{ "ST5", 80,  x86Register::ST5, RegisterType::FloatingPoint },
	//{ "ST6", 80,  x86Register::ST6, RegisterType::FloatingPoint },
	//{ "ST7", 80,  x86Register::ST7, RegisterType::FloatingPoint },

	//{ "MM0", 64,  x86Register::MM0, RegisterType::SSE },
	//{ "MM1", 64,  x86Register::MM1, RegisterType::SSE },
	//{ "MM2", 64,  x86Register::MM2, RegisterType::SSE },
	//{ "MM3", 64,  x86Register::MM3, RegisterType::SSE },
	//{ "MM4", 64,  x86Register::MM4, RegisterType::SSE },
	//{ "MM5", 64,  x86Register::MM5, RegisterType::SSE },
	//{ "MM6", 64,  x86Register::MM6, RegisterType::SSE },
	//{ "MM7", 64,  x86Register::MM7, RegisterType::SSE },

	//{ "XMM0", 128,  x86Register::XMM0, RegisterType::SSE },
	//{ "XMM1", 128,  x86Register::XMM1, RegisterType::SSE },
	//{ "XMM2", 128,  x86Register::XMM2, RegisterType::SSE },
	//{ "XMM3", 128,  x86Register::XMM3, RegisterType::SSE },
	//{ "XMM4", 128,  x86Register::XMM4, RegisterType::SSE },
};

