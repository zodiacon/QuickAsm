#include "pch.h"
#include "App.h"
#include "MainFrame.h"
#include <wx/config.h>

wxIMPLEMENT_APP(App);

bool App::OnInit() {
	::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	wxSystemOptions::SetOption("msw.remap", 0);
	auto config = wxConfig::Get();
	int mode = config->ReadLong(L"DarkMode", 2);
	switch(mode) {
		case 0: MSWEnableDarkMode(DarkMode_Always); break;
		case 2: MSWEnableDarkMode(DarkMode_Auto);
	}
	auto frame = new MainFrame;
	frame->Create(nullptr, wxID_ANY, L"Quick Assembler");
	frame->SetSize(1300, 900);
	frame->Show(true);

	return true;
}
