#pragma once

struct Instruction {
	uint64_t Address;
	std::vector<uint8_t> Bytes;
	std::string Mnemonic;
	std::string Operands;
	unsigned Id;
	std::unique_ptr<cs_detail> Details;
};

class CapstoneEngine final {
public:
	~CapstoneEngine();
	bool Open(cs_mode mode);
	std::vector<Instruction> Disassemble(uint8_t const* code, size_t size, uint64_t address);
	cs_err GetLastError() const;
	static const char* GetError(cs_err error);

private:
	csh m_CS{ 0 };
	cs_err m_Error;
};


