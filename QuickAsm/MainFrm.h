#pragma once

#include "resource.h"
#include "Interfaces.h"
#include "AsmEditor.h"
#include "CapstoneEngine.h"
#include "UnicornEngine.h"
#include "RegistersView.h"
#include "MemoryView.h"
#include <CustomSplitterWindow.h>
#include <NativeCustomTabView.h>
#include <mutex>
#include <unordered_set>

class AssemblerBase;

class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CAutoUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler {
public:
	DECLARE_FRAME_WND_CLASS(L"QuickAsmMainWndClass", IDR_MAINFRAME)

	CMainFrame();
	~CMainFrame();

	// an assembly file goes to the source; any other file is disassembled
	bool OpenFile(PCWSTR path);

	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnIdle() override;

	BEGIN_MSG_MAP(CMainFrame)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChange)
		MESSAGE_HANDLER(WM_UPDATE_DARKMODE, OnUpdateDarkMode)
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_SAVEPOINTREACHED, OnSavePointChanged)
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_SAVEPOINTLEFT, OnSavePointChanged)
		NOTIFY_HANDLER(IDC_DISASSEMBLY, SCN_MARGINCLICK, OnDisassemblyMarginClick)
		MESSAGE_HANDLER(WM_EMULATOR_PAUSED, OnEmulatorPaused)
		MESSAGE_HANDLER(WM_EMULATOR_DONE, OnEmulatorDone)
		COMMAND_HANDLER(IDC_ASSEMBLER, CBN_SELCHANGE, OnAssemblerSelected)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
		COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnFileSaveAs)
		COMMAND_ID_HANDLER(ID_FILE_CLOSE, OnFileClose)
		COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_REDO, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR, OnEditCommand)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_ASM_ASSEMBLE, OnAssemble)
		COMMAND_RANGE_HANDLER(ID_ASSEMBLER_KEYSTONE, ID_ASSEMBLER_NASM, OnAssembler)
		COMMAND_ID_HANDLER(ID_ASM_RUN, OnRun)
		COMMAND_ID_HANDLER(ID_ASM_STOP, OnStop)
		COMMAND_ID_HANDLER(ID_ASM_RESTART, OnRestart)
		COMMAND_ID_HANDLER(ID_ASM_STEPINTO, OnStepInto)
		COMMAND_ID_HANDLER(ID_ASM_STEPOVER, OnStepOver)
		COMMAND_ID_HANDLER(ID_ASM_TOGGLEBP, OnToggleBreakpoint)
		COMMAND_ID_HANDLER(ID_ASM_DELETEALLBP, OnDeleteAllBreakpoints)
		COMMAND_RANGE_HANDLER(ID_MODE_16BIT, ID_MODE_64BIT, OnAsmMode)
		COMMAND_RANGE_HANDLER(ID_THEME_LIGHT, ID_THEME_SYSTEM, OnTheme)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

private:
	enum class AsmMode {
		Bits16, Bits32, Bits64,
	};

	enum class EmulatorState {
		Idle,		// not running (code may have run to its end)
		Running,
		Paused,		// at a breakpoint or after a step: the emulator thread waits in the code hook
	};

	// posted by the emulator thread
	static const UINT WM_EMULATOR_PAUSED = WM_APP + 10;	// lParam: the address
	static const UINT WM_EMULATOR_DONE = WM_APP + 11;	// wParam: uc_err

	void InitMenu(HMENU hMenu);
	void ApplyTheme();
	void UpdateUI();
	void UpdateTitle();
	bool IsStatusBarVisible() const;
	CAsmEditor* GetFocusedEditor();
	// false if the user canceled
	bool SaveChangesIfNeeded();
	bool SaveSource(PCWSTR path);
	bool DoSaveAs();
	void Assemble();
	void Disassemble(uint8_t const* data, size_t size);
	void CreateBottomPane();
	void CreateToolBar();
	// the address in the address box (false if it isn't one)
	bool ParseAddress();

	// emulator (MainFrmEmulator.cpp)
	// the emulator gets the code (at m_Address) after it was disassembled (the breakpoints go, unless it's the same code)
	void InitEmulator(uint8_t const* data, size_t size, bool keepBreakpoints = false);
	// runs from the start (the state is reset if the code ran before), or continues when paused
	void Run(bool stepInto = false, uint64_t stopAt = 0);
	// waits for the emulator thread to end
	void StopEmulator();
	void OnCodeHook(uint64_t address);
	void ShowEmulatorState(int line);
	int LineFromAddress(uint64_t address) const;
	bool IsEmulatorBusy() const;

	LRESULT OnDisassemblyMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnEmulatorPaused(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnEmulatorDone(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRestart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStepInto(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStepOver(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggleBreakpoint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDeleteAllBreakpoints(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	void ToggleBreakpoint(int line);

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnUpdateDarkMode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSavePointChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAssemble(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAssembler(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAssemblerSelected(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAsmMode(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnTheme(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CStatusBarCtrl m_StatusBar;
	CCustomSplitterWindow m_Splitter;
	CCustomHorSplitterWindow m_RightSplitter;
	CAsmEditor m_AsmSource;
	CAsmEditor m_Disassembly;
	CNativeCustomTabView m_Tabs;
	UnicornEngine m_Emulator;
	CRegistersView m_RegistersView{ m_Emulator };
	std::vector<uint8_t> m_Memory;
	CMemoryView m_MemoryView{ m_Memory };
	CStatic m_BreakpointsPlaceholder;
	CComboBox m_AssemblerCombo;
	CEdit m_AddressEdit;
	CStatic m_AssemblerLabel, m_AddressLabel;
	CString m_FileName;
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	int m_AssemblerIndex{ 1 };
	AsmMode m_AsmMode{ AsmMode::Bits32 };
	uint64_t m_Address{ 0x1000 };
	std::vector<uint8_t> m_AsmBytes;
	std::vector<Instruction> m_Instructions;
	std::unordered_map<uint64_t, int> m_AddressToLine;

	// emulator: what the emulator thread uses is set before it runs or while it waits in the code hook,
	// except the breakpoints (which can be toggled while it runs)
	EmulatorState m_EmulatorState{ EmulatorState::Idle };
	wil::unique_threadpool_work m_EmulatorWork;
	void* m_CodeHook{ nullptr };
	wil::unique_event m_ContinueEvent{ wil::EventOptions::None }, m_StopEvent{ wil::EventOptions::None };
	std::unordered_set<uint64_t> m_Breakpoints;
	mutable std::mutex m_BreakpointsLock;
	std::atomic<bool> m_StepInto{ false };
	std::atomic<uint64_t> m_StopAt{ 0 };	// a one-time stop (step over)
	std::atomic<bool> m_StopRequested{ false };
	bool m_CodeRan{ false };	// since the emulator was set up
	bool m_RestartPending{ false };
	int m_CurrentLine{ -1 };
};
