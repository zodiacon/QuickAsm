#pragma once

#include "AssemblerBase.h"

class NasmAssembler : public AssemblerBase {
public:
	NasmAssembler();
	AssemblerResults AssembleFile(std::wstring const& file) override;
	AssemblerResults Assemble(std::string const& source) override;

	void SetNasmPath(std::wstring path);
	std::wstring const& GetNasmPath() const;

private:
	std::wstring m_NasmPath;
	std::wstring m_NasmDir;
};

