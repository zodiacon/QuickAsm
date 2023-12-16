#pragma once

class wxHexView;

struct IHexViewNotify abstract {
	virtual bool OnHexViewNotify(wxHexView* pHexView, int idCtrl, LPNMHDR, WXLPARAM* result) = 0;
};

class HexViewPanel : public wxPanel {
	wxDECLARE_DYNAMIC_CLASS(HexViewPanel);
public:
	HexViewPanel() = default;
	HexViewPanel(wxWindow* parent, IHexViewNotify* pNotify = nullptr);
	bool MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM* result) override;
	LRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) override;

	void SetHexView(wxHexView* pHexView);

private:
	wxHexView* m_pHexView{ nullptr };
	IHexViewNotify* m_pNotify{ nullptr };
};
