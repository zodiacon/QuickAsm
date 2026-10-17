// sessions: the source, its file, the assembler, the mode, the address, the memory and the breakpoints, in one file
//

#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "MemoryDlg.h"
#include "Helpers.h"
#include <WTLHelper.h>
#include <fstream>
#include <sstream>

//
// the file: "key=value" lines under [Session] (a Breakpoint line for each: address,enabled,hit target,condition),
// then the source as it is, after [Source]
//

bool CMainFrame::IsSessionFile(PCWSTR path) {
	auto len = wcslen(path);
	auto ext = wcslen(SessionExtension);
	return len > ext && _wcsicmp(path + len - ext, SessionExtension) == 0;
}

bool CMainFrame::SaveSession(PCWSTR path) {
	std::ostringstream stm;
	CString addressText;
	m_AddressEdit.GetWindowText(addressText);
	stm << "; QuickAsm session\n[Session]\n";
	stm << "File=" << CW2A(m_FileName, CP_UTF8) << "\n";
	stm << "Assembler=" << m_AssemblerIndex << "\n";
	stm << "Mode=" << (int)m_AsmMode << "\n";
	stm << "Address=" << CW2A(addressText) << "\n";
	stm << std::format("MemoryBase=0x{:X}\nMemorySize={}\nStackSize={}\n", m_MemoryBase, m_Memory.size() / 1024, m_StackSize / 1024);
	{
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [bpAddress, bp] : m_Breakpoints)
			stm << std::format("Breakpoint=0x{:X},{},{},{}\n", bpAddress, bp.Enabled ? 1 : 0, bp.HitTarget, bp.Condition);
	}
	stm << "[Source]\n" << m_AsmSource.GetAllText();

	auto text = stm.str();
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (!file.write(text.data(), text.size())) {
		AtlMessageBox(m_hWnd, L"Failed to save the session.", IDR_MAINFRAME, MB_ICONERROR);
		return false;
	}
	m_SessionFile = path;
	AddRecentFile(path);
	return true;
}

bool CMainFrame::OpenSession(PCWSTR path) {
	auto bytes = Helpers::ReadFileContents(path);
	std::string content(bytes.begin(), bytes.end());
	auto sourceMark = content.find("[Source]");
	if (content.find("[Session]") == std::string::npos || sourceMark == std::string::npos) {
		AtlMessageBox(m_hWnd, L"Not a QuickAsm session file.", IDR_MAINFRAME, MB_ICONERROR);
		return false;
	}
	if (!SaveChangesIfNeeded())
		return false;

	// the source: after the [Source] line
	auto sourceStart = content.find('\n', sourceMark);
	auto source = sourceStart == std::string::npos ? std::string() : content.substr(sourceStart + 1);

	struct SavedBreakpoint {
		uint64_t Address;
		bool Enabled;
		uint32_t HitTarget;
		std::string Condition;
	};
	std::vector<SavedBreakpoint> breakpoints;
	std::string sourceFile, address;
	int assembler = m_AssemblerIndex, mode = (int)m_AsmMode;
	uint64_t memoryBase = m_MemoryBase;
	uint32_t memorySize = (uint32_t)(m_Memory.size() / 1024), stackSize = (uint32_t)(m_StackSize / 1024);

	std::istringstream lines(content.substr(0, sourceMark));
	for (std::string line; std::getline(lines, line); ) {
		if (!line.empty() && line.back() == '\r')
			line.pop_back();
		auto eq = line.find('=');
		if (eq == std::string::npos || line[0] == ';')
			continue;
		auto key = line.substr(0, eq), value = line.substr(eq + 1);
		uint64_t number = 0;
		Helpers::ParseNumber(CString(value.c_str()), number);
		if (key == "File")
			sourceFile = value;
		else if (key == "Assembler")
			assembler = (int)number;
		else if (key == "Mode")
			mode = (int)number;
		else if (key == "Address")
			address = value;
		else if (key == "MemoryBase")
			memoryBase = number;
		else if (key == "MemorySize")
			memorySize = (uint32_t)number;
		else if (key == "StackSize")
			stackSize = (uint32_t)number;
		else if (key == "Breakpoint") {
			// address,enabled,hit target,condition (which may have commas)
			std::istringstream fields(value);
			std::string bpAddress, enabled, hitTarget, condition;
			std::getline(fields, bpAddress, ',');
			std::getline(fields, enabled, ',');
			std::getline(fields, hitTarget, ',');
			std::getline(fields, condition);
			uint64_t a = 0, hits = 0;
			if (Helpers::ParseNumber(CString(bpAddress.c_str()), a)) {
				Helpers::ParseNumber(CString(hitTarget.c_str()), hits);
				breakpoints.push_back({ a, enabled != "0", (uint32_t)hits, condition });
			}
		}
	}

	// the setup: the memory first (the address must fit in it)
	StopEmulator();
	if (CMemoryDlg::Validate(memoryBase, memorySize, stackSize).IsEmpty()) {
		auto& settings = AppSettings::Get();
		settings.MemoryBase(memoryBase);
		settings.MemorySize((int)memorySize);
		settings.StackSize((int)stackSize);
		SetupMemory();
	}
	m_AssemblerIndex = std::clamp(assembler, 0, (int)m_Assemblers.size() - 1);
	m_AssemblerCombo.SetCurSel(m_AssemblerIndex);
	m_AsmMode = static_cast<AsmMode>(std::clamp(mode, 0, 2));
	if (!address.empty())
		m_AddressEdit.SetWindowText(CString(address.c_str()));
	UpdateUI();

	// the source; unchanged if it's its file's text
	m_AsmSource.SetText(source.c_str());
	m_AsmSource.EmptyUndoBuffer();
	m_FileName = CA2W(sourceFile.c_str(), CP_UTF8);
	if (!m_FileName.IsEmpty()) {
		auto fileBytes = Helpers::ReadFileContents(m_FileName);
		if (std::string(fileBytes.begin(), fileBytes.end()) == source)
			m_AsmSource.SetSavePoint();
	}
	else if (source.empty()) {
		m_AsmSource.SetSavePoint();
	}
	UpdateTitle();

	// the code, then its breakpoints (those at instructions)
	m_AsmBytes.clear();
	m_CodeSize = 0;
	m_Instructions.clear();
	if (!source.empty())
		Assemble();
	else
		InitEmulator(nullptr, 0);
	{
		std::lock_guard lock(m_BreakpointsLock);
		m_Breakpoints.clear();
		for (auto& saved : breakpoints) {
			if (LineFromAddress(saved.Address) < 0)
				continue;
			Breakpoint bp;
			bp.Enabled = saved.Enabled;
			bp.HitTarget = saved.HitTarget;
			if (!saved.Condition.empty()) {
				std::string error;
				if (auto expr = Expression::Parse(saved.Condition, error)) {
					bp.Condition = saved.Condition;
					bp.Expr = std::move(expr);
				}
			}
			m_Breakpoints[saved.Address] = std::move(bp);
		}
	}
	BreakpointsChanged();

	m_SessionFile = path;
	AddRecentFile(path);
	CString status;
	auto name = wcsrchr(path, L'\\');
	status.Format(L"Session: %s", name ? name + 1 : path);
	m_StatusBar.SetText(2, status);
	return true;
}

LRESULT CMainFrame::OnFileOpenSession(WORD, WORD, HWND, BOOL&) {
	CSimpleFileDialog dlg(TRUE, SessionExtension + 1, nullptr, OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING,
		L"QuickAsm sessions (*.qasession)\0*.qasession\0All files (*.*)\0*.*\0", m_hWnd);
	SuspendResumeHook sr;
	if (dlg.DoModal() != IDOK)
		return 0;

	OpenSession(dlg.m_szFileName);
	return 0;
}

LRESULT CMainFrame::OnFileSaveSession(WORD, WORD, HWND, BOOL&) {
	CSimpleFileDialog dlg(FALSE, SessionExtension + 1, m_SessionFile.IsEmpty() ? nullptr : (PCWSTR)m_SessionFile,
		OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING,
		L"QuickAsm sessions (*.qasession)\0*.qasession\0All files (*.*)\0*.*\0", m_hWnd);
	SuspendResumeHook sr;
	if (dlg.DoModal() != IDOK)
		return 0;

	SaveSession(dlg.m_szFileName);
	return 0;
}
