#pragma once

#include "AssemblyEditCtrl.h"

class AssemblerBase;

class MainFrame : public wxFrame {
public:
	MainFrame();

private:
	void OnCreate(wxWindowCreateEvent& event);
	void CreateMenu();
	void OnExit(wxCommandEvent& event);
	void Assemble(const wxString& text);
	void Disassemble(unsigned char* data, size_t size);
	void DoSaveAs(wxCommandEvent& event);
	void Enable(int id, bool enable);

	wxSplitterWindow m_Splitter;
	AssemblyEditCtrl m_AsmSource, m_DisamSource;
	wxTextCtrl* m_AddressText;
	wxMenuBar* m_MenuBar;
	int m_AsmModeIndex{ 1 };
	wxString m_FileName;
	int m_AssemblerIndex{ 1 };
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	wxNotebook m_Notebook;
	bool m_Modified{ false };
};

