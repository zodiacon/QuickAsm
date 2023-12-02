#pragma once

struct Helpers abstract final {
	static std::wstring GetTempFilePath(PCWSTR name);
	static std::vector<BYTE> ReadFileContents(std::wstring const& path);
};

