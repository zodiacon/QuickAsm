#include "pch.h"
#include "MemoryView.h"
#include <ToolbarHelper.h>
#include <WTLHelper.h>

uint32_t CFixedMemoryBuffer::GetData(int64_t offset, uint8_t* buffer, uint32_t count) {
	if (offset < 0 || offset >= GetSize())
		return 0;
	count = (uint32_t)std::min<int64_t>(count, GetSize() - offset);
	::memcpy(buffer, m_Memory.data() + offset, count);
	return count;
}

bool CFixedMemoryBuffer::SetData(int64_t offset, const uint8_t* data, uint32_t count) {
	if (offset < 0 || offset + count > GetSize())
		return false;
	::memcpy(m_Memory.data() + offset, data, count);
	return true;
}

bool CFixedMemoryBuffer::SetData(int64_t offset, uint8_t value, uint32_t count) {
	if (offset < 0 || offset + count > GetSize())
		return false;
	::memset(m_Memory.data() + offset, value, count);
	return true;
}

BOOL CMemoryView::PreTranslateMessage(MSG* pMsg) {
	// the memory has a fixed size: no insert mode
	return pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_INSERT && pMsg->hwnd == m_Hex.m_hWnd;
}

void CMemoryView::Refresh() {
	m_Hex.Refresh();
}

void CMemoryView::Reset() {
	// (the modified bytes, the undo history and the selection go too)
	m_Hex.SetBufferManager(&m_Buffer);
}

void CMemoryView::GotoAddress(uint64_t address) {
	m_PendingAddress = (int64_t)address;
	GotoPending();
}

void CMemoryView::GotoPending() {
	CRect rc;
	m_Hex.GetClientRect(&rc);
	if (m_PendingAddress >= 0 && rc.Height() > 0 && IsWindowVisible()) {
		m_Hex.GotoOffset(m_PendingAddress);
		m_PendingAddress = -1;
	}
}

void CMemoryView::ApplyTheme() {
	auto dark = WTLHelper::IsDarkMode();
	// the hex control's scroll bars
	::SetWindowTheme(m_Hex, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
	auto& colors = m_Hex.GetColors();
	colors.Text = ::GetSysColor(COLOR_WINDOWTEXT);
	colors.Background = ::GetSysColor(COLOR_WINDOW);
	colors.Ascii = dark ? RGB(0, 240, 240) : RGB(128, 0, 0);
	colors.Offset = dark ? RGB(240, 240, 0) : RGB(0, 0, 128);
	colors.Ruler = dark ? RGB(100, 160, 255) : RGB(0, 0, 192);
	colors.Modified = dark ? RGB(255, 96, 96) : RGB(255, 0, 0);
	colors.SelectionText = dark ? RGB(255, 255, 255) : ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	colors.SelectionBackground = dark ? RGB(38, 79, 120) : ::GetSysColor(COLOR_HIGHLIGHT);
	m_Hex.Invalidate();
}

LRESULT CMemoryView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	ToolBarButtonInfo const buttons[] = {
		{ ID_MEMORY_1BYTE, IDI_1BYTE, BTNS_CHECKGROUP },
		{ ID_MEMORY_2BYTES, IDI_2BYTES, BTNS_CHECKGROUP },
		{ ID_MEMORY_4BYTES, IDI_4BYTES, BTNS_CHECKGROUP },
		{ ID_MEMORY_8BYTES, IDI_8BYTES, BTNS_CHECKGROUP },
	};
	m_ToolBar.Attach(ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons), 16));
	m_ToolBar.CheckButton(ID_MEMORY_1BYTE);

	m_Hex.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS);
	m_Hex.SetFont(L"Consolas", 110);
	m_Hex.SetBufferManager(&m_Buffer);
	m_Hex.SetReadOnly(false);	// (read-only by default)
	m_Hex.SetBytesPerLine(16);
	ApplyTheme();
	return 0;
}

LRESULT CMemoryView::OnSize(UINT, WPARAM, LPARAM, BOOL&) {
	CRect rc;
	GetClientRect(&rc);
	CSize size;
	m_ToolBar.GetButtonSize(size);
	int height = size.cy + 4;
	m_ToolBar.MoveWindow(0, 0, rc.Width(), height);
	m_Hex.MoveWindow(0, height, rc.Width(), rc.Height() - height);
	GotoPending();
	return 0;
}

LRESULT CMemoryView::OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
	m_Hex.SetFocus();
	return 0;
}

LRESULT CMemoryView::OnShowWindow(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
	// (not visible yet while this is handled)
	if (wParam && m_PendingAddress >= 0)
		PostMessage(WM_GOTO_PENDING);
	bHandled = FALSE;
	return 0;
}

LRESULT CMemoryView::OnGotoPending(UINT, WPARAM, LPARAM, BOOL&) {
	GotoPending();
	return 0;
}

LRESULT CMemoryView::OnDataSize(WORD, WORD wID, HWND, BOOL&) {
	m_Hex.SetDataSize(1 << (wID - ID_MEMORY_1BYTE));
	return 0;
}
