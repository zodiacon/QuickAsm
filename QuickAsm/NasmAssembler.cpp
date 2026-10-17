#include "pch.h"
#include "NasmAssembler.h"
#include "Helpers.h"
#include <fstream>

NasmAssembler::NasmAssembler() {
	SetName(L"NASM");
	// (extracted there on startup, if it wasn't already)
	for (auto& path : Helpers::GetNasmLocations()) {
		if (::GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
			m_NasmPath = path;
			m_NasmDir = path.substr(0, path.rfind(L'\\'));
			break;
		}
	}
}

AssemblerResults NasmAssembler::AssembleFile(std::wstring const& file) {
	AssemblerResults results{};
	if (m_NasmPath.empty()) {
		results.Error = ERROR_FILE_NOT_FOUND;
		results.Output = "nasm.exe not found";
		return results;
	}

	auto modeValue = GetIntValue("mode");
	std::string prefix;
	int extraLines = 0;
	if (modeValue) {
		prefix = "bits " + std::to_string(*modeValue) + "\n";
		extraLines++;
	}

	auto address = GetIntValue("address");
	if (address) {
		prefix += "org " + std::to_string(*address) + "\n";
		extraLines++;
	}

	auto srcFile = file;
	if (!prefix.empty()) {
		auto hFile = ::CreateFile(file.c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hFile != INVALID_HANDLE_VALUE) {
			auto size = ::GetFileSize(hFile, nullptr);
			std::vector<BYTE> buffer(size);
			DWORD read;
			::ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &read, nullptr);
			auto tempSrc = Helpers::GetTempFilePath(L"src");
			auto hSrc = ::CreateFile(tempSrc.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
			if (hSrc != INVALID_HANDLE_VALUE) {
				::WriteFile(hSrc, prefix.data(), (DWORD)prefix.length(), &read, nullptr);
				::WriteFile(hSrc, buffer.data(), (DWORD)buffer.size(), &read, nullptr);
				::CloseHandle(hSrc);
				srcFile = tempSrc;
			}
			::CloseHandle(hFile);
		}
	}

	PROCESS_INFORMATION pi{};
	STARTUPINFO si{ sizeof(si) };
	auto tempOutput = Helpers::GetTempFilePath(L"log");
	std::wstring objOutput = file.data();
	objOutput = objOutput.substr(0, file.rfind(L'.')) + L".bin";

	// (quoted: the paths may have spaces)
	auto fullcmdline = L"\"" + m_NasmPath + L"\" -Z \"" + tempOutput + L"\" -o \"" + objOutput + L"\" \"" + srcFile + L"\"";
	if (!::CreateProcess(nullptr, fullcmdline.data(), nullptr, nullptr, FALSE,
		CREATE_NO_WINDOW, nullptr, m_NasmDir.c_str(), &si, &pi)) {
		results.Error = ::GetLastError();
		results.Output = "Failed to run nasm.exe";
		return results;
	}
	{
		::WaitForSingleObject(pi.hProcess, INFINITE);
		std::ifstream stm(tempOutput);
		if (stm.good()) {
			char line[1024];
			while(!stm.eof()) {
				stm.getline(line, sizeof(line));
				auto colon = strchr(line + 2, ':');
				if (colon == nullptr)
					break;
				if (extraLines) {
					//
					// subtract line number to align with actual assembly text
					//
					auto colon2 = strchr(colon + 1, ':');
					auto lineNum = atoi(colon + 1);
					lineNum -= extraLines;
					results.Output += std::to_string(lineNum) + ":" + (colon2 + 1);
				}
				else {
					results.Output += colon + 1;
				}
				results.Output += '\n';
			}
		}
	}
	if (results.Output.empty())
		results.OutputFile = objOutput;
	else
		results.Error = 1;

	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	return results;
}
