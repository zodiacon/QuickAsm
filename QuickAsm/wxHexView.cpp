#include "pch.h"
#include "wxHexView.h"

wxHexView::wxHexView(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name) {
    Create(parent, id, pos, size, style, validator, name);
}

bool wxHexView::Create(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxValidator& validator, const wxString& name) {
    if (!CreateControl(parent, id, pos, size, style, validator, name))
        return false;

    m_hWnd = CreateHexView(parent->GetHWND());
    if(!m_hWnd)
		return false;
    
    return true;
}

void wxHexView::SetCurrentPos(uint64_t position) {
    HexView_SetCurPos(m_hWnd, position);
}

size_t wxHexView::GetCurrentPos() const {
    size_t pos = 0;
    HexView_GetCurPos(m_hWnd, &pos);
    return pos;
}

bool wxHexView::InitSharedBuffer(uint8_t* p, size_t size) {
    return HexView_InitBufShared(m_hWnd, p, size);
}

bool wxHexView::SetFont(wxFont const& font) {
    auto hFont = font.GetHFONT();
    if (hFont) {
        HexView_SetFont(m_hWnd, hFont);
        return true;
    }
    return false;
}

bool wxHexView::ScrollTop(size_t pos) {
    return HexView_ScrollTop(m_hWnd, pos);
}

int wxHexView::GetLineLength() const {
    return HexView_GetLineLen(m_hWnd);
}

bool wxHexView::SetLineLength(int length) {
    return HexView_SetLineLen(m_hWnd, length);
}

bool wxHexView::SelectAll() {
    return HexView_SelectAll(m_hWnd);
}

bool wxHexView::SetStyle(HexViewStyles mask, HexViewStyles style) {
    return HexView_SetStyle(m_hWnd, static_cast<DWORD>(mask), static_cast<DWORD>(style));
}

bool wxHexView::SetGrouping(int bytes) {
    return HexView_SetGrouping(m_hWnd, bytes);
}

int wxHexView::GetGrouping() const {
    return HexView_GetGrouping(m_hWnd);
}

bool wxHexView::SetEditMode(EditMode mode) {
    return HexView_SetEditMode(m_hWnd, mode);
}

EditMode wxHexView::GetEditMode() const {
    return (EditMode)HexView_GetEditMode(m_hWnd);
}

bool wxHexView::IsReadOnly() const {
    return HexView_IsReadOnly(m_hWnd);
}

size_t wxHexView::GetSelStart() const {
    size_t start = -1;
    HexView_GetSelStart(m_hWnd, &start);
    return start;
}

size_t wxHexView::GetSelEnd() const {
    size_t end = -1;
    HexView_GetSelEnd(m_hWnd, &end);
    return end;
}

bool wxHexView::SetSel(size_t start, size_t end) {
    return SetSelStart(start) && SetSelEnd(end);
}

bool wxHexView::SetSelStart(size_t start) {
    return HexView_SetSelStart(m_hWnd, start);
}

bool wxHexView::SetSelEnd(size_t end) {
    return HexView_SetSelEnd(m_hWnd, end);
}

std::pair<size_t, size_t> wxHexView::GetSelection() const {
    return std::make_pair(GetSelStart(), GetSelEnd());
}

size_t wxHexView::GetSelSize() const {
    size_t size = 0;
    HexView_GetSelSize(m_hWnd, &size);
    return size;
}

bool wxHexView::SetAddressOffset(size_t offset) {
    HexView_SetAddressOffset(m_hWnd, offset);
    return true;
}

bool wxHexView::SetColor(ColorType type, wxColour const& color) {
    return HexView_SetColor(m_hWnd, type, RGB(color.Red(), color.Green(), color.Blue()));
}

wxColor wxHexView::GetColor(ColorType type) const {
    auto color = HexView_GetColor(m_hWnd, type);
    return wxColor(GetRValue(color), GetGValue(color), GetBValue(color));
}

bool wxHexView::SetPadding(int left, int right) {
    HexView_SetPadding(m_hWnd, left, right);
    return true;
}

void wxHexView::SetContextMenu(wxMenu* menu) {
    HexView_SetContextMenu(m_hWnd, menu->GetHMenu());
}

bool wxHexView::Copy() const {
    return HexView_Copy(m_hWnd);
}

bool wxHexView::CanCopy() const {
    return GetSelSize() > 0;
}

bool wxHexView::Cut() {
    return HexView_Cut(m_hWnd);
}

bool wxHexView::Paste() {
    return HexView_Paste(m_hWnd);
}

bool wxHexView::CanPaste() const {
    return ::IsClipboardFormatAvailable(CF_TEXT);
}

bool wxHexView::Delete() {
    return HexView_Delete(m_hWnd);
}

bool wxHexView::CanDelete() const {
    return GetEditMode() != EditMode::ReadOnly && GetSelSize() > 0;
}

bool wxHexView::Undo() {
    return HexView_Undo(m_hWnd);
}

bool wxHexView::Redo() {
    return HexView_Redo(m_hWnd);
}

bool wxHexView::CanUndo() const {
    return HexView_CanUndo(m_hWnd);
}

bool wxHexView::CanRedo() const {
    return HexView_CanRedo(m_hWnd);
}

bool wxHexView::OpenFile(PCWSTR path, OpenFileMode mode) {
    return HexView_OpenFile(m_hWnd, path, mode);
}



