#include "pch.h"
#include "KeystoneAssembler.h"
#include <wil\resource.h>

KeystoneAssembler::KeystoneAssembler() {
    SetName(L"Keystone");
}

AssemblerResults KeystoneAssembler::AssembleFile(std::wstring const& file) {
    AssemblerResults results{};
    wil::unique_hfile hFile(::CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
    if (!hFile) {
        results.Error = ::GetLastError();
        return results;
    }

    std::string text;
    text.resize(::GetFileSize(hFile.get(), nullptr));
    if (DWORD read; !::ReadFile(hFile.get(), text.data(), (DWORD)text.length(), &read, nullptr)) {
        results.Error = ::GetLastError();
        return results;
    }
    hFile.reset();

    results = Assemble(text);
    if (results.Bytes.empty())
        return results;

    std::wstring path(file.data());
    auto index = path.rfind(L'.');
    assert(index != std::string::npos);
    path = path.substr(0, index) + L".bin";
    hFile.reset(::CreateFile(path.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, 0, nullptr));
    if (!hFile) {
        results.Error = ::GetLastError();
        return results;
    }
    if (DWORD written; !::WriteFile(hFile.get(), results.Bytes.data(), (DWORD)results.Bytes.size(), &written, nullptr)) {
        results.Error = ::GetLastError();
        return results;
    }
    ::SetEndOfFile(hFile.get());
    results.OutputFile = path;
    return results;
}

AssemblerResults KeystoneAssembler::Assemble(std::string const& source) {
    AssemblerResults results{};
    if (source.empty()) {
        results.Error = 1;
        results.Output = "No source code";
        return results;
    }

    auto mode = GetIntValue("mode");
    if (mode) {
        switch (*mode) {
            case 16: m_Mode = KS_MODE_16; break;
            case 32: m_Mode = KS_MODE_32; break;
            case 64: m_Mode = KS_MODE_64; break;
        }
    }
    auto address = GetIntValue("address");
    if (address)
        m_Address = *address;

    ks_engine* ks;
    results.Error = ks_open(KS_ARCH_X86, m_Mode, &ks);
    if (results.Error != KS_ERR_OK)
        return results;

    uint8_t* encoding;
    size_t size;
    if (ks_asm(ks, source.c_str(), m_Address, &encoding, &size, &results.Instructions)) {
        results.Error = ks_errno(ks);
        results.Output = ks_strerror(ks_errno(ks));
    }
    else {
        results.Bytes.resize(size);
        memcpy(results.Bytes.data(), encoding, size);
        ks_free(encoding);
    }
    ks_close(ks);

    return results;
}

void KeystoneAssembler::SetMode(ks_mode mode) {
    m_Mode = mode;
}

ks_mode KeystoneAssembler::GetMode() const {
    return m_Mode;
}

void KeystoneAssembler::SetAddress(uint64_t address) {
    m_Address = address;
}

uint64_t KeystoneAssembler::GetAddress() const {
    return m_Address;
}
