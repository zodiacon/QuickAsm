#include "pch.h"
#include "Helpers.h"

std::wstring Helpers::GetTempFilePath(PCWSTR name) {
    WCHAR path[MAX_PATH]{};
    ::GetTempPath(_countof(path), path);
    ::GetTempFileName(path, name, 0, path);
    return path;
}

std::vector<BYTE> Helpers::ReadFileContents(std::wstring const& path) {
    auto hFile = ::CreateFile(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return {};
	
    std::vector<BYTE> buffer(::GetFileSize(hFile, nullptr));
	DWORD read;
	::ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &read, nullptr);
	::CloseHandle(hFile);
	return buffer;
}
