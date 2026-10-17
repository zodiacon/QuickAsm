#include "pch.h"
#include <charconv>
#include "Helpers.h"
#include "resource.h"
#include <ShlObj.h>
#include "..\x64Types\x64Types.h"

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
        { RegisterType::FloatingPoint, L"x87" },
        { RegisterType::SSE, L"SSE" },
        { RegisterType::AVX, L"AVX" },
        { RegisterType::Control, L"Control" },
        { RegisterType::Descriptor, L"Descriptor" },
    };

    for (auto& t : types)
        if ((t.value & type) == t.value)
            return t.text;

    return L"";
}

CString Helpers::CPUFlagsToString(uint32_t value) {
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
    CString text;
    for (auto& f : flags)
        if (value & (1 << f.bit))
            text += f.text + CString(L"=1 ");

    text.AppendFormat(L"IOPL=%u", (value >> 12) & 3);
    return text;
}

CString Helpers::CR0ToString(uint64_t value) {
    static const struct {
        int bit;
        PCWSTR text;
    } bits[] = {
        { 0, L"PE" },
        { 1, L"MP" },
        { 2, L"EM" },
        { 3, L"TS" },
        { 4, L"ET" },
        { 5, L"NE" },
        { 16, L"WP" },
        { 18, L"AM" },
        { 29, L"NW" },
        { 30, L"CD" },
        { 31, L"PG" },
    };

    CString text;
    for (auto& b : bits)
        if (value & (1LL << b.bit))
            text += b.text + CString(L"=1 ");
    return text;
}

std::vector<std::wstring> Helpers::GetNasmLocations() {
    std::vector<std::wstring> locations;
    WCHAR path[MAX_PATH];
    ::GetModuleFileName(nullptr, path, _countof(path));
    *wcsrchr(path, L'\\') = 0;
    locations.push_back(std::wstring(path) + L"\\nasm.exe");

    if (wil::unique_cotaskmem_string appData; SUCCEEDED(::SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appData)))
        locations.push_back(std::wstring(appData.get()) + L"\\QuickAsm\\nasm.exe");
    return locations;
}

std::wstring Helpers::ExtractNasm() {
    auto locations = GetNasmLocations();
    for (auto& path : locations)
        if (::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES)
            return path;

    auto hRes = ::FindResource(nullptr, MAKEINTRESOURCE(IDR_NASM), RT_RCDATA);
    auto hData = hRes ? ::LoadResource(nullptr, hRes) : nullptr;
    auto data = hData ? ::LockResource(hData) : nullptr;
    if (!data)
        return L"";
    auto size = ::SizeofResource(nullptr, hRes);

    for (auto& path : locations) {
        auto dir = path.substr(0, path.rfind(L'\\'));
        ::SHCreateDirectoryEx(nullptr, dir.c_str(), nullptr);
        // written whole to a temporary file first, so a failed write doesn't leave a broken nasm.exe
        auto temp = path + L".tmp";
        wil::unique_hfile hFile(::CreateFile(temp.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr));
        if (!hFile)
            continue;
        DWORD written;
        auto ok = ::WriteFile(hFile.get(), data, size, &written, nullptr) && written == size;
        hFile.reset();
        if (ok && ::MoveFileEx(temp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
            return path;
        ::DeleteFile(temp.c_str());
    }
    return L"";
}

bool Helpers::ParseNumber(CString text, uint64_t& value) {
	text.Trim();
	std::string s(CStringA(text).MakeLower().GetString());
	std::string_view digits(s);
	int base = 10;
	if (digits.starts_with("0x"))
		base = 16, digits.remove_prefix(2);
	else if (digits.ends_with('h'))
		base = 16, digits.remove_suffix(1);
	auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
	return !digits.empty() && ec == std::errc() && end == digits.data() + digits.size();
}
