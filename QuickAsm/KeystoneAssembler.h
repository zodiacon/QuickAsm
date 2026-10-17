#pragma once

#include "AssemblerBase.h"

class KeystoneAssembler : public AssemblerBase {
public:
	KeystoneAssembler();
	AssemblerResults AssembleFile(std::wstring const& file) override;
	AssemblerResults Assemble(std::string const& source) override;

	void SetMode(ks_mode mode);
	ks_mode GetMode() const;
	void SetAddress(uint64_t address);
	uint64_t GetAddress() const;

private:
	// Keystone (LLVM) takes ';' as a statement separator, not as the start of a comment: the comments go
	static std::string RemoveComments(std::string const& source);

	ks_mode m_Mode{ KS_MODE_32 };
	uint64_t m_Address{ 0 };
};

