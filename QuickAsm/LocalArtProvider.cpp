#include "pch.h"
#include "LocalArtProvider.h"

wxBitmapBundle LocalArtProvider::CreateBitmapBundle(const wxArtID& id, const wxArtClient& client, const wxSize& size) {
    static const struct {
        wxArtID id;
        wxString icon;
    } iconsMap[] = {
        { wxART_FILE_OPEN, "OPEN" },
        { wxART_UNDO, "UNDO" },
        { wxART_REDO, "REDO" },
        { wxART_COPY, "COPY" },
        { wxART_CUT, "CUT" },
        { wxART_FILE_SAVE, "SAVE" },
        { wxART_FILE_SAVE_AS, "SAVEAS" },
        { wxART_PASTE, "PASTE" },
        { wxART_EDIT, "EDIT" },
    };

    for (auto& icon : iconsMap) {
        if (id == icon.id) {
            wxIconBundle icons(icon.icon, nullptr);
            return wxBitmapBundle::FromIconBundle(icons);
        }
    }

    return wxBitmapBundle::FromIconBundle(wxIconBundle(id, nullptr));
}

