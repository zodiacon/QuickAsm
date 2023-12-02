#pragma once

class LocalArtProvider : public wxArtProvider {
public:
	wxBitmapBundle CreateBitmapBundle(const wxArtID& id, const wxArtClient& client,	const wxSize& size) override;
};

