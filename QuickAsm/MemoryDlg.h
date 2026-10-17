#pragma once

#include "resource.h"
#include "Helpers.h"

// the emulator's memory: where it starts, its size and the stack's (at its top); OK only with valid values
class CMemoryDlg : public CDialogImpl<CMemoryDlg> {
public:
	enum { IDD = IDD_MEMORY };

	uint64_t Base{ 0 };
	uint32_t SizeKB{ 64 };
	uint32_t StackKB{ 4 };

	// the reason the values can't be used (empty if they can)
	static CString Validate(uint64_t base, uint32_t sizeKB, uint32_t stackKB) {
		if (base % 0x1000)
			return L"The base address must be a multiple of 4 KB (0x1000).";
		if (sizeKB < 8 || sizeKB > 256 * 1024 || sizeKB % 4)
			return L"The size must be a multiple of 4 KB, from 8 KB to 256 MB.";
		if (base + sizeKB * 1024ull > 0xFFFFF000)
			return L"The memory must end below 4 GB (the 32-bit address space).";
		if (stackKB < 1 || stackKB > sizeKB / 2)
			return L"The stack size must be at least 1 KB, and at most half the memory.";
		return L"";
	}

	BEGIN_MSG_MAP(CMemoryDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL&) {
		CString text;
		text.Format(L"0x%llX", Base);
		SetDlgItemText(IDC_MEMBASE, text);
		SetDlgItemInt(IDC_MEMSIZE, SizeKB, FALSE);
		SetDlgItemInt(IDC_STACKSIZE, StackKB, FALSE);
		CenterWindow(GetParent());
		return TRUE;
	}

	LRESULT OnOK(WORD, WORD, HWND, BOOL&) {
		CString text;
		GetDlgItemText(IDC_MEMBASE, text);
		uint64_t base;
		BOOL sizeOk, stackOk;
		auto size = GetDlgItemInt(IDC_MEMSIZE, &sizeOk, FALSE);
		auto stack = GetDlgItemInt(IDC_STACKSIZE, &stackOk, FALSE);
		CString error;
		if (!Helpers::ParseNumber(text, base))
			error = L"The base address must be a number (decimal, or hex with 0x).";
		else if (!sizeOk || !stackOk)
			error = L"The sizes must be numbers.";
		else
			error = Validate(base, size, stack);
		if (!error.IsEmpty()) {
			AtlMessageBox(m_hWnd, (PCWSTR)error, IDR_MAINFRAME, MB_ICONWARNING);
			return 0;
		}

		Base = base;
		SizeKB = size;
		StackKB = stack;
		EndDialog(IDOK);
		return 0;
	}

	LRESULT OnCancel(WORD, WORD, HWND, BOOL&) {
		EndDialog(IDCANCEL);
		return 0;
	}
};
