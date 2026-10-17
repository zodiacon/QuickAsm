#include "pch.h"
#include "AboutDlg.h"
#include <VersionResourceHelper.h>

LRESULT CAboutDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
	SetDialogIcon(IDR_MAINFRAME);
	CenterWindow(GetParent());

	VersionResourceHelper vh;
	SetDlgItemText(IDC_VERSION, vh.GetValue(L"ProductName") + L" v" + vh.GetValue(L"ProductVersion"));
	SetDlgItemText(IDC_COPYRIGHT, vh.GetValue(L"LegalCopyright"));
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD, WORD wID, HWND, BOOL&) {
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnClickSyslink(int, LPNMHDR hdr, BOOL&) {
	if (hdr->idFrom == IDC_LINK)
		::ShellExecute(nullptr, L"open", L"https://github.com/zodiacon/QuickAsm", nullptr, nullptr, SW_SHOWDEFAULT);
	return 0;
}
