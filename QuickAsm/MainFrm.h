#pragma once

#include "resource.h"
#include "AsmEditor.h"
#include "CapstoneEngine.h"
#include "UnicornEngine.h"
#include "RegistersView.h"
#include "MemoryView.h"
#include <CustomSplitterWindow.h>
#include <NativeCustomTabView.h>

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
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_SAVEPOINTREACHED, OnSavePointChanged)
		NOTIFY_HANDLER(IDC_ASM_SOURCE, SCN_SAVEPOINTLEFT, OnSavePointChanged)
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
		COMMAND_RANGE_HANDLER(ID_MODE_16BIT, ID_MODE_64BIT, OnAsmMode)
		COMMAND_RANGE_HANDLER(ID_THEME_LIGHT, ID_THEME_SYSTEM, OnTheme)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

private:
	enum class AsmMode {
		Bits16, Bits32, Bits64,
	};

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
	// the emulator gets the code (at m_Address) after it was disassembled
	void InitEmulator(uint8_t const* data, size_t size);
	void CreateBottomPane();

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnSettingChange(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled);
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
	CString m_FileName;
	std::vector<std::unique_ptr<AssemblerBase>> m_Assemblers;
	int m_AssemblerIndex{ 1 };
	AsmMode m_AsmMode{ AsmMode::Bits32 };
	uint64_t m_Address{ 0x1000 };
	std::vector<uint8_t> m_AsmBytes;
	std::vector<Instruction> m_Instructions;
};
