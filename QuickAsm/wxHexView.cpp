#include "pch.h"
#include "wxHexView.h"
#include "..\HexView\HexView.h"

wxHexView::wxHexView(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) {
    Create(parent, id, pos, size, style, name);
}

bool wxHexView::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) {
  //  style |= wxVSCROLL | wxHSCROLL;
  //  if (!wxControl::Create(parent, id, pos, size, style | wxWANTS_CHARS | wxCLIP_CHILDREN, wxDefaultValidator, name))
		//return false;

    AssociateHandle(CreateHexView(parent->GetHWND()));
    if(!m_hWnd)
		return false;

    SetInitialSize(size);
	SetBackgroundStyle(wxBG_STYLE_PAINT);
    return true;
}
