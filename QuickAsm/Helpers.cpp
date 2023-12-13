#include "pch.h"
#include "Helpers.h"

std::wstring Helpers::GetTempFilePath(PCWSTR name) {
    WCHAR path[MAX_PATH]{};
    ::GetTempPath(_countof(path), path);
    ::GetTempFileName(path, name, 0, path);
    return path;
}

std::vector<BYTE> Helpers::ReadFileContents(PCWSTR path) {
    auto hFile = ::CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return {};
	
    std::vector<BYTE> buffer(::GetFileSize(hFile, nullptr));
	DWORD read;
	::ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &read, nullptr);
	::CloseHandle(hFile);
	return buffer;
}

PCWSTR Helpers::RegisterTypeToString(RegisterType type) {
    static const struct {
        RegisterType value;
        PCWSTR text;
    } types[] = {
        { RegisterType::General, L"General" },
        { RegisterType::Segment, L"Segment" },
        { RegisterType::Special, L"Special" },
        { RegisterType::FloatingPoint, L"Float" },
        { RegisterType::SSE, L"SSE" },
        { RegisterType::Control, L"Control" },
        { RegisterType::Descriptor, L"Descriptor" },
    };

    for (auto& t : types)
        if ((t.value & type) == t.value)
            return t.text;

    return L"";
}

wxString Helpers::CPUFlagsToString(uint32_t value) {
    static const struct {
        int bit;
        PCWSTR text;
    } flags[] = {
        { 0, L"C" },
        { 2, L"P" },
        { 4, L"A" },
        { 6, L"Z" },
        { 7, L"S" },
        { 10, L"D" },
        { 11, L"O" },
        { 14, L"NT" },
        { 16, L"RF" },
        { 17, L"VM" },
        { 18, L"AC" },
        { 19, L"VIF" },
        { 20, L"VIP" },
        { 21, L"ID" },
    };
    wxString text;
    for (auto& f : flags)
        if (value & (1 << f.bit))
            text += wxString(f.text) + L"=1 ";

    text += wxString(L"IOPL=") + std::to_wstring((value & (1 << 12) || (1 << 13)) >> 12).c_str();
    return text;
}
