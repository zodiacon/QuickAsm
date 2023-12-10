#include "pch.h"
#include "HexViewPanel.h"
#include "wxHexView.h"
#include "..\HexView\HexView.h"

HexViewPanel::HexViewPanel(wxWindow* parent, IHexViewNotify* pNotify) : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
	wxTAB_TRAVERSAL | wxNO_BORDER | wxCLIP_CHILDREN | wxWANTS_CHARS), m_pNotify(pNotify) {
}

bool HexViewPanel::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM* result) {
	auto hdr = (LPNMHDR)lParam;
	if (hdr->hwndFrom == m_pHexView->GetHWND() && m_pNotify) {
		return m_pNotify->OnHexViewNotify(m_pHexView, idCtrl, hdr, result);
	}
	return wxPanel::MSWOnNotify(idCtrl, lParam, result);
}

LRESULT HexViewPanel::MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) {
	switch (msg) {
		case WM_SETFOCUS:
			if (m_pHexView) {
				m_pHexView->SetFocus();
				return 0;
			}

		case WM_CONTEXTMENU:
			wxMenu menu;
			menu.Append(wxID_COPY)->SetBitmap(wxArtProvider::GetIcon(wxART_COPY, wxART_MENU));
			menu.Append(wxID_PASTE);
			PopupMenu(&menu);
			return 0;
	}
	return wxPanel::MSWWindowProc(msg, wParam, lParam);
}

void HexViewPanel::SetHexView(wxHexView* pHexView) {
	m_pHexView = pHexView;
}
