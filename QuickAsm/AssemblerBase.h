#pragma once

#include <variant>

struct AssemblerResults {
	std::vector<uint8_t> Bytes;
	std::wstring OutputFile;
	std::string Output;
	size_t Instructions;
	int Error{ 0 };
};

using OptionValue = std::variant<int64_t, std::string>;

class AssemblerBase abstract {
public:
	void SetName(PCWSTR name);
	PCWSTR GetName() const;
	virtual AssemblerResults Assemble(std::string const& source);
	virtual AssemblerResults AssembleFile(std::wstring const& file) = 0;
	void SetValue(std::string name, OptionValue value);
	OptionValue const* GetValue(std::string_view name) const;
	int64_t const* GetIntValue(std::string_view name) const;
	std::string const* GetStringValue(std::string_view name) const;

private:
	std::unordered_map<std::string, OptionValue> m_Options;
	std::wstring m_Name;
};

