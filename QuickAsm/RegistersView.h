#pragma once

#include "resource.h"
#include "InPlaceEdit.h"
#include <VirtualListView.h>
#include <unordered_set>

class UnicornEngine;
struct RegisterInfo;

// the registers of the emulated CPU, filtered by size, with their values read when shown
class CRegistersView :
	public CWindowImpl<CRegistersView>,
	public CVirtualListView<CRegistersView> {
public:
	DECLARE_WND_CLASS_EX(L"QuickAsmRegistersView", 0, COLOR_WINDOW)

	explicit CRegistersView(UnicornEngine& emulator);

	// the registers of the emulator's mode (after it was opened)
	void Refresh();
	// the values (after they may have changed)
	void UpdateValues();
	// not live while the emulator runs: the values can't be read then (the emulator isn't thread safe)
	void SetLive(bool live);
	// the values to compare with at the next stop (none shown as changed): after the emulator was set up
	void ResetChanges();
	// the values that changed since the last stop are shown in another color: at a stop
	void MarkChanges();

	CString GetColumnText(HWND, int row, int col) const;
	void DoSort(SortInfo const* si);
	bool IsSortable(HWND, int col) const;
	bool OnDoubleClickList(HWND, int row, int col, POINT const&);

	BEGIN_MSG_MAP(CRegistersView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_HANDLER(IDC_REGISTERS, NM_CUSTOMDRAW, OnListCustomDraw)
		NOTIFY_HANDLER(IDC_REGISTERS, LVN_BEGINSCROLL, OnListBeginScroll)
		COMMAND_RANGE_HANDLER(ID_REGS_8BIT, ID_REGS_64BIT, OnFilter)
		COMMAND_RANGE_HANDLER(ID_REGS_X87, ID_REGS_AVX, OnFilter)
		COMMAND_ID_HANDLER(ID_REGS_EDIT, OnEdit)
		CHAIN_MSG_MAP(CVirtualListView<CRegistersView>)
	END_MSG_MAP()

private:
	RegisterInfo const& GetRegister(int row) const;
	CString GetValueText(RegisterInfo const& ri) const;
	CString GetDetailsText(RegisterInfo const& ri) const;
	// the x87, SSE and AVX registers (wider than 64 bits, or not integers)
	static bool IsVectorOrFloat(RegisterInfo const& ri);
	bool IsShown(RegisterInfo const& ri) const;
	// in place, in the value's cell
	bool EditValue(int row);
	// a value typed for a register: decimal (may be negative), hex (0x1F or 1Fh); false if it isn't one or doesn't fit
	static bool ParseValue(CString text, int bits, uint64_t& value);
	// the values of the registers of the emulator's mode, by index into AllRegisters
	std::unordered_map<int, CString> ReadValues() const;

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnListCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnListBeginScroll(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void UpdateFilterButtons();

	UnicornEngine& m_Emulator;
	CToolBarCtrl m_ToolBar;
	CListViewCtrl m_List;
	CInPlaceEdit m_Edit;
	std::vector<int> m_Items;	// indices into AllRegisters
	// the general register sizes shown: bit 0 for 8 bit, 1 for 16, 2 for 32, 3 for 64 (tested against RegisterInfo::Size >> 3),
	// the mode's size when the mode changes; then the x87, SSE and AVX registers (the others are always shown)
	enum : DWORD { FilterX87 = 1 << 4, FilterSSE = 1 << 5, FilterAVX = 1 << 6 };
	DWORD m_Filter{ 1 << 2 };
	DWORD m_FilterMode{ 0 };	// the mode the filter was set for
	bool m_Live{ true };
	std::unordered_map<int, CString> m_Values;	// at the last stop
	std::unordered_set<int> m_Changed;			// since the stop before
	COLORREF m_ItemTextColor{ CLR_DEFAULT };	// of the item being drawn
};
