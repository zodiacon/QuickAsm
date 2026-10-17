#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "Interfaces.h"
#include <WTLHelper.h>

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	return CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
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
	SendMessageToDescendants(WM_UPDATE_DARKMODE);
}

void CMainFrame::UpdateUI() {
	auto& settings = AppSettings::Get();
	UISetRadioMenuItem(ID_THEME_LIGHT + settings.Theme(), ID_THEME_LIGHT, ID_THEME_SYSTEM);
	UISetRadioMenuItem(ID_MODE_16BIT + (int)m_AsmMode, ID_MODE_16BIT, ID_MODE_64BIT);
	UISetCheck(ID_VIEW_STATUS_BAR, IsStatusBarVisible());
}

bool CMainFrame::IsStatusBarVisible() const {
	// (not IsWindowVisible: false while the frame is hidden, as when it's created or destroyed)
	return (::GetWindowLongPtr(m_hWndStatusBar, GWL_STYLE) & WS_VISIBLE) != 0;
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

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	CMenuHandle hMenu = GetMenu();
	InitMenu(hMenu);
	UIAddMenu(hMenu);

	// not ported from wxWidgets yet
	static const UINT notYet[] = {
		ID_FILE_OPEN, ID_FILE_SAVE, ID_FILE_SAVE_AS, ID_FILE_CLOSE,
		ID_EDIT_UNDO, ID_EDIT_REDO, ID_EDIT_CUT, ID_EDIT_COPY, ID_EDIT_PASTE, ID_EDIT_CLEAR, ID_EDIT_FIND,
		ID_ASM_ASSEMBLE, ID_ASM_RUN, ID_ASM_STOP, ID_ASM_RESTART, ID_ASM_STEPOVER, ID_ASM_STEPINTO,
		ID_ASM_TOGGLEBP, ID_ASM_DELETEALLBP,
	};
	for (auto id : notYet)
		UIEnable(id, false);

	if (!settings.LoadWindowPosition(m_hWnd, L"MainWindowPlacement")) {
		SetWindowPos(nullptr, 0, 0, 1300, 900, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
		CenterWindow();
	}

	UpdateUI();
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

LRESULT CMainFrame::OnEraseBkgnd(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
	if (m_hWndClient) {
		bHandled = FALSE;
		return 0;
	}
	// no client window to cover the frame: the class brush is not themed (the system color is, for dark mode)
	CDCHandle dc((HDC)wParam);
	CRect rc;
	GetClientRect(&rc);
	dc.FillRect(&rc, ::GetSysColorBrush(COLOR_WINDOW));
	return 1;
}

LRESULT CMainFrame::OnFileExit(WORD, WORD, HWND, BOOL&) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD, WORD, HWND, BOOL&) {
	BOOL visible = !IsStatusBarVisible();
	::ShowWindow(m_hWndStatusBar, visible ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, visible);
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnAsmMode(WORD, WORD wID, HWND, BOOL&) {
	m_AsmMode = static_cast<AsmMode>(wID - ID_MODE_16BIT);
	UpdateUI();
	return 0;
}

LRESULT CMainFrame::OnTheme(WORD, WORD wID, HWND, BOOL&) {
	AppSettings::Get().Theme(wID - ID_THEME_LIGHT);
	ApplyTheme();
	UpdateUI();
	return 0;
}
