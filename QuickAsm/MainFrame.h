#pragma once

#include "AssemblyEditCtrl.h"
#include "CapstoneEngine.h"
#include "UnicornEngine.h"

class AssemblerBase;
struct RegisterInfo;
class wxHexView;
class wxNativeWindow;

class MainFrame : public wxFrame {
public:
	MainFrame();

	LRESULT MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) override;

private:
	void OnCreate(wxWindowCreateEvent& event);
	void CreateMenu();
	void OnExit(wxCommandEvent& event);
	void Assemble(const wxString& text);
	void Disassemble(uint8_t const* data, size_t size);
	void Disassemble(std::vector<uint8_t> const& bytes);
	void DoSaveAs(wxCommandEvent& event);
	void Enable(int id, bool enable);
	void ShowRegisters();
	void Run(wxCommandEvent& e);
	void Debug(wxCommandEvent& e);
	void UpdateEmulatorState();
	void SetRegisterValue(int row, RegisterInfo const& reg);
	void DoSortRegisters(int col, bool asc);
	void RunOnThreadPool();

	enum class EmulatorState {
		Idle,
		Running,
		Breakpoint,
	};

	enum class EmulatorMessage {
		RunComplete = WM_USER + 100,
	};

	wxSplitterWindow m_Splitter, m_HSplitter;
	wxNotebook* m_Notebook;
	wxListView m_RegistersList;
	AssemblyEditCtrl m_AsmSource, m_DisamSource;
	wxTextCtrl* m_AddressText;
	wxMenuBar* m_MenuBar;
	wxNativeWindow* m_MemoryView;
	int m_AsmModeIndex{ 1 };
	wxString m_FileName;
	int m_AssemblerIndex{ 1 };
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	std::vector<uint8_t> m_AsmBytes;
	wxImageList m_ImageList;
	std::vector<Instruction> m_Instructions;
	UnicornEngine m_Emulator;
	EmulatorState m_EmulatorState;
	DWORD m_RegViewFilter{ 1 << 2 };
	void* m_EmulatorContext{ nullptr };
	std::vector<uint8_t> m_Memory;
	bool m_Modified{ false };
};

