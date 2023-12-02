#include "pch.h"
#include "NasmAssembler.h"
#include "Helpers.h"
#include <fstream>

NasmAssembler::NasmAssembler() {
    SetName(L"NASM");
    //
    // try the exe directory
    //
    WCHAR path[MAX_PATH];
    ::GetModuleFileName(nullptr, path, _countof(path));
    *wcsrchr(path, L'\\') = 0;
    m_NasmDir = path;
    wcscat_s(path, L"\\nasm.exe");
    if (::GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES)
        m_NasmPath = path;
}

AssemblerResults NasmAssembler::Assemble(std::string const& source) {
    AssemblerResults results{};
    results.Error = true;
    if (source.empty()) {
        results.Output = "Error: no source";
        return results;
    }

    auto modeValue = GetIntValue("mode");
    std::string prefix;
    if (modeValue) {
        prefix = "bits " + std::to_string(*modeValue) + "\n";
    }

    auto address = GetIntValue("address");
    if(address) {
		prefix += "org " + std::to_string(*address) + "\n";
	}
    auto path = Helpers::GetTempFilePath(L"asm");
    std::ofstream stm(path);

    if (stm.good()) {
        if (!prefix.empty())
            stm.write(prefix.c_str(), prefix.length());
        stm.write(source.c_str(), source.length());
        stm.close();
        return AssembleFile(path);
    }
    return results;
}

AssemblerResults NasmAssembler::AssembleFile(std::wstring const& file) {
    AssemblerResults results{};
    if (m_NasmPath.empty()) {
        results.Error = ERROR_FILE_NOT_FOUND;
        return results;
    }

    PROCESS_INFORMATION pi;
    STARTUPINFO si{ sizeof(si) };
    auto tempOutput = Helpers::GetTempFilePath(L"log");
    std::wstring objOutput = file.data();
    objOutput = objOutput.substr(0, file.rfind(L'.')) + L".bin";

    auto fullcmdline = m_NasmPath + L" -Z" + tempOutput + L" -o " + objOutput + L" " + file;
    if (::CreateProcess(nullptr, fullcmdline.data(), nullptr, nullptr, TRUE, 
        CREATE_NO_WINDOW, nullptr, m_NasmDir.c_str(), &si, &pi)) {
        ::WaitForSingleObject(pi.hProcess, INFINITE);
        auto hFile = ::CreateFile(tempOutput.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hFile != INVALID_HANDLE_VALUE) {
            results.Output.resize(::GetFileSize(hFile, nullptr));
            if (!results.Output.empty()) {
                DWORD read;
                ::ReadFile(hFile, results.Output.data(), (DWORD)results.Output.length(), &read, nullptr);
                results.Error = 1;
            }
            ::CloseHandle(hFile);
        }
        if (results.Output.empty())
            results.OutputFile = objOutput;
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
    }

    return results;
}
