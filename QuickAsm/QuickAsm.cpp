// QuickAsm.cpp : main source file for QuickAsm.exe
//

#include "pch.h"
#include "resource.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include <WTLHelper.h>

CAppModule _Module;
AppSettings g_Settings;

int Run(LPCTSTR /*lpstrCmdLine*/ = nullptr, int nCmdShow = SW_SHOWDEFAULT) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;
	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(L"Main window creation failed!\n");
		return 0;
	}

	wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow) {
	HRESULT hRes = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES);

	auto& settings = AppSettings::Get();
	settings.LoadFromKey(L"SOFTWARE\\ScorpioSoftware\\QuickAsm");

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	WTLHelper::InitDarkMode(settings.EffectiveTheme());

	int nRet = Run(lpstrCmdLine, nCmdShow);
	settings.Save();

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
