#pragma once

#include "ScintillaCtrl.h"

// a Scintilla control set up for x86 assembly: used for the source and the disassembly
class CAsmEditor : public CScintillaCtrl {
public:
	enum {
		BreakpointMarker = 1,
		CurrentLineMarker = 2,
		DisabledBreakpointMarker = 3,
		CurrentLineIndicator = 2,
		ErrorIndicator = 3,
	};

	HWND Create(HWND hParent, UINT id, bool readOnly = false);
	// the colors of the current theme (light or dark)
	void ApplyTheme();
	// the breakpoint marker of a line: none, enabled or disabled
	void SetBreakpointMarker(int line, bool exists, bool enabled = true);
	void DeleteBreakpointMarkers();
	// the lines with errors are underlined (none: the marks go)
	void MarkErrorLines(std::vector<int> const& lines);
	// the line where execution stopped (-1: none), made visible
	void SetCurrentLine(int line);

private:
	int m_CurrentLine{ -1 };
};
