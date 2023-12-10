#pragma once

#include "..\HexView\HexView.h"
#include "../../Github/wxWidgets/include/wx/event.h"

enum class HexViewStyles {
	FormatHex = HVS_FORMAT_HEX,
	FormatDec = HVS_FORMAT_DEC,
	FormatOct = HVS_FORMAT_OCT,
	FormatBin = HVS_FORMAT_BIN,

	AddressHex = HVS_ADDR_HEX,
	AddressDec = HVS_ADDR_DEC,
	AddressVisible = HVS_ADDR_VISIBLE,
	AddressResizing = HVS_ADDR_RESIZING,
	AddressEndColon = HVS_ADDR_ENDCOLON,
	AddressMidColon = HVS_ADDR_MIDCOLON,
	AddressInvisible = HVS_ADDR_INVISIBLE,

	EndianLittle = HVS_ENDIAN_LITTLE,
	EndianBig = HVS_ENDIAN_BIG,

	AsciiVisible = HVS_ASCII_VISIBLE,
	AsciiShowCtrls = HVS_ASCII_SHOWCTRLS,
	AsciiShowExtended = HVS_ASCII_SHOWEXTD,
	AsciiInvisible = HVS_ASCII_INVISIBLE,
};
DEFINE_ENUM_FLAG_OPERATORS(HexViewStyles);

enum class EditMode {
	ReadOnly = HVMODE_READONLY,
	Insert = HVMODE_INSERT,
	Overwrite = HVMODE_OVERWRITE,
	QuickSave = HVMODE_QUICKSAVE,
};

enum class OpenFileMode {
	Default = HVOF_DEFAULT,
	ReadOnly = HVOF_READONLY,
	QuickLoad = HVOF_QUICKLOAD,
	QuickSave = HVOF_QUICKSAVE,
	AutoQuickLoad = HVOF_AUTOQUICKLOAD,
};

enum class ColorType {
	Background =	HVC_BACKGROUND,
	Selection =		HVC_SELECTION,
	Selection2 =	HVC_SELECTION2,
	Address =		HVC_ADDRESS,
	Hexodd =		HVC_HEXODD,
	Hexoddsel =		HVC_HEXODDSEL,
	Hexoddsel2 =	HVC_HEXODDSEL2,
	Hexeven =		HVC_HEXEVEN,
	Hexevensel =	HVC_HEXEVENSEL,
	Hexevensel2 =	HVC_HEXEVENSEL2,
	Ascii =			HVC_ASCII,
	Asciisel =		HVC_ASCIISEL,
	Asciisel2 =		HVC_ASCIISEL2,
	Modify =		HVC_MODIFY,
	Modifysel =		HVC_MODIFYSEL,
	Modifysel2 =	HVC_MODIFYSEL2,
	Bookmark_fg	=	HVC_BOOKMARK_FG,
	Bookmark_bg	=	HVC_BOOKMARK_BG,
	Booksel =		HVC_BOOKSEL,
	Resizebar =		HVC_RESIZEBAR,
	Selection3 =	HVC_SELECTION3,
	Selection4 =	HVC_SELECTION4,
	Matched =		HVC_MATCHED,
	Matchedsel =	HVC_MATCHEDSEL,
	Matchedsel2	=	HVC_MATCHEDSEL2,
};

class wxHexView : public wxControl {
public:
	wxHexView(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = L"wxHexView");
	bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0, const wxValidator& validator = wxDefaultValidator, const wxString& name = L"wxHexView");

	void SetCurrentPos(uint64_t position);
	size_t GetCurrentPos() const;
	bool InitSharedBuffer(uint8_t* p, size_t size);
	bool SetFont(wxFont const& font) override;
	bool ScrollTop(size_t pos);
	int GetLineLength() const;
	bool SetLineLength(int length);
	bool SelectAll();
	bool SetStyle(HexViewStyles mask, HexViewStyles style);
	bool SetGrouping(int bytes);
	int GetGrouping() const;
	bool SetEditMode(EditMode mode);
	EditMode GetEditMode() const;
	bool IsReadOnly() const;
	size_t GetSelStart() const;
	size_t GetSelEnd() const;
	bool SetSel(size_t start, size_t end);
	bool SetSelStart(size_t start);
	bool SetSelEnd(size_t end);
	std::pair<size_t, size_t> GetSelection() const;
	size_t GetSelSize() const;
	bool SetAddressOffset(size_t offset);
	bool SetColor(ColorType type, wxColour const& color);
	wxColor GetColor(ColorType type) const;
	bool SetPadding(int left, int right);
	void SetContextMenu(wxMenu* menu);

	bool Copy() const;
	bool CanCopy() const;
	bool Cut();
	bool Paste();
	bool CanPaste() const;
	bool Delete();
	bool CanDelete() const;
	bool Undo();
	bool Redo();
	bool CanUndo() const;
	bool CanRedo() const;

	bool OpenFile(PCWSTR path, OpenFileMode mode = OpenFileMode::Default);

private:
};

