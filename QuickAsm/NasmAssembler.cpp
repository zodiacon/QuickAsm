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

	// the files made here go when done (GetTempFileName creates its files, and has a limited number of names)
	std::vector<std::wstring> tempFiles;
	auto cleanup = wil::scope_exit([&] {
		for (auto& path : tempFiles)
			::DeleteFile(path.c_str());
		});

	auto srcFile = file;
	if (!prefix.empty()) {
		auto source = Helpers::ReadFileContents(file.c_str());
		auto tempSrc = Helpers::GetTempFilePath(L"src");
		tempFiles.push_back(tempSrc);
		std::ofstream stm(tempSrc, std::ios::binary | std::ios::trunc);
		stm.write(prefix.data(), prefix.size());
		stm.write((const char*)source.data(), source.size());
		if (stm.good())
			srcFile = tempSrc;
	}

	auto tempOutput = Helpers::GetTempFilePath(L"log");
	tempFiles.push_back(tempOutput);
	auto objOutput = file.substr(0, file.rfind(L'.')) + L".bin";
	tempFiles.push_back(objOutput);

	// (quoted: the paths may have spaces)
	auto fullcmdline = L"\"" + m_NasmPath + L"\" -Z \"" + tempOutput + L"\" -o \"" + objOutput + L"\" \"" + srcFile + L"\"";
	PROCESS_INFORMATION pi{};
	STARTUPINFO si{ sizeof(si) };
	if (!::CreateProcess(nullptr, fullcmdline.data(), nullptr, nullptr, FALSE,
		CREATE_NO_WINDOW, nullptr, m_NasmDir.c_str(), &si, &pi)) {
		results.Error = ::GetLastError();
		results.Output = "Failed to run nasm.exe";
		return results;
	}
	wil::unique_handle hProcess(pi.hProcess), hThread(pi.hThread);
	::WaitForSingleObject(hProcess.get(), INFINITE);
	DWORD exitCode = 1;
	::GetExitCodeProcess(hProcess.get(), &exitCode);

	// the messages are "file:line: text" (the file may start with a drive letter); others are kept as they are
	std::ifstream stm(tempOutput);
	for (std::string line; std::getline(stm, line); ) {
		if (line.empty())
			continue;
		auto colon = line.find(':', line.size() > 2 && line[1] == ':' ? 2 : 0);
		auto colon2 = colon == std::string::npos ? colon : line.find(':', colon + 1);
		auto number = colon2 == std::string::npos ? std::string() : line.substr(colon + 1, colon2 - colon - 1);
		if (!number.empty() && std::ranges::all_of(number, [](char ch) { return ch >= '0' && ch <= '9'; })) {
			// (the line numbers of the source, without the lines added to it)
			results.Output += std::to_string(std::stoi(number) - extraLines) + ":" + line.substr(colon2 + 1);
		}
		else {
			results.Output += line;
		}
		results.Output += '\n';
	}

	// warnings only (nasm succeeded): the output stays, as warnings
	if (exitCode != 0) {
		results.Error = 1;
		if (results.Output.empty())
			results.Output = "nasm.exe failed (exit code " + std::to_string(exitCode) + ")";
		return results;
	}
	results.Bytes = Helpers::ReadFileContents(objOutput.c_str());
	return results;
}
