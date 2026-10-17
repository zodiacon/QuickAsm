#pragma once

// an edit box over a cell (of a list view): Enter accepts the text (if the handler takes it), Escape cancels,
// losing the focus accepts it if it can
class CInPlaceEdit : public CWindowImpl<CInPlaceEdit, CEdit> {
public:
	DECLARE_WND_SUPERCLASS(L"QuickAsmInPlaceEdit", CEdit::GetWndClassName())

	// false: the text isn't valid (Enter keeps editing)
	using AcceptHandler = std::function<bool(CString const& text)>;

	bool Start(HWND hParent, CRect const& rc, PCWSTR text, DWORD style, AcceptHandler accept) {
		Cancel();
		m_Accept = std::move(accept);
		m_Ending = false;
		if (!Create(hParent, const_cast<CRect&>(rc), text, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | style))
			return false;
		SetFont(CWindow(hParent).GetFont());
		SetSel(0, -1);
		SetFocus();
		return true;
	}

	bool IsEditing() const {
		return IsWindow();
	}

	// ends without accepting the text
	void Cancel() {
		End(true);
	}

	BEGIN_MSG_MAP(CInPlaceEdit)
		MESSAGE_HANDLER(WM_GETDLGCODE, OnGetDlgCode)
		MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
		MESSAGE_HANDLER(WM_CHAR, OnChar)
		MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
	END_MSG_MAP()

private:
	// restoreFocus: to the parent (not when the focus went elsewhere)
	void End(bool restoreFocus) {
		if (!IsWindow() || m_Ending)
			return;
		m_Ending = true;	// (no accepting when the focus leaves now)
		if (restoreFocus && GetFocus() == m_hWnd)
			GetParent().SetFocus();
		DestroyWindow();
	}

	bool Accept() {
		CString text;
		GetWindowText(text);
		return m_Accept && m_Accept(text);
	}

	LRESULT OnGetDlgCode(UINT, WPARAM, LPARAM, BOOL&) {
		return DLGC_WANTALLKEYS;
	}

	LRESULT OnKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
		switch (wParam) {
			case VK_RETURN:
				if (Accept())
					End(true);
				else
					::MessageBeep(MB_ICONWARNING);
				return 0;
			case VK_ESCAPE:
				End(true);
				return 0;
		}
		bHandled = FALSE;
		return 0;
	}

	LRESULT OnChar(UINT, WPARAM wParam, LPARAM, BOOL& bHandled) {
		// (no beep for these)
		bHandled = wParam == VK_RETURN || wParam == VK_ESCAPE;
		return 0;
	}

	LRESULT OnKillFocus(UINT, WPARAM, LPARAM, BOOL& bHandled) {
		bHandled = FALSE;
		if (!m_Ending) {
			Accept();
			End(false);
		}
		return 0;
	}

	AcceptHandler m_Accept;
	bool m_Ending{ false };
};
