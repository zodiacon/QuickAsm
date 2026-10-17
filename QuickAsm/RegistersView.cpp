#include "pch.h"
#include "RegistersView.h"
#include "RegisterInfo.h"
#include "Helpers.h"
#include <ToolbarHelper.h>
#include <SortHelper.h>
#include <WTLHelper.h>
#include <charconv>
#include <cmath>
#include <limits>

CRegistersView::CRegistersView(UnicornEngine& emulator) : m_Emulator(emulator) {
}

void CRegistersView::Refresh() {
	// (the rows change)
	m_Edit.Cancel();
	m_Items.clear();
	if (m_Emulator.IsOpen()) {
		auto mode = (DWORD)m_Emulator.GetMode();
		if (mode != m_FilterMode) {
			// the mode's general registers (CS_MODE_16/32/64 are 2, 4 and 8: the sizes in bytes); the x87/SSE/AVX choice stays
			m_Filter = mode | (m_Filter & (FilterX87 | FilterSSE | FilterAVX));
			m_FilterMode = mode;
			UpdateFilterButtons();
		}
		for (int i = 0; i < (int)AllRegisters.size(); i++)
			if (((DWORD)AllRegisters[i].Category & mode) && IsShown(AllRegisters[i]))
				m_Items.push_back(i);
	}
	Sort(m_List);
	m_List.SetItemCountEx((int)m_Items.size(), LVSICF_NOSCROLL);
	m_List.RedrawItems(0, m_List.GetItemCount());
}

void CRegistersView::UpdateValues() {
	m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
}

void CRegistersView::SetLive(bool live) {
	if (!live)
		m_Edit.Cancel();
	m_Live = live;
	UpdateValues();
}

std::unordered_map<int, CString> CRegistersView::ReadValues() const {
	std::unordered_map<int, CString> values;
	if (!m_Emulator.IsOpen())
		return values;

	auto mode = (DWORD)m_Emulator.GetMode();
	for (int i = 0; i < (int)AllRegisters.size(); i++)
		if ((DWORD)AllRegisters[i].Category & mode)
			values[i] = GetValueText(AllRegisters[i]);
	return values;
}

void CRegistersView::ResetChanges() {
	m_Values = ReadValues();
	m_Changed.clear();
	UpdateValues();
}

void CRegistersView::MarkChanges() {
	auto values = ReadValues();
	m_Changed.clear();
	for (auto& [index, value] : values)
		if (auto it = m_Values.find(index); it != m_Values.end() && it->second != value)
			m_Changed.insert(index);
	m_Values = std::move(values);
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

bool CRegistersView::IsVectorOrFloat(RegisterInfo const& ri) {
	return (ri.Category & (RegisterType::FloatingPoint | RegisterType::SSE | RegisterType::AVX)) != RegisterType::None;
}

bool CRegistersView::IsShown(RegisterInfo const& ri) const {
	if ((ri.Category & RegisterType::AVX) != RegisterType::None)
		return m_Filter & FilterAVX;
	if ((ri.Category & RegisterType::SSE) != RegisterType::None)
		return m_Filter & FilterSSE;
	if ((ri.Category & RegisterType::FloatingPoint) != RegisterType::None)
		return m_Filter & FilterX87;
	if ((ri.Category & RegisterType::General) != RegisterType::None)
		return m_Filter & (ri.Size >> 3);
	return true;
}

namespace {
	// an x87 register: a 64-bit mantissa (with its integer bit) and a sign and 15-bit exponent
	struct Float80 {
		uint64_t Mantissa;
		uint16_t SignExponent;
	};

	double ToDouble(Float80 const& value) {
		auto exponent = value.SignExponent & 0x7FFF;
		double result;
		if (exponent == 0x7FFF)
			result = (value.Mantissa << 1) ? std::numeric_limits<double>::quiet_NaN() : std::numeric_limits<double>::infinity();
		else
			result = std::ldexp((double)value.Mantissa, exponent - 16383 - 63);
		return (value.SignExponent & 0x8000) ? -result : result;
	}
}

CString CRegistersView::GetValueText(RegisterInfo const& ri) const {
	CString text;
	if (ri.Size > 64 && IsVectorOrFloat(ri)) {
		// (as wide as the widest, YMM)
		uint64_t value[4]{};
		if (!m_Emulator.ReadReg(ri.Id, value))
			return L"(not available)";
		if (ri.Size == 80) {
			auto& f = *(Float80*)value;
			text.Format(L"0x%04X%016llX", f.SignExponent, f.Mantissa);
			return text;
		}
		// the qwords, the most significant first
		text = L"0x";
		for (int i = ri.Size / 64 - 1; i >= 0; i--)
			text.AppendFormat(L"%016llX", value[i]);
		return text;
	}
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
	if (IsVectorOrFloat(ri) && ri.Size > 64) {
		uint64_t value[4]{};
		if (!m_Emulator.ReadReg(ri.Id, value))
			return L"";
		CString text;
		if (ri.Size == 80) {
			text.Format(L"%.17g", ToDouble(*(Float80*)value));
			return text;
		}
		// as floats, and as doubles
		auto floats = (float const*)value;
		auto doubles = (double const*)value;
		text = L"float:";
		for (int i = ri.Size / 32 - 1; i >= 0; i--)
			text.AppendFormat(L" %g", floats[i]);
		text += L"  double:";
		for (int i = ri.Size / 64 - 1; i >= 0; i--)
			text.AppendFormat(L" %g", doubles[i]);
		return text;
	}
	if (ri.Id == x86Register::FPSW) {
		auto sw = m_Emulator.ReadReg<uint16_t>(ri.Id);
		CString text;
		text.Format(L"TOP=%d C3=%d C2=%d C1=%d C0=%d", (sw >> 11) & 7, (sw >> 14) & 1, (sw >> 10) & 1, (sw >> 9) & 1, (sw >> 8) & 1);
		return text;
	}

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
	if ((ri.Category & (RegisterType::General | RegisterType::Segment)) == RegisterType::None || IsVectorOrFloat(ri)) {
		::MessageBeep(MB_ICONWARNING);
		return false;
	}

	m_List.EnsureVisible(row, FALSE);
	m_List.SelectItem(row);
	CRect rc;
	m_List.GetSubItemRect(row, 2, LVIR_LABEL, &rc);
	auto id = ri.Id;
	auto bits = ri.Size;
	return m_Edit.Start(m_List, rc, GetValueText(ri), ES_RIGHT, [this, id, bits](CString const& text) {
		uint64_t value;
		if (!ParseValue(text, bits, value))
			return false;
		m_Emulator.WriteReg(id, value);
		// (an edit isn't a change made by the code: the next stop compares with it)
		m_Values = ReadValues();
		// (the registers that overlap it change too)
		UpdateValues();
		return true;
		});
}

bool CRegistersView::ParseValue(CString text, int bits, uint64_t& value) {
	text.Trim();
	std::string s(CStringA(text).MakeLower().GetString());
	bool negative = !s.empty() && s[0] == '-';
	std::string_view digits(s);
	if (negative)
		digits.remove_prefix(1);
	int base = 10;
	if (digits.starts_with("0x"))
		base = 16, digits.remove_prefix(2);
	else if (digits.ends_with('h'))
		base = 16, digits.remove_suffix(1);
	auto [end, ec] = std::from_chars(digits.data(), digits.data() + digits.size(), value, base);
	if (digits.empty() || ec != std::errc() || end != digits.data() + digits.size())
		return false;

	if (negative) {
		// (down to the smallest signed value of the size)
		if (value > (1ull << (bits - 1)))
			return false;
		value = 0 - value;
	}
	if (bits < 64) {
		auto mask = (1ull << bits) - 1;
		if (!negative && value > mask)
			return false;
		value &= mask;
	}
	return true;
}

LRESULT CRegistersView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	ToolBarButtonInfo const buttons[] = {
		{ ID_REGS_8BIT, IDI_8BIT, BTNS_CHECK },
		{ ID_REGS_16BIT, IDI_16BIT, BTNS_CHECK },
		{ ID_REGS_32BIT, IDI_32BIT, BTNS_CHECK },
		{ ID_REGS_64BIT, IDI_64BIT, BTNS_CHECK },
		{ 0 },
		{ ID_REGS_X87, 0, BTNS_CHECK | BTNS_AUTOSIZE, L"x87" },
		{ ID_REGS_SSE, 0, BTNS_CHECK | BTNS_AUTOSIZE, L"SSE" },
		{ ID_REGS_AVX, 0, BTNS_CHECK | BTNS_AUTOSIZE, L"AVX" },
		{ 0 },
		{ ID_REGS_EDIT, IDI_EDIT },
	};
	m_ToolBar.Attach(ToolbarHelper::CreateAndInitToolBar(m_hWnd, buttons, _countof(buttons), 16));
	UpdateFilterButtons();

	m_List.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
		0, IDC_REGISTERS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	m_List.InsertColumn(0, L"Name", LVCFMT_LEFT, 70);
	m_List.InsertColumn(1, L"Size", LVCFMT_RIGHT, 60);
	// (wide enough for 128 bits; wider values show in full in the tooltip)
	m_List.InsertColumn(2, L"Value", LVCFMT_RIGHT, 270);
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
	if ((kd->wVKey == VK_RETURN || kd->wVKey == VK_F2) && m_List.GetSelectedIndex() >= 0)
		EditValue(m_List.GetSelectedIndex());
	return 0;
}

LRESULT CRegistersView::OnListCustomDraw(int, LPNMHDR pnmh, BOOL&) {
	auto cd = (NMLVCUSTOMDRAW*)pnmh;
	switch (cd->nmcd.dwDrawStage) {
		case CDDS_PREPAINT:
			return CDRF_NOTIFYITEMDRAW;

		case CDDS_ITEMPREPAINT:
			m_ItemTextColor = cd->clrText;
			return CDRF_NOTIFYSUBITEMDRAW;

		case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
		{
			// the value and its details, of a register that changed
			auto row = (int)cd->nmcd.dwItemSpec;
			auto changed = m_Live && (cd->iSubItem == 2 || cd->iSubItem == 4) && row < (int)m_Items.size() && m_Changed.contains(m_Items[row]);
			cd->clrText = changed ? (WTLHelper::IsDarkMode() ? RGB(255, 96, 96) : RGB(220, 0, 0)) : m_ItemTextColor;
			return CDRF_NEWFONT;
		}
	}
	return CDRF_DODEFAULT;
}

LRESULT CRegistersView::OnListBeginScroll(int, LPNMHDR, BOOL&) {
	// (the edit box would stay where the cell was)
	m_Edit.Cancel();
	return 0;
}

void CRegistersView::UpdateFilterButtons() {
	for (int i = 0; i < 4; i++)
		m_ToolBar.CheckButton(ID_REGS_8BIT + i, (m_Filter & (1 << i)) != 0);
	for (int i = 0; i < 3; i++)
		m_ToolBar.CheckButton(ID_REGS_X87 + i, (m_Filter & (FilterX87 << i)) != 0);
}

LRESULT CRegistersView::OnFilter(WORD, WORD wID, HWND, BOOL&) {
	auto bit = wID >= ID_REGS_X87 ? FilterX87 << (wID - ID_REGS_X87) : 1 << (wID - ID_REGS_8BIT);
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
