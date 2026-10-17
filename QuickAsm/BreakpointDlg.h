#pragma once

#include "resource.h"
#include "Expression.h"

// a breakpoint's condition and hit count; OK only with a valid condition
class CBreakpointDlg : public CDialogImpl<CBreakpointDlg> {
public:
	enum { IDD = IDD_BREAKPOINT };

	CString Title;
	std::string Condition;
	std::unique_ptr<Expression> Expr;	// (the condition's; null if there's none)
	uint32_t HitTarget{ 0 };

	BEGIN_MSG_MAP(CBreakpointDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		SetWindowText(Title);
		SetDlgItemText(IDC_CONDITION, CString(Condition.c_str()));
		if (HitTarget > 1)
			SetDlgItemInt(IDC_HITCOUNT, HitTarget, FALSE);
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnOK(WORD, WORD, HWND, BOOL&) {
		CString text;
		GetDlgItemText(IDC_CONDITION, text);
		text.Trim();
		std::string condition(CStringA(text).GetString());
		std::unique_ptr<Expression> expr;
		if (!condition.empty()) {
			std::string error;
			expr = Expression::Parse(condition, error);
			if (!expr) {
				AtlMessageBox(m_hWnd, (PCWSTR)CString(("Invalid condition: " + error).c_str()), IDR_MAINFRAME, MB_ICONWARNING);
				GotoDlgCtrl(GetDlgItem(IDC_CONDITION));
				return 0;
			}
		}

		GetDlgItemText(IDC_HITCOUNT, text);
		text.Trim();
		uint32_t hits = 0;
		if (!text.IsEmpty()) {
			PWSTR end;
			auto value = ::wcstoul(text, &end, 10);
			if (*end || text[0] == L'-') {
				AtlMessageBox(m_hWnd, L"The hit count must be a number", IDR_MAINFRAME, MB_ICONWARNING);
				GotoDlgCtrl(GetDlgItem(IDC_HITCOUNT));
				return 0;
			}
			hits = value;
		}

		Condition = std::move(condition);
		Expr = std::move(expr);
		HitTarget = hits;
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnCancel(WORD, WORD, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}
};
