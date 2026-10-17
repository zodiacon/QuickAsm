#pragma once

#include "AssemblerBase.h"

class NasmAssembler : public AssemblerBase {
public:
	NasmAssembler();
	AssemblerResults AssembleFile(std::wstring const& file) override;

	void SetNasmPath(std::wstring path);
	std::wstring const& GetNasmPath() const;

private:
	// the source line (0 based) of each offset, from nasm's listing (extraLines: the lines added before the source)
	static std::vector<std::pair<uint32_t, int>> ParseListing(std::wstring const& path, int extraLines);
	// the size of .text, from the map file, if other sections (data) follow it in the output; 0 otherwise
	static size_t GetCodeSize(std::wstring const& mapFile, int64_t origin);

	std::wstring m_NasmPath;
	std::wstring m_NasmDir;
};

