#include "pch.h"
#include "RegistersView.h"
#include "RegisterInfo.h"
#include "Helpers.h"
#include "EditValueDlg.h"
#include <ToolbarHelper.h>
#include <SortHelper.h>

CRegistersView::CRegistersView(UnicornEngine& emulator) : m_Emulator(emulator) {
}

void CRegistersView::Refresh() {
	m_Items.clear();
	if (m_Emulator.IsOpen()) {
		auto mode = (DWORD)m_Emulator.GetMode();
		for (int i = 0; i < (int)AllRegisters.size(); i++) {
			auto& ri = AllRegisters[i];
			if (((DWORD)ri.Category & mode) && (m_Filter & (ri.Size >> 3)))
				m_Items.push_back(i);
		}
	}
	Sort(m_List);
	m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOSCROLL);
	m_List.RedrawItems(0, m_List.GetItemCount());
}

void CRegistersView::UpdateValues() {
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

void CRegistersView::SetLive(bool live) {
	m_Live = live;
	UpdateValues();
}

RegisterInfo const& CRegistersView::GetRegister(int row) const {
	return AllRegisters[m_Items[row]];
}

CString CRegistersView::GetColumnText(HWND, int row, int col) const {
	auto& ri = GetRegister(row);
	CString text;
	switch (col) {
		case 0: return CString(ri.Name.c_str());
		case 1: text.Format(L"%d bit", ri.Size); break;
		case 2: return m_Live ? GetValueText(ri) : CString();
		case 3: return Helpers::RegisterTypeToString(ri.Category);
		case 4: return m_Live ? GetDetailsText(ri) : CString();
	}
	return text;
}

CString CRegistersView::GetValueText(RegisterInfo const& ri) const {
	CString text;
	if ((ri.Category & RegisterType::Descriptor) == RegisterType::None)
		text.Format(L"0x%llX", m_Emulator.ReadReg<size_t>(ri.Id));
	else {
		auto value = m_Emulator.ReadReg<uc_x86_mmr>(ri.Id);
		text.Format(L"0x%X:%llX", value.limit, value.base);
	}
	return text;
}

CString CRegistersView::GetDetailsText(RegisterInfo const& ri) const {
	if ((ri.Category & RegisterType::Descriptor) != RegisterType::None)
		return L"";

	auto value = m_Emulator.ReadReg<size_t>(ri.Id);
	if (ri.Id == x86Register::EFLAGS)
		return Helpers::CPUFlagsToString((uint32_t)value);

	CString text;
	switch (ri.Size) {
		case 8: text.Format(L"%u (%d)", (uint8_t)value, (int8_t)value); break;
		case 16: text.Format(L"%u (%d)", (uint16_t)value, (int16_t)value); break;
		case 32: text.Format(L"%u (%d)", (uint32_t)value, (int32_t)value); break;
		case 64: text.Format(L"%llu (%lld)", (uint64_t)value, (int64_t)value); break;
	}
	if (ri.Id == x86Register::CR0)
		text += L" " + Helpers::CR0ToString(value);
	return text;
}

void CRegistersView::DoSort(SortInfo const* si) {
	auto col = si->SortColumn;
	auto asc = si->SortAscending;
	std::ranges::sort(m_Items, [&](auto i1, auto i2) {
		auto& r1 = AllRegisters[i1];
		auto& r2 = AllRegisters[i2];
		switch (col) {
			case 0: return SortHelper::Sort(r1.Name, r2.Name, asc);
			case 1: return SortHelper::Sort(r1.Size, r2.Size, asc);
			case 3: return SortHelper::Sort(Helpers::RegisterTypeToString(r1.Category), Helpers::RegisterTypeToString(r2.Category), asc);
		}
		return false;
		});
}

bool CRegistersView::IsSortable(HWND, int col) const {
	// not by the values, which change as the code runs
	return col == 0 || col == 1 || col == 3;
}

bool CRegistersView::OnDoubleClickList(HWND, int row, int, POINT const&) {
	if (row >= 0)
		EditValue(row);
	return true;
}

bool CRegistersView::EditValue(int row) {
	auto& ri = GetRegister(row);
	if (!m_Live) {
		::MessageBeep(MB_ICONWARNING);
		return false;
	}
	if ((ri.Category & (RegisterType::General | RegisterType::Segment)) == RegisterType::None) {
		::MessageBeep(MB_ICONWARNING);
		return false;
	}

	CString title(L"Value for register ");
	title += ri.Name.c_str();
	CEditValueDlg dlg(title, GetValueText(ri));
	if (dlg.DoModal(m_hWnd) != IDOK || dlg.GetValue().IsEmpty())
		return false;

	m_Emulator.WriteReg(ri.Id, (uint64_t)::wcstoull(dlg.GetValue(), nullptr, 0));
	// (the registers that overlap it change too)
	UpdateValues();
	return true;
}

LRESULT CRegistersView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	ToolBarButtonInfo const buttons[] = {
		{ ID_REGS_8BIT, IDI_8BIT, BTNS_CHECK },
		{ ID_REGS_16BIT, IDI_16BIT, BTNS_CHECK },
		{ ID_REGS_32BIT, IDI_32BIT, BTNS_CHECK },
		{ ID_REGS_64BIT, IDI_64BIT, BTNS_CHECK },
		{ 0 },
		{ ID_REGS_EDIT, IDI_EDIT },
	};
	m_ToolBar.Attach(ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons), 16));
	for (int i = 0; i < 4; i++)
		m_ToolBar.CheckButton(ID_REGS_8BIT + i, (m_Filter & (1 << i)) != 0);

	m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, IDC_REGISTERS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_List.InsertColumn(0, L"Name", LVCFMT_LEFT, 70);
	m_List.InsertColumn(1, L"Size", LVCFMT_RIGHT, 60);
	m_List.InsertColumn(2, L"Value", LVCFMT_RIGHT, 160);
	m_List.InsertColumn(3, L"Type", LVCFMT_LEFT, 80);
	m_List.InsertColumn(4, L"Details", LVCFMT_LEFT, 300);
	return 0;
}

LRESULT CRegistersView::OnSize(UINT, WPARAM, LPARAM, BOOL&) {
	CRect rc;
	GetClientRect(&rc);
	CSize size;
	m_ToolBar.GetButtonSize(size);
	int height = size.cy + 4;
	m_ToolBar.MoveWindow(0, 0, rc.Width(), height);
	m_List.MoveWindow(0, height, rc.Width(), rc.Height() - height);
	return 0;
}

LRESULT CRegistersView::OnSetFocus(UINT, WPARAM, LPARAM, BOOL&) {
	m_List.SetFocus();
	return 0;
}

LRESULT CRegistersView::OnListKeyDown(int, LPNMHDR pnmh, BOOL&) {
	auto kd = (NMLVKEYDOWN*)pnmh;
	if (kd->wVKey == VK_RETURN && m_List.GetSelectedIndex() >= 0)
		EditValue(m_List.GetSelectedIndex());
	return 0;
}

LRESULT CRegistersView::OnFilter(WORD, WORD wID, HWND, BOOL&) {
	auto bit = 1 << (wID - ID_REGS_8BIT);
	if (m_ToolBar.IsButtonChecked(wID))
		m_Filter |= bit;
	else
		m_Filter &= ~bit;
	Refresh();
	return 0;
}

LRESULT CRegistersView::OnEdit(WORD, WORD, HWND, BOOL&) {
	if (auto row = m_List.GetSelectedIndex(); row >= 0)
		EditValue(row);
	return 0;
}
