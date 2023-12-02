#include "pch.h"
#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit() {
	auto frame = new MainFrame;
	frame->Create(nullptr, wxID_ANY, L"Quick Assembler");
	frame->SetSize(1200, 800);
	frame->Show(true);

	return true;
}
