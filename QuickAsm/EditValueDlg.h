#pragma once

#include "resource.h"

// asks for a value (as text)
class CEditValueDlg : public CDialogImpl<CEditValueDlg> {
public:
	enum { IDD = IDD_EDITVALUE };

	CEditValueDlg(PCWSTR title, PCWSTR value) : m_Title(title), m_Value(value) {}

	CString const& GetValue() const {
		return m_Value;
	}

	BEGIN_MSG_MAP(CEditValueDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnCloseCmd)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		SetWindowText(m_Title);
		SetDlgItemText(IDC_VALUE, m_Value);
		CEdit edit(GetDlgItem(IDC_VALUE));
		edit.SetSel(0, -1);
		edit.SetFocus();
		CenterWindow(GetParent());
		return FALSE;
	}

	LRESULT OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
		if (wID == IDOK)
			GetDlgItemText(IDC_VALUE, m_Value);
		EndDialog(wID);
		return 0;
	}

	CString m_Title;
	CString m_Value;
};
