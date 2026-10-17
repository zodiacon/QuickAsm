#pragma once

#include "resource.h"
#include <VirtualListView.h>

struct StackEntry {
	uint64_t Address;
	uint64_t Value;
	CString Notes;		// the registers pointing at it, what the value is (a return address...)
};

// the stack from its top (the stack pointer) up, an entry of the mode's size in each row
class CStackView :
	public CWindowImpl<CStackView>,
	public CVirtualListView<CStackView> {
public:
	DECLARE_WND_CLASS_EX(L"QuickAsmStackView", 0, COLOR_WINDOW)

	// entrySize: 2, 4 or 8 bytes (none while the code runs)
	void SetEntries(std::vector<StackEntry> entries, int entrySize);

	CString GetColumnText(HWND, int row, int col) const;

	BEGIN_MSG_MAP(CStackView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		CHAIN_MSG_MAP(CVirtualListView<CStackView>)
	END_MSG_MAP()

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

	CListViewCtrl m_List;
	std::vector<StackEntry> m_Entries;
	int m_EntrySize{ 4 };
};
