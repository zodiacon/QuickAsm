#pragma once

#include "AssemblyEditCtrl.h"
#include "CapstoneEngine.h"
#include "UnicornEngine.h"
#include "HexViewPanel.h"

class AssemblerBase;
struct RegisterInfo;
class wxHexView;

class MainFrame : public wxFrame, public IHexViewNotify {
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
	void Stop(wxCommandEvent& e);
	void ToggleBreakpoint(wxCommandEvent& e);
	void ToggleBreakpoint(int line);

	void UpdateEmulatorState();
	void SetRegisterValue(int row, RegisterInfo const& reg);
	void DoSortRegisters(int col, bool asc);
	void RunOnThreadPool();
	bool OnHexViewNotify(wxHexView* pHexView, int idCtrl, LPNMHDR hdr, WXLPARAM* result) override;

	enum class EmulatorState {
		Idle,
		Running,
		Breakpoint,
	};

	enum class EmulatorMessage {
		RunComplete = WM_USER + 100,
		BreakpointHit,
	};

	struct BreakpointInfo {
		size_t Address;
		int Line;
		bool IsEnabled : 1{ true };
	};

	wxSplitterWindow* m_Splitter;
	wxSplitterWindow* m_HSplitter;
	wxNotebook* m_Notebook;
	wxListView m_RegistersList;
	AssemblyEditCtrl* m_AsmSource;
	AssemblyEditCtrl* m_DisamSource;
	wxTextCtrl* m_AddressText;
	wxMenuBar* m_MenuBar;
	wxHexView* m_MemoryView;
	int m_AsmModeIndex{ 1 };
	wxString m_FileName;
	int m_AssemblerIndex{ 1 };
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	std::unordered_map<size_t, BreakpointInfo> m_Breakpoints;
	std::vector<uint8_t> m_AsmBytes;
	wxImageList m_ImageList;
	std::vector<Instruction> m_Instructions;
	UnicornEngine m_Emulator;
	EmulatorState m_EmulatorState;
	DWORD m_RegViewFilter{ 1 << 2 };
	void* m_EmulatorContext{ nullptr };
	std::vector<uint8_t> m_Memory;
	wil::unique_handle m_hContinueEvent, m_hStopEvent;
	std::atomic<uint64_t> m_BreakpointAddress{ 0 };
	bool m_Modified{ false };
	bool m_HexViewActive{ false };
};

