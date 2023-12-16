#include "pch.h"
#include "LocalArtProvider.h"
#include "resource.h"

wxBitmapBundle LocalArtProvider::CreateBitmapBundle(const wxArtID& id, const wxArtClient& client, const wxSize& size) {
    static const struct {
        wxArtID id;
        wxString icon;
    } iconsMap[] = {
        { wxART_FILE_OPEN, L"OPEN" },
        { wxART_UNDO, L"UNDO" },
        { wxART_REDO, L"REDO" },
        { wxART_COPY, L"COPY" },
        { wxART_CUT, L"CUT" },
        { wxART_FILE_SAVE, L"SAVE" },
        { wxART_FILE_SAVE_AS, L"SAVEAS" },
        { wxART_PASTE, L"PASTE" },
        { wxART_EDIT, L"EDIT" },
        { "RESTART", wxString::Format("#%d", IDI_RESTART) },
    };

    for (auto& icon : iconsMap) {
        if (id == icon.id) {
            wxIconBundle icons(icon.icon, nullptr);
            return wxBitmapBundle::FromIconBundle(icons);
        }
    }

    return wxBitmapBundle::FromIconBundle(wxIconBundle(id, nullptr));
}

