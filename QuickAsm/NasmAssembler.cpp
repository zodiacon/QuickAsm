#include "pch.h"
#include "NasmAssembler.h"
#include "Helpers.h"
#include <fstream>
#include <sstream>

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

	// the sections' places (where the code ends and data starts); nasm takes the map file's name as is, quotes included:
	// it runs in the map file's folder, which is named without its path
	auto mapFile = Helpers::GetTempFilePath(L"map");
	tempFiles.push_back(mapFile);
	auto tempDir = mapFile.substr(0, mapFile.rfind(L'\\'));
	prefix += "[map sections " + std::string(CW2A(mapFile.substr(tempDir.size() + 1).c_str())) + "]\n";
	extraLines++;

	auto srcFile = file;
	{
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
	auto listing = Helpers::GetTempFilePath(L"lst");
	tempFiles.push_back(listing);

	// (quoted: the paths may have spaces)
	auto fullcmdline = L"\"" + m_NasmPath + L"\" -Z \"" + tempOutput + L"\" -l \"" + listing + L"\" -o \"" + objOutput + L"\" \"" + srcFile + L"\"";
	PROCESS_INFORMATION pi{};
	STARTUPINFO si{ sizeof(si) };
	if (!::CreateProcess(nullptr, fullcmdline.data(), nullptr, nullptr, FALSE,
		CREATE_NO_WINDOW, nullptr, tempDir.c_str(), &si, &pi)) {
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
	results.SourceLines = ParseListing(listing, extraLines);
	results.CodeSize = GetCodeSize(mapFile, address ? *address : 0);
	return results;
}

size_t NasmAssembler::GetCodeSize(std::wstring const& mapFile, int64_t origin) {
	//
	// the summary's lines: vstart, start, stop, length (hex), class and name of each section
	//
	std::ifstream stm(mapFile);
	bool summary = false;
	int64_t codeStart = -1, codeLength = 0;
	bool others = false;
	for (std::string text; std::getline(stm, text); ) {
		if (text.starts_with("-- ")) {
			summary = text.find("Sections (summary)") != std::string::npos;
			continue;
		}
		if (!summary)
			continue;
		std::istringstream line(text);
		std::string vstart, start, stop, length, cls, name;
		if (!(line >> vstart >> start >> stop >> length >> cls >> name) || cls == "Class")
			continue;
		if (name == ".text") {
			codeStart = std::stoll(start, nullptr, 16);
			codeLength = std::stoll(length, nullptr, 16);
		}
		else if (cls == "progbits") {
			others = true;
		}
	}
	// (the code first, where the output starts)
	return others && codeStart == origin ? (size_t)codeLength : 0;
}

std::vector<std::pair<uint32_t, int>> NasmAssembler::ParseListing(std::wstring const& path, int extraLines) {
	//
	// the listing's lines: the source line number (6 wide), the offset (8 hex digits) of what it made, its bytes,
	// and the source, after "<n>" (at column 36) in a macro's expansion; there the line numbers are the macro's
	// definition: the code goes to the line that used the macro instead
	//
	std::vector<std::pair<uint32_t, int>> lines;
	std::ifstream stm(path);
	int usingLine = 0;
	uint32_t lastOffset = UINT32_MAX;
	// the offsets are of the section they're in: only the code's (.text, the first) are the code's
	bool code = true;
	for (std::string text; std::getline(stm, text); ) {
		if (text.size() < 15 || !std::isdigit((unsigned char)text[5]))
			continue;
		auto number = std::atoi(text.substr(0, 6).c_str());
		bool expansion = text.size() > 36 && text[36] == '<';
		if (!expansion)
			usingLine = number;

		// the source (from column 40): "section .data", "[section .text]"...
		if (text.size() > 40) {
			std::istringstream source(text.substr(40));
			std::string directive, name;
			source >> directive >> name;
			for (auto& ch : directive)
				ch = (char)std::tolower((unsigned char)ch);
			if (directive.starts_with('['))
				directive.erase(0, 1);
			if (directive == "section" || directive == "segment") {
				if (!name.empty() && name.back() == ']')
					name.pop_back();
				code = name == ".text";
				continue;
			}
		}
		if (!code)
			continue;

		auto offset = text.substr(7, 8);
		if (text[6] != ' ' || !std::ranges::all_of(offset, [](char ch) { return std::isxdigit((unsigned char)ch) != 0; }))
			continue;
		auto value = (uint32_t)std::stoul(offset, nullptr, 16);
		// (a line's bytes may go on in the next line, at another offset: the first is the code's)
		if (value == lastOffset)
			continue;
		lastOffset = value;
		auto line = (expansion ? usingLine : number) - extraLines - 1;
		if (line >= 0)
			lines.emplace_back(value, line);
	}
	return lines;
}
