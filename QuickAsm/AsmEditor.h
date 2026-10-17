#pragma once

#include "ScintillaCtrl.h"

// a Scintilla control set up for x86 assembly: used for the source and the disassembly
class CAsmEditor : public CScintillaCtrl {
public:
	enum {
		BreakpointMarker = 1,
		CurrentLineMarker = 2,
		DisabledBreakpointMarker = 3,
		ConditionalBreakpointMarker = 4,
		DisabledConditionalBreakpointMarker = 5,
		CurrentLineIndicator = 2,
		ErrorIndicator = 3,
	};

	HWND Create(HWND hParent, UINT id, bool readOnly = false);
	// the colors of the current theme (light or dark), and the font
	void ApplyTheme();
	// the font of the text (applied with the theme)
	void SetEditorFont(std::string const& face, int sizePoints);
	// the instructions, registers, directives and the source's labels that start with the word at the caret;
	// typing: only after two characters of a word (outside comments), explicit: also for a shorter (or no) word
	void AutoComplete(bool explicitRequest);
	// the breakpoint marker of a line: none, enabled or disabled; with a condition (or a hit count) or not
	void SetBreakpointMarker(int line, bool exists, bool enabled = true, bool conditional = false);
	void DeleteBreakpointMarkers();
	// the lines with errors are underlined (none: the marks go)
	void MarkErrorLines(std::vector<int> const& lines);

	// selects the next (or previous) occurrence, wrapping around; false if there's none
	bool Find(std::string const& text, bool down, bool matchCase, bool wholeWord);
	// replaces the selection if it's an occurrence, then finds the next; false if there's no next
	bool Replace(std::string const& text, std::string const& replacement, bool matchCase, bool wholeWord);
	// (one undo step) the number replaced
	int ReplaceAll(std::string const& text, std::string const& replacement, bool matchCase, bool wholeWord);

private:
	void SetSearchFlags(bool matchCase, bool wholeWord);
	// the labels defined in the text ("name:" at a line's start)
	std::vector<std::string> GetLabels() const;
	Sci_Position SearchInRange(std::string const& text, Sci_Position from, Sci_Position to);

public:
	// the line where execution stopped (-1: none), made visible
	void SetCurrentLine(int line);

private:
	int m_CurrentLine{ -1 };
	std::string m_FontFace{ "Consolas" };
	int m_FontSize{ 12 };
};
