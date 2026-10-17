// QuickAsm.cpp : main source file for QuickAsm.exe
//

#include "pch.h"
#include "resource.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "Helpers.h"
#include <WTLHelper.h>

CAppModule _Module;
AppSettings g_Settings;

#pragma comment(lib, "imm32")

int Run(int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;
	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(L"Main window creation failed!\n");
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);
	if (__argc > 1)
		wndMain.OpenFile(__wargv[1]);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow) {
	HRESULT hRes = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_LINK_CLASS);

	auto& settings = AppSettings::Get();
	settings.LoadFromKey(L"SOFTWARE\\ScorpioSoftware\\QuickAsm");

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// before the main window: its NASM assembler looks for nasm.exe when it's created
	Helpers::ExtractNasm();
	Scintilla_RegisterClasses(hInstance);
	WTLHelper::InitDarkMode(settings.EffectiveTheme());

	int nRet = Run(nCmdShow);
	settings.Save();

	Scintilla_ReleaseResources();
	_Module.Term();
	::CoUninitialize();

	return nRet;
}
