#pragma once

#include "resource.h"
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
		COMMAND_RANGE_HANDLER(ID_REGS_8BIT, ID_REGS_64BIT, OnFilter)
		COMMAND_ID_HANDLER(ID_REGS_EDIT, OnEdit)
		CHAIN_MSG_MAP(CVirtualListView<CRegistersView>)
	END_MSG_MAP()

private:
	RegisterInfo const& GetRegister(int row) const;
	CString GetValueText(RegisterInfo const& ri) const;
	CString GetDetailsText(RegisterInfo const& ri) const;
	bool EditValue(int row);
	// the values of the registers of the emulator's mode, by index into AllRegisters
	std::unordered_map<int, CString> ReadValues() const;

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnListCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnFilter(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEdit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	UnicornEngine& m_Emulator;
	CToolBarCtrl m_ToolBar;
	CListViewCtrl m_List;
	std::vector<int> m_Items;	// indices into AllRegisters
	// the register sizes shown: bit 0 for 8 bit, 1 for 16, 2 for 32, 3 for 64 (tested against RegisterInfo::Size >> 3)
	DWORD m_Filter{ 1 << 2 };
	bool m_Live{ true };
	std::unordered_map<int, CString> m_Values;	// at the last stop
	std::unordered_set<int> m_Changed;			// since the stop before
	COLORREF m_ItemTextColor{ CLR_DEFAULT };	// of the item being drawn
};
