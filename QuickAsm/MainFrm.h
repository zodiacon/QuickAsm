#pragma once

#include "resource.h"
#include "Interfaces.h"
#include "AsmEditor.h"
#include "CapstoneEngine.h"
#include "UnicornEngine.h"
#include "RegistersView.h"
#include "MemoryView.h"
#include "BreakpointsView.h"
#include "RecentFilesManager.h"
#include "StackView.h"
#include "Expression.h"
#include <CustomSplitterWindow.h>
#include <NativeCustomTabView.h>
#include <mutex>

class AssemblerBase;

class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CAutoUpdateUI<CMainFrame>,
	public CMessageFilter,
	public CIdleHandler,
	public IBreakpointsHost {
public:
	DECLARE_FRAME_WND_CLASS(L"QuickAsmMainWndClass", IDR_MAINFRAME)

	CMainFrame();
	~CMainFrame();

	// an assembly file goes to the source, a session sets everything up; any other file is disassembled
	bool OpenFile(PCWSTR path);

	BOOL PreTranslateMessage(MSG* pMsg) override;

	// IBreakpointsHost
	std::vector<BreakpointInfo> GetBreakpoints() const override;
	void EnableBreakpoint(uint64_t address, bool enable) override;
	void EnableAllBreakpoints(bool enable) override;
	void DeleteBreakpoint(uint64_t address) override;
	void DeleteAllBreakpoints() override;
	void GoToBreakpoint(uint64_t address) override;
	void EditBreakpoint(uint64_t address) override;
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
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_MARGINCLICK, OnSourceMarginClick)
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_CHARADDED, OnSourceCharAdded)
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_URIDROPPED, OnEditorFileDropped)
		NOTIFY_HANDLER(IDC_DISASSEMBLY, SCN_URIDROPPED, OnEditorFileDropped)
		MESSAGE_HANDLER(WM_DROPFILES, OnDropFiles)
		MESSAGE_HANDLER(WM_OPEN_DROPPED, OnOpenDropped)
		COMMAND_RANGE_HANDLER(ID_VIEW_REGISTERS, ID_VIEW_STACK, OnViewTab)
		MESSAGE_HANDLER(CFindReplaceDialog::GetFindReplaceMsg(), OnFindReplace)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnEditFind)
		COMMAND_ID_HANDLER(ID_EDIT_REPLACE, OnEditFind)
		COMMAND_ID_HANDLER(ID_EDIT_FINDNEXT, OnEditFindNext)
		COMMAND_ID_HANDLER(ID_EDIT_FINDPREV, OnEditFindNext)
		NOTIFY_HANDLER(IDC_DISASSEMBLY, SCN_DOUBLECLICK, OnDisassemblyDoubleClick)
		MESSAGE_HANDLER(WM_EMULATOR_PAUSED, OnEmulatorPaused)
		MESSAGE_HANDLER(WM_EMULATOR_DONE, OnEmulatorDone)
		COMMAND_HANDLER(IDC_ASSEMBLER, CBN_SELCHANGE, OnAssemblerSelected)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		COMMAND_ID_HANDLER(ID_FILE_NEW, OnFileNew)
		COMMAND_RANGE_HANDLER(ID_FILE_NEW16BIT, ID_FILE_NEW64BIT, OnFileNewFromTemplate)
		COMMAND_ID_HANDLER(ID_FILE_OPEN, OnFileOpen)
		COMMAND_ID_HANDLER(ID_FILE_SAVE, OnFileSave)
		COMMAND_ID_HANDLER(ID_FILE_SAVE_AS, OnFileSaveAs)
		COMMAND_ID_HANDLER(ID_FILE_OPENSESSION, OnFileOpenSession)
		COMMAND_ID_HANDLER(ID_FILE_SAVESESSION, OnFileSaveSession)
		COMMAND_ID_HANDLER(ID_FILE_CLOSE, OnFileClose)
		COMMAND_RANGE_HANDLER(ATL_IDS_MRU_FILE, ATL_IDS_MRU_FILE + 14, OnRecentFile)
		COMMAND_ID_HANDLER(ID_EDIT_UNDO, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_REDO, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_CUT, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_PASTE, OnEditCommand)
		COMMAND_ID_HANDLER(ID_EDIT_CLEAR, OnEditCommand)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_EDIT_COMPLETEWORD, OnEditCompleteWord)
		COMMAND_ID_HANDLER(ID_OPTIONS_AUTOCOMPLETE, OnOptionsAutoComplete)
		COMMAND_ID_HANDLER(ID_OPTIONS_FONT, OnOptionsFont)
		COMMAND_RANGE_HANDLER(ID_DISASM_INTEL, ID_DISASM_UPPERCASE, OnDisassemblyOption)
		COMMAND_ID_HANDLER(ID_OPTIONS_MEMORY, OnOptionsMemory)
		COMMAND_ID_HANDLER(ID_ASM_ASSEMBLE, OnAssemble)
		COMMAND_RANGE_HANDLER(ID_ASSEMBLER_KEYSTONE, ID_ASSEMBLER_NASM, OnAssembler)
		COMMAND_ID_HANDLER(ID_ASM_RUN, OnRun)
		COMMAND_ID_HANDLER(ID_ASM_BREAK, OnBreak)
		COMMAND_ID_HANDLER(ID_ASM_STOP, OnStop)
		COMMAND_ID_HANDLER(ID_ASM_RESTART, OnRestart)
		COMMAND_ID_HANDLER(ID_ASM_STEPINTO, OnStepInto)
		COMMAND_ID_HANDLER(ID_ASM_STEPOVER, OnStepOver)
		COMMAND_ID_HANDLER(ID_ASM_STEPOUT, OnStepOut)
		COMMAND_ID_HANDLER(ID_ASM_RUNTOCURSOR, OnRunToCursor)
		COMMAND_ID_HANDLER(ID_ASM_TOGGLEBP, OnToggleBreakpoint)
		COMMAND_ID_HANDLER(ID_ASM_ENABLEBP, OnEnableBreakpoint)
		COMMAND_ID_HANDLER(ID_ASM_DELETEALLBP, OnDeleteAllBreakpoints)
		COMMAND_ID_HANDLER(ID_ASM_BPCONDITION, OnBreakpointCondition)
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

	struct Breakpoint {
		bool Enabled{ true };
		std::string Condition;
		std::shared_ptr<Expression> Expr;	// (the condition's)
		uint32_t HitTarget{ 0 };
		uint32_t Hits{ 0 };
	};

	// posted by the emulator thread
	static const UINT WM_EMULATOR_PAUSED = WM_APP + 10;	// lParam: the address; wParam: TRUE if a breakpoint's condition couldn't be evaluated
	static const UINT WM_EMULATOR_DONE = WM_APP + 11;	// wParam: uc_err
	static const UINT WM_OPEN_DROPPED = WM_APP + 12;

	void InitMenu(HMENU hMenu);
	void ApplyTheme();
	void UpdateUI();
	void UpdateTitle();
	bool IsStatusBarVisible() const;
	CAsmEditor* GetFocusedEditor();
	// false if the user canceled
	bool SaveChangesIfNeeded();
	// the source becomes this text (the changes saved first if the user wants); false if the user canceled
	bool NewSource(const char* text);
	// the editors' font from the settings
	void ApplyEditorFont();
	bool SaveSource(PCWSTR path);
	void AddRecentFile(PCWSTR path);
	void UpdateRecentFilesMenu();
	bool DoSaveAs();
	void Assemble();
	void ShowWarnings(std::string const& warnings);
	// the source line of each instruction: from the assembler, or (if it doesn't tell) the source's instruction lines in order
	void MapSourceLines(std::vector<std::pair<uint32_t, int>> const& sourceLines);
	// the source line of the instruction at a line of the disassembly (-1: none, or the source changed since)
	int SourceLineFromDisassemblyLine(int line) const;
	// the current instruction (-1: none), in the disassembly and the source
	void ShowCurrentLine(int line);
	static std::vector<int> GetErrorLines(std::string const& errors);
	// the line of an error ("<line>: ..."), 0 based; -1 if none
	static int ParseErrorLine(std::string const& text);
	// shows the code's instructions and sets up the emulator with it
	void Disassemble(uint8_t const* data, size_t size);
	// the instructions and the text, with the disassembly options (the same code: the emulator stays as it is)
	void DisassembleText(uint8_t const* data, size_t size);
	// with the case option
	std::string FormatInstruction(Instruction const& insn) const;
	// the code's part of the bytes (the rest is data, not run or disassembled)
	size_t GetCodeSize() const;
	void CreateBottomPane();
	void CreateToolBar();
	// the address in the address box (false if it isn't one)
	bool ParseAddress();

	// emulator (MainFrmEmulator.cpp)
	// the emulator gets the code (at m_Address) after it was disassembled (the breakpoints go, unless it's the same code)
	void InitEmulator(uint8_t const* data, size_t size, bool keepBreakpoints = false);
	// runs from the start (the state is reset if the code ran before), or continues when paused;
	// stops at the next instruction, at an address, or (step out) after a return to the caller
	void Run(bool stepInto = false, uint64_t stopAt = 0, bool stepOut = false);
	uint64_t ReadStackPointer() const;
	bool IsReturn(uint64_t address) const;
	// waits for the emulator thread to end
	void StopEmulator();
	// tells the emulator thread to end (without waiting)
	void RequestStop();
	void OnCodeHook(uint64_t address);
	void ShowEmulatorState(int line);
	int LineFromAddress(uint64_t address) const;
	bool IsEmulatorBusy() const;

	LRESULT OnDisassemblyMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnSourceMarginClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnFindReplace(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnEditFind(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditFindNext(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDisassemblyDoubleClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnEmulatorPaused(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnEmulatorDone(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnRun(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBreak(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRestart(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStepInto(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStepOver(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnStepOut(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRunToCursor(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggleBreakpoint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEnableBreakpoint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDeleteAllBreakpoints(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBreakpointCondition(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// asks for a breakpoint's condition (create: a new breakpoint there if it's set)
	void EditBreakpoint(uint64_t address, bool create);
	// true: break (also if the condition can't be evaluated: error is set); counts the hit (called on the emulator thread)
	bool CheckBreakpoint(Breakpoint& bp, bool& error);
	void ToggleBreakpoint(int line);
	// the disassembly line of the code of a source line: -1 if none (or the source changed since it was assembled)
	int DisassemblyLineFromSourceLine(int sourceLine);
	// the disassembly line a command is about: at the caret, in the source (its code) if it has the focus
	int GetTargetLine();
	void UpdateStackView();
	void OpenFindDialog(bool replace);
	void FindNext(bool down);
	// the disassembly's markers and the breakpoints view, after the breakpoints changed
	void BreakpointsChanged();
	int GetDisassemblyCaretLine() const;

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
	LRESULT OnUpdateDarkMode(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSavePointChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNew(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileNewFromTemplate(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileOpen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSave(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSaveAs(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileClose(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnRecentFile(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCommand(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEditCompleteWord(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOptionsAutoComplete(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOptionsFont(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDisassemblyOption(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOptionsMemory(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// sessions (MainFrmSession.cpp)
	static constexpr PCWSTR SessionExtension = L".qasession";
	static bool IsSessionFile(PCWSTR path);
	bool SaveSession(PCWSTR path);
	// the source, its setup (assembler, mode, address, memory) and its breakpoints; the code is assembled
	bool OpenSession(PCWSTR path);
	LRESULT OnFileOpenSession(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFileSaveSession(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// the memory from the settings (the defaults if they aren't valid)
	void SetupMemory();
	uint64_t MemoryEnd() const;
	// the code (or data) fits there: in the memory, below the stack
	bool FitsInMemory(uint64_t address, size_t size) const;
	// the host's bytes of an emulated range; null if it isn't all in the memory
	uint8_t const* MemoryAt(uint64_t address, size_t size) const;
	LRESULT OnSourceCharAdded(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnEditorFileDropped(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/);
	LRESULT OnDropFiles(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOpenDropped(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnViewTab(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
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
	std::vector<uint8_t> m_Memory;	// at m_MemoryBase
	uint64_t m_MemoryBase{ 0 };
	uint64_t m_StackSize{ 0x1000 };	// (at the top of the memory)
	CMemoryView m_MemoryView{ m_Memory };
	CBreakpointsView m_BreakpointsView{ *this };
	CStackView m_StackView;
	// find and replace (the dialog deletes itself when closed)
	CFindReplaceDialog* m_pFindDlg{ nullptr };
	bool m_FindReplace{ false };
	CAsmEditor* m_FindTarget{ nullptr };
	CString m_SearchText, m_ReplaceText;
	bool m_MatchCase{ false }, m_WholeWord{ false };
	CComboBox m_AssemblerCombo;
	CEdit m_AddressEdit;
	CStatic m_AssemblerLabel, m_AddressLabel;
	CString m_FileName;
	CString m_SessionFile;	// the last opened or saved
	RecentFilesManager m_RecentFiles;
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	int m_AssemblerIndex{ 1 };
	AsmMode m_AsmMode{ AsmMode::Bits32 };
	uint64_t m_Address{ 0x1000 };
	std::vector<uint8_t> m_AsmBytes;
	size_t m_CodeSize{ 0 };		// the code at the start of m_AsmBytes, before data (other sections); 0: all
	std::vector<Instruction> m_Instructions;
	std::unordered_map<uint64_t, int> m_AddressToLine;
	std::unordered_map<uint64_t, int> m_AddressToSourceLine;
	std::string m_AssembledSource;	// (the source lines are of this text)

	// emulator: what the emulator thread uses is set before it runs or while it waits in the code hook,
	// except the breakpoints (which can be toggled while it runs)
	EmulatorState m_EmulatorState{ EmulatorState::Idle };
	wil::unique_threadpool_work m_EmulatorWork;
	void* m_CodeHook{ nullptr };
	wil::unique_event m_ContinueEvent{ wil::EventOptions::None }, m_StopEvent{ wil::EventOptions::None };
	std::map<uint64_t, Breakpoint> m_Breakpoints;
	mutable std::mutex m_BreakpointsLock;
	std::atomic<bool> m_StepInto{ false };
	std::atomic<uint64_t> m_StopAt{ 0 };	// a one-time stop (step over, run to cursor)
	std::atomic<bool> m_StepOut{ false };
	std::atomic<bool> m_BreakRequested{ false };	// pause at the next instruction (Break)
	// an enabled breakpoint exists (else the code hook has nothing to look up); set with the markers (BreakpointsChanged)
	std::atomic<bool> m_AnyBreakpoints{ false };
	uint64_t m_StepOutSP{ 0 };		// the stack pointer when stepping out started
	bool m_LastWasReturn{ false };	// (the emulator thread's: the previous instruction was a return)
	std::atomic<bool> m_StopRequested{ false };
	bool m_CodeRan{ false };	// since the emulator was set up
	bool m_RestartPending{ false };
	int m_CurrentLine{ -1 };
	CString m_DroppedFile;	// on an editor, to open (WM_OPEN_DROPPED)
};
