#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "Interfaces.h"
#include "Helpers.h"
#include "KeystoneAssembler.h"
#include "NasmAssembler.h"
#include "AboutDlg.h"
#include "Templates.h"
#include "MemoryDlg.h"
#include <WTLHelper.h>
#include <ToolbarHelper.h>
#include <fstream>

CMainFrame::CMainFrame() {
	SetupMemory();
	m_Assemblers.push_back(std::make_unique<KeystoneAssembler>());
	m_Assemblers.push_back(std::make_unique<NasmAssembler>());
}

CMainFrame::~CMainFrame() = default;

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (m_pFindDlg && m_pFindDlg->IsDialogMessage(pMsg))
		return TRUE;
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	if (m_MemoryView.PreTranslateMessage(pMsg))
		return TRUE;

	return m_Tabs.PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	auto anyText = m_AsmSource.GetTextLength() > 0;
	UIEnable(ID_FILE_SAVE, m_AsmSource.IsModified() || (anyText && m_FileName.IsEmpty()));
	UIEnable(ID_FILE_SAVE_AS, anyText);
	auto busy = IsEmulatorBusy();
	auto running = m_EmulatorState == EmulatorState::Running;
	auto haveCode = !m_Instructions.empty() && m_Emulator.IsOpen();
	UIEnable(ID_ASM_ASSEMBLE, anyText && !busy);
	UIEnable(ID_ASM_RUN, haveCode && !running);
	UIEnable(ID_ASM_BREAK, running);
	UIEnable(ID_ASM_STOP, busy);
	UIEnable(ID_ASM_RESTART, haveCode);
	UIEnable(ID_ASM_STEPINTO, haveCode && !running);
	UIEnable(ID_ASM_STEPOVER, haveCode && !running);
	UIEnable(ID_ASM_STEPOUT, m_EmulatorState == EmulatorState::Paused);
	UIEnable(ID_ASM_RUNTOCURSOR, haveCode && !running);
	UIEnable(ID_ASM_TOGGLEBP, haveCode);
	UIEnable(ID_EDIT_FINDNEXT, !m_SearchText.IsEmpty());
	UIEnable(ID_EDIT_FINDPREV, !m_SearchText.IsEmpty());
	UIEnable(ID_ASM_ENABLEBP, haveCode);
	UIEnable(ID_ASM_DELETEALLBP, haveCode);
	UIEnable(ID_ASM_BPCONDITION, haveCode);
	for (UINT id = ID_DISASM_INTEL; id <= ID_DISASM_UPPERCASE; id++)
		UIEnable(id, !running);
	// the code's setup stays as it is while it runs
	for (UINT id = ID_MODE_16BIT; id <= ID_MODE_64BIT; id++)
		UIEnable(id, !busy);
	for (UINT id = ID_ASSEMBLER_KEYSTONE; id <= ID_ASSEMBLER_NASM; id++)
		UIEnable(id, !busy);
	m_AssemblerCombo.EnableWindow(!busy);
	m_AddressEdit.EnableWindow(!busy);

	if (auto& hex = m_MemoryView.GetHexControl(); ::GetFocus() == hex) {
		UIEnable(ID_EDIT_UNDO, hex.CanUndo());
		UIEnable(ID_EDIT_REDO, hex.CanRedo());
		UIEnable(ID_EDIT_CUT, hex.CanCut());
		UIEnable(ID_EDIT_COPY, hex.CanCopy());
		UIEnable(ID_EDIT_PASTE, hex.CanPaste());
		UIEnable(ID_EDIT_CLEAR, hex.CanDelete());
	}
	else {
		auto editor = GetFocusedEditor();
		auto writable = editor && !editor->IsReadOnly();
		UIEnable(ID_EDIT_UNDO, writable && editor->CanUndo());
		UIEnable(ID_EDIT_REDO, writable && editor->CanRedo());
		UIEnable(ID_EDIT_CUT, writable && !editor->IsSelectionEmpty());
		UIEnable(ID_EDIT_COPY, editor && !editor->IsSelectionEmpty());
		UIEnable(ID_EDIT_PASTE, writable && editor->CanPaste());
		UIEnable(ID_EDIT_CLEAR, writable && !editor->IsSelectionEmpty());
	}

	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::InitMenu(HMENU hMenu) {
	MenuItemData commands[] {
		{ ID_FILE_NEW, IDI_FILENEW },
		{ ID_FILE_OPEN, IDI_OPEN },
		{ ID_FILE_SAVE, IDI_SAVE },
		{ ID_FILE_SAVE_AS, IDI_SAVEAS },
		{ ID_EDIT_UNDO, IDI_UNDO },
		{ ID_EDIT_REDO, IDI_REDO },
		{ ID_EDIT_CUT, IDI_CUT },
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_EDIT_PASTE, IDI_PASTE },
		{ ID_ASM_ASSEMBLE, IDI_BUILD },
		{ ID_MODE_16BIT, IDI_16BIT },
		{ ID_MODE_32BIT, IDI_32BIT },
		{ ID_MODE_64BIT, IDI_64BIT },
		{ ID_ASM_RUN, IDI_RUN },
		{ ID_ASM_BREAK, IDI_BREAK },
		{ ID_ASM_STOP, IDI_STOP },
		{ ID_ASM_RESTART, IDI_RESTART },
		{ ID_ASM_STEPOVER, IDI_STEPOVER },
		{ ID_ASM_STEPINTO, IDI_STEPINTO },
		{ ID_ASM_STEPOUT, IDI_STEPOUT },
		{ ID_ASM_TOGGLEBP, IDI_BREAKPOINT },
		{ ID_APP_ABOUT, IDR_MAINFRAME },
	};
	WTLHelper::InitMenu(hMenu, commands, _countof(commands));
}

void CMainFrame::ApplyTheme() {
	auto& settings = AppSettings::Get();
	// (already in that mode, as at startup: the windows are themed and redrawn all the same)
	if (!WTLHelper::SwitchToMode(settings.EffectiveTheme(), m_hWnd))
		WTLHelper::ApplyMode(m_hWnd);
	// the menu item bitmaps are drawn on the menu's background
	InitMenu(GetMenu());
	DrawMenuBar();
	m_AsmSource.ApplyTheme();
	m_Disassembly.ApplyTheme();
	m_MemoryView.ApplyTheme();
	SendMessageToDescendants(WM_UPDATE_DARKMODE);
}

void CMainFrame::UpdateUI() {
	auto& settings = AppSettings::Get();
	UISetRadioMenuItem(ID_THEME_LIGHT + settings.Theme(), ID_THEME_LIGHT, ID_THEME_SYSTEM);
	UISetRadioMenuItem(ID_MODE_16BIT + (int)m_AsmMode, ID_MODE_16BIT, ID_MODE_64BIT);
	// the radio state is a press on a toolbar, which doesn't last: the toolbar buttons are checked
	for (int i = 0; i < 3; i++)
		UISetCheck(ID_MODE_16BIT + i, i == (int)m_AsmMode);
	UISetRadioMenuItem(ID_ASSEMBLER_KEYSTONE + m_AssemblerIndex, ID_ASSEMBLER_KEYSTONE, ID_ASSEMBLER_NASM);
	UISetCheck(ID_VIEW_STATUS_BAR, IsStatusBarVisible());
	UISetCheck(ID_OPTIONS_AUTOCOMPLETE, settings.AutoComplete());
	UISetRadioMenuItem(ID_DISASM_INTEL + std::clamp(settings.DisasmSyntax(), 0, 1), ID_DISASM_INTEL, ID_DISASM_ATT);
	UISetCheck(ID_DISASM_BYTES, settings.DisasmBytes());
	UISetCheck(ID_DISASM_UPPERCASE, settings.DisasmUpperCase());
}

void CMainFrame::UpdateTitle() {
	CString title;
	title.LoadString(IDR_MAINFRAME);
	if (!m_FileName.IsEmpty())
		title += L" - " + m_FileName + (m_AsmSource.IsModified() ? L"*" : L"");
	SetWindowText(title);
}

bool CMainFrame::IsStatusBarVisible() const {
	// (not IsWindowVisible: false while the frame is hidden, as when it's created or destroyed)
	return (::GetWindowLongPtr(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE) != 0;
}

CAsmEditor* CMainFrame::GetFocusedEditor() {
	auto hFocus = ::GetFocus();
	if (hFocus == m_AsmSource)
		return &m_AsmSource;
	if (hFocus == m_Disassembly)
		return &m_Disassembly;
	return nullptr;
}

bool CMainFrame::SaveChangesIfNeeded() {
	if (!m_AsmSource.IsModified())
		return true;

	CString text;
	text.Format(L"Save changes to %s?", m_FileName.IsEmpty() ? L"the source" : (PCWSTR)m_FileName);
	switch (AtlMessageBox(m_hWnd, (PCWSTR)text, IDR_MAINFRAME, MB_YESNOCANCEL | MB_ICONQUESTION)) {
		case IDYES:
			return m_FileName.IsEmpty() ? DoSaveAs() : SaveSource(m_FileName);
		case IDNO:
			return true;
	}
	return false;
}

bool CMainFrame::SaveSource(PCWSTR path) {
	std::ofstream stm(path, std::ios::binary | std::ios::trunc);
	auto text = m_AsmSource.GetAllText();
	if (!stm.write(text.data(), text.size())) {
		AtlMessageBox(m_hWnd, L"Failed to save the file.", IDR_MAINFRAME, MB_ICONERROR);
		return false;
	}
	m_FileName = path;
	m_AsmSource.SetSavePoint();
	UpdateTitle();
	AddRecentFile(path);
	return true;
}

void CMainFrame::AddRecentFile(PCWSTR path) {
	m_RecentFiles.AddFile(path);
	AppSettings::Get().RecentFiles(m_RecentFiles.Files());
	UpdateRecentFilesMenu();
}

void CMainFrame::UpdateRecentFilesMenu() {
	// the File menu's submenu that has the files (or the empty item): not New from Template's
	auto fileMenu = CMenuHandle(GetMenu()).GetSubMenu(0);
	CMenuHandle menu;
	for (int i = 0; i < fileMenu.GetMenuItemCount() && !menu; i++) {
		CMenuHandle sub = fileMenu.GetSubMenu(i);
		if (!sub)
			continue;
		auto id = sub.GetMenuItemID(0);
		if (id == ID_RECENTFILES_EMPTY || (id >= ATL_IDS_MRU_FILE && id < ATL_IDS_MRU_FILE + 16))
			menu = sub;
	}
	ATLASSERT(menu);

	while (menu.DeleteMenu(0, MF_BYPOSITION))
		;
	if (m_RecentFiles.IsEmpty()) {
		menu.AppendMenu(MF_STRING | MF_GRAYED, ID_RECENTFILES_EMPTY, L"(Empty)");
		return;
	}
	int i = 0;
	for (auto& file : m_RecentFiles.Files())
		menu.AppendMenu(MF_STRING, ATL_IDS_MRU_FILE + i++, file.c_str());
}

bool CMainFrame::DoSaveAs() {
	CSimpleFileDialog dlg(FALSE, L"asm", m_FileName.IsEmpty() ? nullptr : (PCWSTR)m_FileName, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING,
		L"Assembly files (*.asm)\0*.asm\0All files (*.*)\0*.*\0", m_hWnd);
	SuspendResumeHook sr;
	if (dlg.DoModal() != IDOK)
		return false;
	return SaveSource(dlg.m_szFileName);
}

bool CMainFrame::ParseAddress() {
	CString text;
	m_AddressEdit.GetWindowText(text);
	text.Trim();
	uint64_t address;
	if (!Helpers::ParseNumber(text, address) || !FitsInMemory(address, 1)) {
		CString message;
		message.Format(L"The address must be a number from 0x%llX to below 0x%llX, under the stack (decimal, or hex with 0x).",
			m_MemoryBase, MemoryEnd() - m_StackSize);
		AtlMessageBox(m_hWnd, (PCWSTR)message, IDR_MAINFRAME, MB_ICONWARNING);
		m_AddressEdit.SetFocus();
		m_AddressEdit.SetSelAll();
		return false;
	}
	m_Address = address;
	return true;
}

void CMainFrame::Assemble() {
	if (!ParseAddress())
		return;

	auto assembler = m_Assemblers[m_AssemblerIndex].get();
	assembler->SetValue("mode", int64_t(1) << (4 + (int)m_AsmMode));
	assembler->SetValue("address", (int64_t)m_Address);

	auto results = assembler->Assemble(m_AsmSource.GetAllText());
	ShowWarnings(results.Error ? std::string() : results.Output);
	m_AsmSource.MarkErrorLines(results.Error ? GetErrorLines(results.Output) : std::vector<int>());
	m_AssembledSource.clear();
	m_AddressToSourceLine.clear();
	if (results.Error) {
		m_AsmBytes.clear();
		m_Instructions.clear();
		m_Disassembly.SetReadOnly(false);
		m_Disassembly.SetText(results.Output.c_str());
		m_Disassembly.SetReadOnly(true);
		InitEmulator(nullptr, 0);
		m_StatusBar.SetText(0, L"Assembly failed");
		return;
	}

	if (results.Bytes.empty())
		m_AsmBytes = Helpers::ReadFileContents(results.OutputFile.c_str());
	else
		m_AsmBytes = std::move(results.Bytes);
	if (!FitsInMemory(m_Address, m_AsmBytes.size())) {
		AtlMessageBox(m_hWnd, L"The code doesn't fit in the emulator's memory (below the stack) at this address.", IDR_MAINFRAME, MB_ICONWARNING);
		m_AsmBytes.clear();
	}
	Disassemble(m_AsmBytes.data(), m_AsmBytes.size());
	m_AssembledSource = m_AsmSource.GetAllText();
	MapSourceLines(results.SourceLines);
	BreakpointsChanged();
}

void CMainFrame::MapSourceLines(std::vector<std::pair<uint32_t, int>> const& sourceLines) {
	m_AddressToSourceLine.clear();
	if (m_Instructions.empty())
		return;

	if (!sourceLines.empty()) {
		for (auto& [offset, line] : sourceLines)
			m_AddressToSourceLine.try_emplace(m_Address + offset, line);
		return;
	}

	// the lines with an instruction: not empty after the comment and any labels, not a directive
	std::vector<int> lines;
	auto& text = m_AssembledSource;
	int number = 0;
	for (size_t start = 0, end; start < text.size(); start = end + 1, number++) {
		end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		auto line = text.substr(start, end - start);
		if (auto semicolon = line.find(';'); semicolon != std::string::npos)
			line.resize(semicolon);
		auto trim = [](std::string& s) {
			auto first = s.find_first_not_of(" \t\r");
			s = first == std::string::npos ? std::string() : s.substr(first, s.find_last_not_of(" \t\r") - first + 1);
			};
		trim(line);
		// labels ("name:")
		for (;;) {
			auto colon = line.find(':');
			if (colon == std::string::npos || colon == 0 || !std::all_of(line.begin(), line.begin() + colon,
				[](char ch) { return std::isalnum((unsigned char)ch) || ch == '_' || ch == '.' || ch == '@' || ch == '$'; }))
				break;
			line = line.substr(colon + 1);
			trim(line);
		}
		if (!line.empty() && line[0] != '.')
			lines.push_back(number);
	}
	// (not one for each instruction: data, or directives that make code; the lines would be wrong)
	if (lines.size() != m_Instructions.size())
		return;
	for (size_t i = 0; i < lines.size(); i++)
		m_AddressToSourceLine[m_Instructions[i].Address] = lines[i];
}

int CMainFrame::SourceLineFromDisassemblyLine(int line) const {
	if (line < 0 || line >= (int)m_Instructions.size() || m_AddressToSourceLine.empty())
		return -1;
	auto it = m_AddressToSourceLine.find(m_Instructions[line].Address);
	if (it == m_AddressToSourceLine.end())
		return -1;
	// the lines are of the text that was assembled
	if (m_AsmSource.GetAllText() != m_AssembledSource)
		return -1;
	return it->second;
}

std::vector<int> CMainFrame::GetErrorLines(std::string const& errors) {
	// the lines starting with "<line>:" (0 based)
	std::vector<int> lines;
	for (size_t start = 0, end; start < errors.size(); start = end + 1) {
		end = errors.find('\n', start);
		if (end == std::string::npos)
			end = errors.size();
		if (auto line = ParseErrorLine(errors.substr(start, end - start)); line >= 0)
			lines.push_back(line);
	}
	return lines;
}

int CMainFrame::ParseErrorLine(std::string const& text) {
	size_t digits = 0;
	while (digits < text.size() && text[digits] >= '0' && text[digits] <= '9')
		digits++;
	if (digits == 0 || digits > 7 || digits >= text.size() || text[digits] != ':')
		return -1;
	return std::stoi(text.substr(0, digits)) - 1;
}

void CMainFrame::ShowWarnings(std::string const& warnings) {
	// the first one, and how many more (in the status bar: the disassembly's lines are the instructions)
	CString text;
	int count = 0;
	std::string first;
	for (size_t start = 0, end; start < warnings.size(); start = end + 1) {
		end = warnings.find('\n', start);
		if (end == std::string::npos)
			end = warnings.size();
		if (end > start && count++ == 0)
			first = warnings.substr(start, end - start);
	}
	if (count) {
		// "<line>: warning: <text>" shown as "Warning, line <line>: <text>"
		auto colon = first.find(": warning: ");
		if (colon != std::string::npos && colon > 0 && std::ranges::all_of(first.substr(0, colon), [](char ch) { return ch >= '0' && ch <= '9'; }))
			text = L"Warning, line " + CString(first.substr(0, colon).c_str()) + L": " + CString(first.substr(colon + 11).c_str());
		else
			text = L"Warning: " + CString(first.c_str());
		if (count > 1)
			text.AppendFormat(L" (+%d more)", count - 1);
	}
	m_StatusBar.SetText(2, text);
}

void CMainFrame::Disassemble(uint8_t const* data, size_t size) {
	DisassembleText(data, size);
	InitEmulator(data, size);

	CString status;
	status.Format(L"%zu instructions, %zu bytes", m_Instructions.size(), size);
	m_StatusBar.SetText(0, status);
}

void CMainFrame::DisassembleText(uint8_t const* data, size_t size) {
	auto& settings = AppSettings::Get();
	CapstoneEngine cs;
	if (!cs.Open((cs_mode)(1 << (1 + (int)m_AsmMode))))
		return;
	cs.SetSyntax(settings.DisasmSyntax() == 1);

	m_Instructions = cs.Disassemble(data, size, m_Address);
	auto bytes = settings.DisasmBytes();
	CStringA text;
	for (auto& insn : m_Instructions) {
		auto mnemonicSize = insn.Mnemonic.size();
		auto line = FormatInstruction(insn);
		// (the mnemonic and the operands in columns)
		text.AppendFormat("0x%08X %-8s %-28s", (DWORD)insn.Address, line.substr(0, mnemonicSize).c_str(),
			line.size() > mnemonicSize ? line.substr(mnemonicSize + 1).c_str() : "");
		if (bytes) {
			text += " ;";
			for (auto b : insn.Bytes)
				text.AppendFormat(" %02X", b);
		}
		text.TrimRight();
		text += "\n";
	}
	m_Disassembly.SetReadOnly(false);
	m_Disassembly.SetText(text);
	m_Disassembly.SetReadOnly(true);
}

std::string CMainFrame::FormatInstruction(Instruction const& insn) const {
	auto text = insn.Operands.empty() ? insn.Mnemonic : insn.Mnemonic + " " + insn.Operands;
	if (AppSettings::Get().DisasmUpperCase()) {
		std::ranges::transform(text, text.begin(), [](char ch) { return (char)::toupper((unsigned char)ch); });
		// (the numbers' prefix stays as it was: 0X1000 is harder to read)
		for (size_t pos = 0; (pos = text.find("0X", pos)) != std::string::npos; pos += 2)
			text[pos + 1] = 'x';
	}
	return text;
}

LRESULT CMainFrame::OnDisassemblyOption(WORD, WORD wID, HWND, BOOL&) {
	auto& settings = AppSettings::Get();
	switch (wID) {
		case ID_DISASM_INTEL: settings.DisasmSyntax(0); break;
		case ID_DISASM_ATT: settings.DisasmSyntax(1); break;
		case ID_DISASM_BYTES: settings.DisasmBytes(!settings.DisasmBytes()); break;
		case ID_DISASM_UPPERCASE: settings.DisasmUpperCase(!settings.DisasmUpperCase()); break;
	}
	UpdateUI();
	// (not while the code runs: the emulator thread reads the instructions; the menu items are disabled then)
	if (!m_AsmBytes.empty() && m_EmulatorState != EmulatorState::Running) {
		DisassembleText(m_AsmBytes.data(), m_AsmBytes.size());
		// (the text is new: its markers too)
		BreakpointsChanged();
		ShowCurrentLine(m_CurrentLine);
		m_BreakpointsView.Refresh();
	}
	return 0;
}

void CMainFrame::SetupMemory() {
	auto& settings = AppSettings::Get();
	uint64_t base = settings.MemoryBase();
	uint32_t size = settings.MemorySize(), stack = settings.StackSize();
	if (!CMemoryDlg::Validate(base, size, stack).IsEmpty())
		base = 0, size = 64, stack = 4;
	m_MemoryBase = base;
	m_StackSize = stack * 1024ull;
	// (zeroed: set up again by the emulator anyway)
	m_Memory.assign(size * 1024ull, 0);
}

uint64_t CMainFrame::MemoryEnd() const {
	return m_MemoryBase + m_Memory.size();
}

bool CMainFrame::FitsInMemory(uint64_t address, size_t size) const {
	return address >= m_MemoryBase && address + size <= MemoryEnd() - m_StackSize;
}

uint8_t const* CMainFrame::MemoryAt(uint64_t address, size_t size) const {
	if (address < m_MemoryBase || address - m_MemoryBase > m_Memory.size() || m_Memory.size() - (address - m_MemoryBase) < size)
		return nullptr;
	return m_Memory.data() + (address - m_MemoryBase);
}

LRESULT CMainFrame::OnOptionsMemory(WORD, WORD, HWND, BOOL&) {
	auto& settings = AppSettings::Get();
	CMemoryDlg dlg;
	dlg.Base = m_MemoryBase;
	dlg.SizeKB = (uint32_t)(m_Memory.size() / 1024);
	dlg.StackKB = (uint32_t)(m_StackSize / 1024);
	if (dlg.DoModal(m_hWnd) != IDOK)
		return 0;

	// (the memory is reallocated: the emulator, which maps it, is set up again)
	StopEmulator();
	settings.MemoryBase(dlg.Base);
	settings.MemorySize((int)dlg.SizeKB);
	settings.StackSize((int)dlg.StackKB);
	SetupMemory();

	if (!FitsInMemory(m_Address, 1)) {
		// an address in the new memory, for the next assembly
		m_Address = m_MemoryBase + std::min<uint64_t>(0x1000, m_Memory.size() / 4);
		CString text;
		text.Format(L"0x%llX", m_Address);
		m_AddressEdit.SetWindowText(text);
	}
	if (!m_AsmBytes.empty() && (m_Instructions.empty() || m_Instructions[0].Address != m_Address || !FitsInMemory(m_Address, m_AsmBytes.size()))) {
		// the code was made for an address the memory doesn't have now
		m_AsmBytes.clear();
		m_Instructions.clear();
		m_AddressToSourceLine.clear();
		m_AssembledSource.clear();
		m_Disassembly.SetReadOnly(false);
		m_Disassembly.SetText("; the code doesn't fit in the memory now: assemble it again\n");
		m_Disassembly.SetReadOnly(true);
	}
	auto bytes = m_AsmBytes;
	InitEmulator(bytes.data(), bytes.size(), !bytes.empty());
	CString status;
	status.Format(L"Memory: 0x%llX-0x%llX, stack %llu KB", m_MemoryBase, MemoryEnd() - 1, m_StackSize / 1024);
	m_StatusBar.SetText(0, status);
	return 0;
}

bool CMainFrame::OpenFile(PCWSTR file) {
	// (as the recent files keep it: a relative path, as from the command line, wouldn't do later)
	WCHAR path[MAX_PATH];
	if (!::GetFullPathName(file, _countof(path), path, nullptr))
		wcscpy_s(path, file);

	if (::GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {
		CString text;
		text.Format(L"File not found: %s", path);
		AtlMessageBox(m_hWnd, (PCWSTR)text, IDR_MAINFRAME, MB_ICONERROR);
		return false;
	}

	auto bytes = Helpers::ReadFileContents(path);
	if (CString(path).Right(4).CompareNoCase(L".asm") != 0) {
		// a binary file is disassembled; the source (and its file) stays as it is
		if (!ParseAddress())
			return false;
		if (!FitsInMemory(m_Address, bytes.size())) {
			AtlMessageBox(m_hWnd, L"The file doesn't fit in the emulator's memory (below the stack) at this address.", IDR_MAINFRAME, MB_ICONWARNING);
			return false;
		}
		m_AsmBytes = std::move(bytes);
		m_AssembledSource.clear();
		m_AddressToSourceLine.clear();
		Disassemble(m_AsmBytes.data(), m_AsmBytes.size());
		AddRecentFile(path);
		return true;
	}

	if (!SaveChangesIfNeeded())
		return false;

	std::string text(bytes.begin(), bytes.end());
	m_AsmSource.SetText(text.c_str());
	m_AsmSource.EmptyUndoBuffer();
	m_AsmSource.SetSavePoint();
	m_FileName = path;
	UpdateTitle();
	AddRecentFile(path);
	return true;
}

void CMainFrame::OpenFindDialog(bool replace) {
	// what is searched: the editor with the focus (the source to replace in)
	auto editor = GetFocusedEditor();
	m_FindTarget = replace || !editor ? &m_AsmSource : editor;

	// the selection (on one line) to look for
	auto selection = m_FindTarget->GetSelText();
	if (!selection.empty() && selection.find_first_of("\r\n") == std::string::npos)
		m_SearchText = CA2W(selection.c_str(), CP_UTF8);

	if (m_pFindDlg && m_FindReplace != replace) {
		m_pFindDlg->DestroyWindow();
		m_pFindDlg = nullptr;
	}
	if (!m_pFindDlg) {
		m_pFindDlg = new CFindReplaceDialog;
		m_FindReplace = replace;
		DWORD flags = FR_DOWN | (m_MatchCase ? FR_MATCHCASE : 0) | (m_WholeWord ? FR_WHOLEWORD : 0);
		m_pFindDlg->Create(!replace, m_SearchText, m_ReplaceText, flags, m_hWnd);
		m_pFindDlg->ShowWindow(SW_SHOW);
	}
	else {
		m_pFindDlg->SetDlgItemText(edt1, m_SearchText);
		m_pFindDlg->SetFocus();
	}
}

void CMainFrame::FindNext(bool down) {
	if (m_SearchText.IsEmpty()) {
		OpenFindDialog(false);
		return;
	}
	if (auto editor = GetFocusedEditor())
		m_FindTarget = editor;
	if (!m_FindTarget)
		m_FindTarget = &m_AsmSource;

	if (!m_FindTarget->Find((PCSTR)CW2A(m_SearchText, CP_UTF8), down, m_MatchCase, m_WholeWord)) {
		::MessageBeep(MB_ICONWARNING);
		m_StatusBar.SetText(0, L"Not found: " + m_SearchText);
	}
}

LRESULT CMainFrame::OnEditFind(WORD, WORD wID, HWND, BOOL&) {
	OpenFindDialog(wID == ID_EDIT_REPLACE);
	return 0;
}

LRESULT CMainFrame::OnEditFindNext(WORD, WORD wID, HWND, BOOL&) {
	FindNext(wID == ID_EDIT_FINDNEXT);
	return 0;
}

LRESULT CMainFrame::OnFindReplace(UINT, WPARAM, LPARAM lParam, BOOL&) {
	auto dlg = CFindReplaceDialog::GetNotifier(lParam);
	if (dlg->IsTerminating()) {
		m_pFindDlg = nullptr;
		return 0;
	}

	m_SearchText = dlg->GetFindString();
	m_ReplaceText = dlg->GetReplaceString();
	m_MatchCase = dlg->MatchCase();
	m_WholeWord = dlg->MatchWholeWord();
	if (!m_FindTarget)
		m_FindTarget = &m_AsmSource;
	CW2A text(m_SearchText, CP_UTF8), replacement(m_ReplaceText, CP_UTF8);
	bool found = true;
	if (dlg->FindNext())
		found = m_FindTarget->Find((PCSTR)text, dlg->SearchDown() != FALSE, m_MatchCase, m_WholeWord);
	else if (dlg->ReplaceCurrent())
		found = m_FindTarget->Replace((PCSTR)text, (PCSTR)replacement, m_MatchCase, m_WholeWord);
	else if (dlg->ReplaceAll()) {
		auto count = m_FindTarget->ReplaceAll((PCSTR)text, (PCSTR)replacement, m_MatchCase, m_WholeWord);
		CString status;
		status.Format(L"Replaced %d", count);
		m_StatusBar.SetText(0, status);
		found = count > 0;
	}
	if (!found) {
		::MessageBeep(MB_ICONWARNING);
		m_StatusBar.SetText(0, L"Not found: " + m_SearchText);
	}
	return 0;
}

LRESULT CMainFrame::OnRecentFile(WORD, WORD wID, HWND, BOOL&) {
	auto index = wID - ATL_IDS_MRU_FILE;
	if (index >= m_RecentFiles.Files().size())
		return 0;

	auto path = m_RecentFiles.Files()[index];
	if (!OpenFile(path.c_str()) && ::GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES) {
		// gone: not offered again
		m_RecentFiles.RemoveFile(path);
		AppSettings::Get().RecentFiles(m_RecentFiles.Files());
		UpdateRecentFilesMenu();
	}
	return 0;
}

void CMainFrame::CreateToolBar() {
	ToolBarButtonInfo const buttons[] = {
		{ ID_FILE_NEW, IDI_FILENEW },
		{ ID_FILE_OPEN, IDI_OPEN },
		{ ID_FILE_SAVE, IDI_SAVE },
		{ 0 },
		{ ID_MODE_16BIT, IDI_16BIT },
		{ ID_MODE_32BIT, IDI_32BIT },
		{ ID_MODE_64BIT, IDI_64BIT },
		{ 0 },
		{ ID_ASM_ASSEMBLE, IDI_BUILD },
		{ 0 },
		{ ID_ASM_RUN, IDI_RUN },
		{ ID_ASM_BREAK, IDI_BREAK },
		{ ID_ASM_STOP, IDI_STOP },
		{ ID_ASM_RESTART, IDI_RESTART },
		{ 0 },
		{ ID_ASM_STEPOVER, IDI_STEPOVER },
		{ ID_ASM_STEPINTO, IDI_STEPINTO },
		{ ID_ASM_STEPOUT, IDI_STEPOUT },
		{ 0 },
		{ ID_ASM_TOGGLEBP, IDI_BREAKPOINT },
	};
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	CToolBarCtrl tb(ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons)));

	// the assembler and the address go on the toolbar (over separators as wide as they are), each after a label:
	// rebar band titles aren't drawn with the dark mode's rebar painting
	auto font = AtlGetDefaultGuiFont();
	auto addSpace = [&](int width) {
		TBBUTTON button{ width, 0, TBSTATE_ENABLED, BTNS_SEP };
		tb.AddButtons(1, &button);
		CRect rc;
		tb.GetItemRect(tb.GetButtonCount() - 1, &rc);
		return rc;
		};
	auto addLabel = [&](CStatic& label, PCWSTR text, int width) {
		auto rc = addSpace(width);
		label.Create(tb, rc, text, WS_CHILD | WS_VISIBLE | SS_RIGHT | SS_CENTERIMAGE);
		label.SetFont(font);
		};
	auto center = [](CRect rc, int height) {
		rc.top += (rc.Height() - height) / 2;
		rc.bottom = rc.top + height;
		return rc;
		};

	addSpace(8);
	addLabel(m_AssemblerLabel, L"Assembler: ", 70);
	auto rc = addSpace(100);
	// (the height given is the list's; the closed box is as high as the font needs, and goes in the middle)
	CRect rcCombo(rc.left, rc.top, rc.right, rc.top + 200);
	m_AssemblerCombo.Create(tb, rcCombo, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST, 0, IDC_ASSEMBLER);
	m_AssemblerCombo.SetFont(font);
	for (auto& assembler : m_Assemblers)
		m_AssemblerCombo.AddString(assembler->GetName());
	m_AssemblerCombo.SetCurSel(m_AssemblerIndex);
	m_AssemblerCombo.GetWindowRect(&rcCombo);
	m_AssemblerCombo.SetWindowPos(nullptr, rc.left, center(rc, rcCombo.Height()).top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
	addLabel(m_AddressLabel, L"Address: ", 60);
	auto rcEdit = center(addSpace(90), 22);
	CString address;
	address.Format(L"0x%llX", m_Address);
	m_AddressEdit.Create(tb, rcEdit, address, WS_CHILD | WS_VISIBLE | ES_RIGHT | ES_AUTOHSCROLL, WS_EX_CLIENTEDGE, IDC_ADDRESS);
	m_AddressEdit.SetFont(font);

	AddSimpleReBarBand(tb, nullptr, FALSE, 0, TRUE);
	UIAddToolBar(tb);
}

void CMainFrame::CreateBottomPane() {
	m_Tabs.m_bTabCloseButton = FALSE;
	m_Tabs.Create(m_RightSplitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	CImageList images;
	images.Create(16, 16, ILC_COLOR32, 3, 0);
	for (auto id : { IDI_REGISTERS, IDI_MEMORY, IDI_BREAKPOINT, IDI_STACK })
		images.AddIcon(AtlLoadIconImage(id, 0, 16, 16));
	m_Tabs.SetImageList(images);	// (destroyed by the tab view)

	m_RegistersView.Create(m_Tabs, rcDefault, nullptr, WS_CHILD | WS_CLIPCHILDREN);
	m_MemoryView.Create(m_Tabs, rcDefault, nullptr, WS_CHILD | WS_CLIPCHILDREN);
	m_BreakpointsView.Create(m_Tabs, rcDefault, nullptr, WS_CHILD | WS_CLIPCHILDREN);
	m_StackView.Create(m_Tabs, rcDefault, nullptr, WS_CHILD | WS_CLIPCHILDREN);

	m_Tabs.AddPage(m_RegistersView, L"Registers", 0);
	m_Tabs.AddPage(m_MemoryView, L"Memory", 1);
	m_Tabs.AddPage(m_BreakpointsView, L"Breakpoints", 2);
	m_Tabs.AddPage(m_StackView, L"Stack", 3);
	m_Tabs.SetActivePage(0);
}

LRESULT CMainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	auto& settings = AppSettings::Get();
	// as last used
	m_AssemblerIndex = std::clamp(settings.Assembler(), 0, (int)m_Assemblers.size() - 1);
	m_AsmMode = static_cast<AsmMode>(std::clamp(settings.AsmMode(), 0, 2));
	if (uint64_t address; Helpers::ParseNumber(settings.Address().c_str(), address) && FitsInMemory(address, 1))
		m_Address = address;
	else
		m_Address = m_MemoryBase + std::min<uint64_t>(0x1000, m_Memory.size() / 4);

	CreateToolBar();
	CreateSimpleStatusBar();
	m_StatusBar.Attach(m_hWndStatusBar);
	int parts[] = { 200, 500, -1 };
	m_StatusBar.SetParts(_countof(parts), parts);
	m_StatusBar.SetText(1, L"Idle");
	if (!settings.ViewStatusBar())
		m_StatusBar.ShowWindow(SW_HIDE);

	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_AsmSource.Create(m_Splitter, IDC_ASM_SOURCE);
	m_RightSplitter.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Disassembly.Create(m_RightSplitter, IDC_DISASSEMBLY, true);
	CreateBottomPane();
	m_RightSplitter.SetSplitterPanes(m_Disassembly, m_Tabs);
	m_Splitter.SetSplitterPanes(m_AsmSource, m_RightSplitter);
	m_Splitter.SetSplitterPosPct(40);
	m_RightSplitter.SetSplitterPosPct(50);
	ApplyEditorFont();

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	CMenuHandle hMenu = GetMenu();
	InitMenu(hMenu);
	UIAddMenu(hMenu);
	m_RecentFiles.Set(settings.RecentFiles());
	UpdateRecentFilesMenu();


	if (!settings.LoadWindowPosition(m_hWnd, L"MainWindowPlacement")) {
		SetWindowPos(nullptr, 0, 0, 1300, 900, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		CenterWindow();
	}

	UpdateUI();
	m_AsmSource.SetFocus();
	// the dark mode hook themes the frame after WM_CREATE: the theme goes on everything once it has
	PostMessage(WM_UPDATE_DARKMODE);
	return 0;
}

LRESULT CMainFrame::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	// not closing if the user canceled
	bHandled = !SaveChangesIfNeeded();
	if (!bHandled)
		StopEmulator();
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	// (not only closed: a thread waiting at a breakpoint would never end)
	StopEmulator();

	auto& settings = AppSettings::Get();
	settings.SaveWindowPosition(m_hWnd, L"MainWindowPlacement");
	settings.Assembler(m_AssemblerIndex);
	settings.AsmMode((int)m_AsmMode);
	CString address;
	m_AddressEdit.GetWindowText(address);
	settings.Address((PCWSTR)address);
	settings.ViewStatusBar(IsStatusBarVisible());

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnSettingChange(UINT, WPARAM, LPARAM lParam, BOOL& bHandled) {
	// the system switched between light and dark
	if (lParam && ::wcscmp((PCWSTR)lParam, L"ImmersiveColorSet") == 0 && AppSettings::Get().ThemeKind() == DarkModeKind::System)
		ApplyTheme();
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnUpdateDarkMode(UINT, WPARAM, LPARAM, BOOL&) {
	ApplyTheme();
	return 0;
}

LRESULT CMainFrame::OnSavePointChanged(int, LPNMHDR, BOOL&) {
	UpdateTitle();
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD, WORD, HWND, BOOL&) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD, WORD, HWND, BOOL&) {
	CAboutDlg dlg;
	dlg.DoModal(m_hWnd);
	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD, WORD, HWND, BOOL&) {
	CSimpleFileDialog dlg(TRUE, L"asm", nullptr, OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING,
		L"Assembly files (*.asm)\0*.asm\0Binary files (*.bin)\0*.bin\0All files (*.*)\0*.*\0", m_hWnd);
	SuspendResumeHook sr;
	if (dlg.DoModal() != IDOK)
		return 0;

	OpenFile(dlg.m_szFileName);
	return 0;
}

LRESULT CMainFrame::OnFileSave(WORD, WORD, HWND, BOOL&) {
	if (m_FileName.IsEmpty())
		DoSaveAs();
	else
		SaveSource(m_FileName);
	return 0;
}

LRESULT CMainFrame::OnFileSaveAs(WORD, WORD, HWND, BOOL&) {
	DoSaveAs();
	return 0;
}

bool CMainFrame::NewSource(const char* text) {
	if (!SaveChangesIfNeeded())
		return false;

	m_AsmSource.SetText(text);
	m_AsmSource.Execute(SCI_CONVERTEOLS, m_AsmSource.Execute(SCI_GETEOLMODE));
	m_AsmSource.EmptyUndoBuffer();
	m_AsmSource.SetSavePoint();
	m_AsmSource.MarkErrorLines({});
	m_Disassembly.SetReadOnly(false);
	m_Disassembly.ClearAll();
	m_Disassembly.SetReadOnly(true);
	m_AsmBytes.clear();
	m_Instructions.clear();
	m_AddressToSourceLine.clear();
	m_AssembledSource.clear();
	InitEmulator(nullptr, 0);
	m_FileName.Empty();
	UpdateTitle();
	m_AsmSource.SetFocus();
	return true;
}

LRESULT CMainFrame::OnFileClose(WORD, WORD, HWND, BOOL&) {
	NewSource("");
	return 0;
}

LRESULT CMainFrame::OnFileNew(WORD, WORD, HWND, BOOL&) {
	NewSource("");
	return 0;
}

LRESULT CMainFrame::OnFileNewFromTemplate(WORD, WORD wID, HWND, BOOL&) {
	int index = wID - ID_FILE_NEW16BIT;
	if (!NewSource(SourceTemplates[index]))
		return 0;

	// (the template is written for its mode)
	m_AsmMode = static_cast<AsmMode>(index);
	UpdateUI();
	return 0;
}

void CMainFrame::ApplyEditorFont() {
	auto& settings = AppSettings::Get();
	std::string face(CW2A(settings.FontFace().c_str(), CP_UTF8));
	auto size = std::clamp(settings.FontSize(), 6, 72);
	m_AsmSource.SetEditorFont(face, size);
	m_Disassembly.SetEditorFont(face, size);
}

LRESULT CMainFrame::OnOptionsFont(WORD, WORD, HWND, BOOL&) {
	auto& settings = AppSettings::Get();
	LOGFONT lf{};
	::wcsncpy_s(lf.lfFaceName, settings.FontFace().c_str(), _TRUNCATE);
	CClientDC dc(m_hWnd);
	lf.lfHeight = -::MulDiv(settings.FontSize(), dc.GetDeviceCaps(LOGPIXELSY), 72);
	// (fixed pitch: the columns of the code line up)
	CFontDialog dlg(&lf, CF_SCREENFONTS | CF_FIXEDPITCHONLY | CF_NOVERTFONTS | CF_INITTOLOGFONTSTRUCT, nullptr, m_hWnd);
	if (dlg.DoModal(m_hWnd) != IDOK)
		return 0;

	settings.FontFace(dlg.GetFaceName());
	// (tenths of a point)
	settings.FontSize(std::max(1, (dlg.GetSize() + 5) / 10));
	ApplyEditorFont();
	return 0;
}

LRESULT CMainFrame::OnOptionsAutoComplete(WORD, WORD, HWND, BOOL&) {
	auto& settings = AppSettings::Get();
	settings.AutoComplete(!settings.AutoComplete());
	UpdateUI();
	return 0;
}

LRESULT CMainFrame::OnEditCompleteWord(WORD, WORD, HWND, BOOL&) {
	if (::GetFocus() == m_AsmSource)
		m_AsmSource.AutoComplete(true);
	return 0;
}

LRESULT CMainFrame::OnSourceCharAdded(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	if (AppSettings::Get().AutoComplete() && scn->ch > ' ')
		m_AsmSource.AutoComplete(false);
	return 0;
}

LRESULT CMainFrame::OnEditCommand(WORD, WORD wID, HWND, BOOL&) {
	if (auto& hex = m_MemoryView.GetHexControl(); ::GetFocus() == hex) {
		switch (wID) {
			case ID_EDIT_UNDO: hex.Undo(); break;
			case ID_EDIT_REDO: hex.Redo(); break;
			case ID_EDIT_CUT: hex.Cut(); break;
			case ID_EDIT_COPY: hex.Copy(); break;
			case ID_EDIT_PASTE: hex.Paste(); break;
			case ID_EDIT_CLEAR: hex.Delete(); break;
		}
		return 0;
	}

	auto editor = GetFocusedEditor();
	if (!editor)
		return 0;

	switch (wID) {
		case ID_EDIT_UNDO: editor->Undo(); break;
		case ID_EDIT_REDO: editor->Redo(); break;
		case ID_EDIT_CUT: editor->Cut(); break;
		case ID_EDIT_COPY: editor->Copy(); break;
		case ID_EDIT_PASTE: editor->Paste(); break;
		case ID_EDIT_CLEAR: editor->Clear(); break;
	}
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD, WORD, HWND, BOOL&) {
	BOOL visible = !IsStatusBarVisible();
	::ShowWindow(m_hWndStatusBar, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, visible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAssemble(WORD, WORD, HWND, BOOL&) {
	CWaitCursor wait;
	Assemble();
	return 0;
}

LRESULT CMainFrame::OnAssembler(WORD, WORD wID, HWND, BOOL&) {
	m_AssemblerIndex = wID - ID_ASSEMBLER_KEYSTONE;
	m_AssemblerCombo.SetCurSel(m_AssemblerIndex);
	UpdateUI();
	return 0;
}

LRESULT CMainFrame::OnAssemblerSelected(WORD, WORD, HWND, BOOL&) {
	m_AssemblerIndex = m_AssemblerCombo.GetCurSel();
	UpdateUI();
	return 0;
}

LRESULT CMainFrame::OnAsmMode(WORD, WORD wID, HWND, BOOL&) {
	m_AsmMode = static_cast<AsmMode>(wID - ID_MODE_16BIT);
	UpdateUI();
	if (!m_AsmBytes.empty())
		Disassemble(m_AsmBytes.data(), m_AsmBytes.size());
	return 0;
}

LRESULT CMainFrame::OnTheme(WORD, WORD wID, HWND, BOOL&) {
	AppSettings::Get().Theme(wID - ID_THEME_LIGHT);
	ApplyTheme();
	UpdateUI();
	return 0;
}
