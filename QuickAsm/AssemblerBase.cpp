#include "pch.h"
#include "AssemblerBase.h"
#include <fstream>
#include "Helpers.h"

void AssemblerBase::SetName(PCWSTR name) {
    m_Name = name;
}

PCWSTR AssemblerBase::GetName() const {
    return m_Name.c_str();
}

AssemblerResults AssemblerBase::Assemble(std::string const& source) {
    AssemblerResults results{};
    results.Error = true;
    if (source.empty()) {
        results.Output = "Error: no source";
        return results;
    }

    auto path = Helpers::GetTempFilePath(L"asm");
    std::ofstream stm(path);

    if (stm.good()) {
        stm.write(source.c_str(), source.length());
        stm.close();
        return AssembleFile(path);
    }
    return results;
}

void AssemblerBase::SetValue(std::string name, OptionValue value) {
    m_Options[name] = std::move(value);
}

OptionValue const* AssemblerBase::GetValue(std::string_view name) const {
    if (auto it = m_Options.find(name.data()); it != m_Options.end())
        return &it->second;
    return nullptr;
}

int64_t const* AssemblerBase::GetIntValue(std::string_view name) const {
    auto value = GetValue(name);
    return value ? &std::get<0>(*value) : nullptr;
}

std::string const* AssemblerBase::GetStringValue(std::string_view name) const {
    auto value = GetValue(name);
    return value ? &std::get<1>(*value) : nullptr;
}

