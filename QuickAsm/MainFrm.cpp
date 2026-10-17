#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "Interfaces.h"
#include "Helpers.h"
#include "KeystoneAssembler.h"
#include "NasmAssembler.h"
#include <WTLHelper.h>
#include <fstream>

CMainFrame::CMainFrame() {
	m_Assemblers.push_back(std::make_unique<KeystoneAssembler>());
	m_Assemblers.push_back(std::make_unique<NasmAssembler>());
}

CMainFrame::~CMainFrame() = default;

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	auto anyText = m_AsmSource.GetTextLength() > 0;
	UIEnable(ID_FILE_SAVE, m_AsmSource.IsModified() || (anyText && m_FileName.IsEmpty()));
	UIEnable(ID_FILE_SAVE_AS, anyText);
	UIEnable(ID_ASM_ASSEMBLE, anyText);

	auto editor = GetFocusedEditor();
	auto writable = editor && !editor->IsReadOnly();
	UIEnable(ID_EDIT_UNDO, writable && editor->CanUndo());
	UIEnable(ID_EDIT_REDO, writable && editor->CanRedo());
	UIEnable(ID_EDIT_CUT, writable && !editor->IsSelectionEmpty());
	UIEnable(ID_EDIT_COPY, editor && !editor->IsSelectionEmpty());
	UIEnable(ID_EDIT_PASTE, writable && editor->CanPaste());
	UIEnable(ID_EDIT_CLEAR, writable && !editor->IsSelectionEmpty());

	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::InitMenu(HMENU hMenu) {
	MenuItemData commands[] {
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
		{ ID_ASM_STOP, IDI_STOP },
		{ ID_ASM_RESTART, IDI_RESTART },
		{ ID_ASM_STEPOVER, IDI_STEPOVER },
		{ ID_ASM_STEPINTO, IDI_STEPINTO },
		{ ID_ASM_TOGGLEBP, IDI_BREAKPOINT },
	};
	WTLHelper::InitMenu(hMenu, commands, _countof(commands));
}

void CMainFrame::ApplyTheme() {
	auto& settings = AppSettings::Get();
	WTLHelper::SwitchToMode(settings.EffectiveTheme(), m_hWnd);
	// the menu item bitmaps are drawn on the menu's background
	InitMenu(GetMenu());
	DrawMenuBar();
	m_AsmSource.ApplyTheme();
	m_Disassembly.ApplyTheme();
	SendMessageToDescendants(WM_UPDATE_DARKMODE);
}

void CMainFrame::UpdateUI() {
	auto& settings = AppSettings::Get();
	UISetRadioMenuItem(ID_THEME_LIGHT + settings.Theme(), ID_THEME_LIGHT, ID_THEME_SYSTEM);
	UISetRadioMenuItem(ID_MODE_16BIT + (int)m_AsmMode, ID_MODE_16BIT, ID_MODE_64BIT);
	UISetRadioMenuItem(ID_ASSEMBLER_KEYSTONE + m_AssemblerIndex, ID_ASSEMBLER_KEYSTONE, ID_ASSEMBLER_NASM);
	UISetCheck(ID_VIEW_STATUS_BAR, IsStatusBarVisible());
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
	return true;
}

bool CMainFrame::DoSaveAs() {
	CSimpleFileDialog dlg(FALSE, L"asm", m_FileName.IsEmpty() ? nullptr : (PCWSTR)m_FileName, OFN_OVERWRITEPROMPT | OFN_EXPLORER | OFN_ENABLESIZING,
		L"Assembly files (*.asm)\0*.asm\0All files (*.*)\0*.*\0", m_hWnd);
	if (dlg.DoModal() != IDOK)
		return false;
	return SaveSource(dlg.m_szFileName);
}

void CMainFrame::Assemble() {
	auto assembler = m_Assemblers[m_AssemblerIndex].get();
	assembler->SetValue("mode", int64_t(1) << (4 + (int)m_AsmMode));
	assembler->SetValue("address", (int64_t)m_Address);

	auto results = assembler->Assemble(m_AsmSource.GetAllText());
	if (results.Error) {
		m_AsmBytes.clear();
		m_Instructions.clear();
		m_Disassembly.SetReadOnly(false);
		m_Disassembly.SetText(results.Output.c_str());
		m_Disassembly.SetReadOnly(true);
		m_StatusBar.SetText(0, L"Assembly failed");
		return;
	}

	if (results.Bytes.empty())
		m_AsmBytes = Helpers::ReadFileContents(results.OutputFile.c_str());
	else
		m_AsmBytes = std::move(results.Bytes);
	Disassemble(m_AsmBytes.data(), m_AsmBytes.size());
}

void CMainFrame::Disassemble(uint8_t const* data, size_t size) {
	CapstoneEngine cs;
	if (!cs.Open((cs_mode)(1 << (1 + (int)m_AsmMode))))
		return;

	m_Instructions = cs.Disassemble(data, size, m_Address);
	CStringA text;
	for (auto& insn : m_Instructions) {
		text.AppendFormat("0x%08X %-8s %-24s ;", (DWORD)insn.Address, insn.Mnemonic.c_str(), insn.Operands.c_str());
		for (auto b : insn.Bytes)
			text.AppendFormat(" %02X", b);
		text += "\n";
	}
	m_Disassembly.SetReadOnly(false);
	m_Disassembly.SetText(text);
	m_Disassembly.SetReadOnly(true);

	CString status;
	status.Format(L"%zu instructions, %zu bytes", m_Instructions.size(), size);
	m_StatusBar.SetText(0, status);
}

bool CMainFrame::OpenFile(PCWSTR path) {
	if (::GetFileAttributes(path) == INVALID_FILE_ATTRIBUTES) {
		CString text;
		text.Format(L"File not found: %s", path);
		AtlMessageBox(m_hWnd, (PCWSTR)text, IDR_MAINFRAME, MB_ICONERROR);
		return false;
	}

	auto bytes = Helpers::ReadFileContents(path);
	if (CString(path).Right(4).CompareNoCase(L".asm") != 0) {
		// a binary file is disassembled; the source (and its file) stays as it is
		m_AsmBytes = std::move(bytes);
		Disassemble(m_AsmBytes.data(), m_AsmBytes.size());
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
	return true;
}

LRESULT CMainFrame::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	auto& settings = AppSettings::Get();

	CreateSimpleStatusBar();
	m_StatusBar.Attach(m_hWndStatusBar);
	int parts[] = { 200, 500, -1 };
	m_StatusBar.SetParts(_countof(parts), parts);
	m_StatusBar.SetText(1, L"Idle");
	if (!settings.ViewStatusBar())
		m_StatusBar.ShowWindow(SW_HIDE);

	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_AsmSource.Create(m_Splitter, IDC_ASM_SOURCE);
	m_Disassembly.Create(m_Splitter, IDC_DISASSEMBLY, true);
	m_Splitter.SetSplitterPanes(m_AsmSource, m_Disassembly);
	m_Splitter.SetSplitterPosPct(40);

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	CMenuHandle hMenu = GetMenu();
	InitMenu(hMenu);
	UIAddMenu(hMenu);

	// not ported from wxWidgets yet
	static const UINT notYet[] = {
		ID_EDIT_FIND,
		ID_ASM_RUN, ID_ASM_STOP, ID_ASM_RESTART, ID_ASM_STEPOVER, ID_ASM_STEPINTO,
		ID_ASM_TOGGLEBP, ID_ASM_DELETEALLBP,
	};
	for (auto id : notYet)
		UIEnable(id, false);

	if (!settings.LoadWindowPosition(m_hWnd, L"MainWindowPlacement")) {
		SetWindowPos(nullptr, 0, 0, 1300, 900, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		CenterWindow();
	}

	UpdateUI();
	m_AsmSource.SetFocus();
	return 0;
}

LRESULT CMainFrame::OnClose(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	// not closing if the user canceled
	bHandled = !SaveChangesIfNeeded();
	return 0;
}

LRESULT CMainFrame::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled) {
	auto& settings = AppSettings::Get();
	settings.SaveWindowPosition(m_hWnd, L"MainWindowPlacement");
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

LRESULT CMainFrame::OnSavePointChanged(int, LPNMHDR, BOOL&) {
	UpdateTitle();
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD, WORD, HWND, BOOL&) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnFileOpen(WORD, WORD, HWND, BOOL&) {
	CSimpleFileDialog dlg(TRUE, L"asm", nullptr, OFN_FILEMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING,
		L"Assembly files (*.asm)\0*.asm\0Binary files (*.bin)\0*.bin\0All files (*.*)\0*.*\0", m_hWnd);
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

LRESULT CMainFrame::OnFileClose(WORD, WORD, HWND, BOOL&) {
	if (!SaveChangesIfNeeded())
		return 0;

	m_AsmSource.ClearAll();
	m_AsmSource.EmptyUndoBuffer();
	m_AsmSource.SetSavePoint();
	m_Disassembly.SetReadOnly(false);
	m_Disassembly.ClearAll();
	m_Disassembly.SetReadOnly(true);
	m_AsmBytes.clear();
	m_Instructions.clear();
	m_FileName.Empty();
	UpdateTitle();
	return 0;
}

LRESULT CMainFrame::OnEditCommand(WORD, WORD wID, HWND, BOOL&) {
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
