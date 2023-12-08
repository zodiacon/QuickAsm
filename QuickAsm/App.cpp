#include "pch.h"
#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit() {
	wxSystemOptions::SetOption("msw.remap", 0);

	auto frame = new MainFrame;
	frame->Create(nullptr, wxID_ANY, L"Quick Assembler");
	frame->SetSize(1300, 900);
	frame->Show(true);

	return true;
}
