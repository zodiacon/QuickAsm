#pragma once

#include "ScintillaCtrl.h"

// a Scintilla control set up for x86 assembly: used for the source and the disassembly
class CAsmEditor : public CScintillaCtrl {
public:
	enum {
		BreakpointMarker = 1,
		CurrentLineMarker = 2,
		CurrentLineIndicator = 2,
	};

	HWND Create(HWND hParent, UINT id, bool readOnly = false);
	// the colors of the current theme (light or dark)
	void ApplyTheme();
	bool ToggleBreakpoint(int line);
	// the line where execution stopped (-1: none), made visible
	void SetCurrentLine(int line);

private:
	int m_CurrentLine{ -1 };
};
