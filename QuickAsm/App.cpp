#include "pch.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/utils.h>
#include "LocalArtProvider.h"

wxIMPLEMENT_APP(App);

bool App::OnInit() {
	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

	wxArtProvider::Push(new LocalArtProvider);
	wxSystemOptions::SetOption("msw.remap", 0);
	auto config = wxConfig::Get();
	int mode = config->ReadLong(L"DarkMode", 2);


	//switch (mode) {
	//	case 0: MSWEnableDarkMode(DarkMode _Always); break;
	//	case 2: MSWEnableDarkMode(DarkMode_Auto); break;
	//}

	auto frame = new MainFrame;
	frame->Create(nullptr, wxID_ANY, L"Quick Assembler");
	frame->SetSize(1300, 900);
	frame->Show(true);

	return true;
}

int App::OnExit() {
	if (Restart) {
		WCHAR path[MAX_PATH];
		::GetModuleFileName(nullptr, path, _countof(path));
		wxExecute(path);
	}
	return 0;
}
