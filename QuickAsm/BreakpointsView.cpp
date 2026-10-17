#include "pch.h"
#include "BreakpointsView.h"
#include <ToolbarHelper.h>

void CBreakpointsView::Refresh() {
	auto selected = m_List.GetSelectedIndex();
	auto address = selected >= 0 && selected < (int)m_Items.size() ? m_Items[selected].Address : 0;

	m_Items = m_Host.GetBreakpoints();
	m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOSCROLL);
	m_List.RedrawItems(0, m_List.GetItemCount());

	// the same breakpoint stays selected
	if (selected >= 0) {
		auto it = std::ranges::find(m_Items, address, &BreakpointInfo::Address);
		if (it != m_Items.end())
			m_List.SelectItem((int)(it - m_Items.begin()));
	}
	UpdateToolBar();
}

CString CBreakpointsView::GetColumnText(HWND, int row, int col) const {
	auto& bp = m_Items[row];
	CString text;
	switch (col) {
		case 0: text.Format(L"0x%llX", bp.Address); break;
		case 1: text.Format(L"%d", bp.Line + 1); break;
		case 2: return CString(bp.Instruction.c_str());
		case 3: return CString(bp.Condition.c_str());
		case 4:
			if (bp.HitTarget > 1)
				text.Format(L"%u (from %u)", bp.Hits, bp.HitTarget);
			else
				text.Format(L"%u", bp.Hits);
			break;
	}
	return text;
}

ListViewRowCheck CBreakpointsView::IsRowChecked(HWND, int row) const {
	return m_Items[row].Enabled ? ListViewRowCheck::Checked : ListViewRowCheck::Unchecked;
}

bool CBreakpointsView::OnDoubleClickList(HWND, int row, int col, POINT const&) {
	if (row < 0)
		return true;
	if (col >= 3)
		m_Host.EditBreakpoint(m_Items[row].Address);
	else
		m_Host.GoToBreakpoint(m_Items[row].Address);
	return true;
}

void CBreakpointsView::OnListViewClick(HWND, int row, int, POINT const& pt) {
	// (an owner data list view doesn't check its rows by itself)
	LVHITTESTINFO info{ pt };
	if (row >= 0 && m_List.HitTest(&info) >= 0 && (info.flags & LVHT_ONITEMSTATEICON))
		ToggleEnabled(row);
}

void CBreakpointsView::ToggleEnabled(int row) {
	auto& bp = m_Items[row];
	m_Host.EnableBreakpoint(bp.Address, !bp.Enabled);
	// (the host refreshes the view)
}

void CBreakpointsView::UpdateToolBar() {
	auto any = !m_Items.empty();
	m_ToolBar.EnableButton(ID_BREAKPOINTS_DELETE, m_List.GetSelectedIndex() >= 0);
	m_ToolBar.EnableButton(ID_BREAKPOINTS_CONDITION, m_List.GetSelectedIndex() >= 0);
	m_ToolBar.EnableButton(ID_BREAKPOINTS_DELETEALL, any);
	m_ToolBar.EnableButton(ID_BREAKPOINTS_ENABLEALL, any);
	m_ToolBar.EnableButton(ID_BREAKPOINTS_DISABLEALL, any);
}

LRESULT CBreakpointsView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	ToolBarButtonInfo const buttons[] = {
		{ ID_BREAKPOINTS_DELETE, 0, BTNS_BUTTON | BTNS_AUTOSIZE, L"Delete" },
		{ ID_BREAKPOINTS_DELETEALL, 0, BTNS_BUTTON | BTNS_AUTOSIZE, L"Delete All" },
		{ 0 },
		{ ID_BREAKPOINTS_CONDITION, 0, BTNS_BUTTON | BTNS_AUTOSIZE, L"Condition..." },
		{ 0 },
		{ ID_BREAKPOINTS_ENABLEALL, 0, BTNS_BUTTON | BTNS_AUTOSIZE, L"Enable All" },
		{ ID_BREAKPOINTS_DISABLEALL, 0, BTNS_BUTTON | BTNS_AUTOSIZE, L"Disable All" },
	};
	m_ToolBar.Attach(ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons), 16));

	m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, IDC_BREAKPOINTS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_CHECKBOXES);
	m_List.InsertColumn(0, L"Address", LVCFMT_LEFT, 130);
	m_List.InsertColumn(1, L"Line", LVCFMT_RIGHT, 50);
	m_List.InsertColumn(2, L"Instruction", LVCFMT_LEFT, 200);
	m_List.InsertColumn(3, L"Condition", LVCFMT_LEFT, 200);
	m_List.InsertColumn(4, L"Hits", LVCFMT_RIGHT, 90);
	UpdateToolBar();
	return 0;
}

LRESULT CBreakpointsView::OnSize(UINT, WPARAM, LPARAM, BOOL&) {
	CRect rc;
	GetClientRect(&rc);
	CSize size;
	m_ToolBar.GetButtonSize(size);
	int height = size.cy + 4;
	m_ToolBar.MoveWindow(0, 0, rc.Width(), height);
	m_List.MoveWindow(0, height, rc.Width(), rc.Height() - height);
	return 0;
}

LRESULT CBreakpointsView::OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
	m_List.SetFocus();
	return 0;
}

LRESULT CBreakpointsView::OnListKeyDown(int, LPNMHDR pnmh, BOOL&) {
	auto kd = (NMLVKEYDOWN*)pnmh;
	auto row = m_List.GetSelectedIndex();
	if (row < 0)
		return 0;

	switch (kd->wVKey) {
		case VK_SPACE: ToggleEnabled(row); break;
		case VK_DELETE: m_Host.DeleteBreakpoint(m_Items[row].Address); break;
		case VK_RETURN: m_Host.GoToBreakpoint(m_Items[row].Address); break;
		case VK_F2: m_Host.EditBreakpoint(m_Items[row].Address); break;
	}
	return 0;
}

LRESULT CBreakpointsView::OnItemChanged(int, LPNMHDR, BOOL& bHandled) {
	UpdateToolBar();
	bHandled = FALSE;
	return 0;
}

LRESULT CBreakpointsView::OnDelete(WORD, WORD, HWND, BOOL&) {
	if (auto row = m_List.GetSelectedIndex(); row >= 0)
		m_Host.DeleteBreakpoint(m_Items[row].Address);
	return 0;
}

LRESULT CBreakpointsView::OnCondition(WORD, WORD, HWND, BOOL&) {
	if (auto row = m_List.GetSelectedIndex(); row >= 0)
		m_Host.EditBreakpoint(m_Items[row].Address);
	return 0;
}

LRESULT CBreakpointsView::OnDeleteAll(WORD, WORD, HWND, BOOL&) {
	m_Host.DeleteAllBreakpoints();
	return 0;
}

LRESULT CBreakpointsView::OnEnableAll(WORD, WORD wID, HWND, BOOL&) {
	m_Host.EnableAllBreakpoints(wID == ID_BREAKPOINTS_ENABLEALL);
	return 0;
}
