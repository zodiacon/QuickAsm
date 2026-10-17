#include "pch.h"
#include "StackView.h"

void CStackView::SetEntries(std::vector<StackEntry> entries, int entrySize) {
	m_Entries = std::move(entries);
	m_EntrySize = entrySize;
	m_List.SetItemCountEx((int)m_Entries.size(), LVSICF_NOSCROLL);
	m_List.RedrawItems(0, m_List.GetItemCount());
}

CString CStackView::GetColumnText(HWND, int row, int col) const {
	auto& entry = m_Entries[row];
	CString text;
	switch (col) {
		case 0: text.Format(L"0x%llX", entry.Address); break;
		case 1: text.Format(L"0x%0*llX", m_EntrySize * 2, entry.Value); break;
		case 2: return entry.Notes;
	}
	return text;
}

LRESULT CStackView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, IDC_STACK);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_List.InsertColumn(0, L"Address", LVCFMT_LEFT, 100);
	m_List.InsertColumn(1, L"Value", LVCFMT_RIGHT, 160);
	m_List.InsertColumn(2, L"Notes", LVCFMT_LEFT, 320);
	return 0;
}

LRESULT CStackView::OnSize(UINT, WPARAM, LPARAM, BOOL&) {
	CRect rc;
	GetClientRect(&rc);
	m_List.MoveWindow(&rc);
	return 0;
}

LRESULT CStackView::OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
	m_List.SetFocus();
	return 0;
}
