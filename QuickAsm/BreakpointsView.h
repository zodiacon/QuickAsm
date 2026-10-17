#pragma once

#include "resource.h"
#include "Interfaces.h"
#include <VirtualListView.h>

// the breakpoints: enabled or not (the check box), deleted, given a condition (double click on it),
// or shown in the disassembly (double click elsewhere)
class CBreakpointsView :
	public CWindowImpl<CBreakpointsView>,
	public CVirtualListView<CBreakpointsView> {
public:
	DECLARE_WND_CLASS_EX(L"QuickAsmBreakpointsView", 0, COLOR_WINDOW)

	explicit CBreakpointsView(IBreakpointsHost& host) : m_Host(host) {}

	// the breakpoints (after they changed)
	void Refresh();

	CString GetColumnText(HWND, int row, int col) const;
	ListViewRowCheck IsRowChecked(HWND, int row) const;
	bool OnDoubleClickList(HWND, int row, int col, POINT const&);
	void OnListViewClick(HWND, int row, int col, POINT const& pt);

	BEGIN_MSG_MAP(CBreakpointsView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnListKeyDown)
		NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnItemChanged)
		COMMAND_ID_HANDLER(ID_BREAKPOINTS_DELETE, OnDelete)
		COMMAND_ID_HANDLER(ID_BREAKPOINTS_CONDITION, OnCondition)
		COMMAND_ID_HANDLER(ID_BREAKPOINTS_DELETEALL, OnDeleteAll)
		COMMAND_ID_HANDLER(ID_BREAKPOINTS_ENABLEALL, OnEnableAll)
		COMMAND_ID_HANDLER(ID_BREAKPOINTS_DISABLEALL, OnEnableAll)
		CHAIN_MSG_MAP(CVirtualListView<CBreakpointsView>)
	END_MSG_MAP()

private:
	void ToggleEnabled(int row);
	void UpdateToolBar();

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnListKeyDown(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& bHandled);
	LRESULT OnDelete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCondition(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDeleteAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnableAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	IBreakpointsHost& m_Host;
	CToolBarCtrl m_ToolBar;
	CListViewCtrl m_List;
	std::vector<BreakpointInfo> m_Items;
};
