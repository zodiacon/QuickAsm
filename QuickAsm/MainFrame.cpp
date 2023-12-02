#include "pch.h"
#include "MainFrame.h"
#include "LocalArtProvider.h"
#include "KeystoneAssembler.h"
#include "NasmAssembler.h"
#include "Helpers.h"

enum {
	wxID_ASSEMBLE = wxID_HIGHEST + 1,
	wxID_16BIT,
	wxID_32BIT,
	wxID_64BIT,
};

MainFrame::MainFrame() {
	Bind(wxEVT_CREATE, &MainFrame::OnCreate, this);
	Bind(wxEVT_MENU, &MainFrame::OnExit, this, wxID_EXIT);

	auto handler = [this](auto& e) {
		auto focus = FindFocus();
		if (focus) {
			focus->GetEventHandler()->ProcessEventLocally(e);
		}
		};

	Bind(wxEVT_MENU, handler, wxID_EDIT, wxID_EDIT + 10);
	Bind(wxEVT_MENU, handler, wxID_UNDO, wxID_REDO);
	Bind(wxEVT_MENU, [this](auto& e) {
		wxFileDialog dlg(this, L"Open Assembly file", wxEmptyString, wxEmptyString,
		L"Assembly files (*.asm)|*.asm|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		m_FileName = dlg.GetPath();
		m_AsmSource.LoadFile(m_FileName);
		SetTitle(wxString::Format(L"QuickAsm - %s", m_FileName));
		m_AsmSource.SetModified(false);
	}
		}, wxID_OPEN);

	Bind(wxEVT_MENU, [this](auto& e) { DoSaveAs(e); }, wxID_SAVEAS);

	Bind(wxEVT_MENU, [this](auto& e) {
		if (m_FileName.IsEmpty())
			DoSaveAs(e);
		else {
			m_AsmSource.SaveFile(m_FileName);
			m_AsmSource.SetModified(false);
		}
		}, wxID_SAVE);

	Bind(wxEVT_MENU, [this](auto& e) {
		auto text = m_AsmSource.GetText();
		Assemble(text);
		}, wxID_ASSEMBLE);
	Bind(wxEVT_MENU, [this](auto& e) {
		m_AsmModeIndex = e.GetId() - wxID_16BIT;
		}, wxID_16BIT, wxID_64BIT);

	m_Assemblers.push_back(std::make_unique<KeystoneAssembler>());
	m_Assemblers.push_back(std::make_unique<NasmAssembler>());
}

void MainFrame::OnCreate(wxWindowCreateEvent& event) {
	Unbind(wxEVT_CREATE, &MainFrame::OnCreate, this);

	wxArtProvider::Push(new LocalArtProvider);
	SetIcon(wxICON(0APP));

	wxSystemOptions::SetOption("msw.remap", 2);
	CreateMenu();
	CreateStatusBar();

	auto tb = CreateToolBar(wxTB_HORZ_LAYOUT | wxTB_HORIZONTAL | wxTB_TEXT);
	wxSize size(24, 24);
	tb->AddTool(wxID_OPEN, L"Open", wxArtProvider::GetIcon(wxART_FILE_OPEN, wxART_TOOLBAR, size));
	tb->AddTool(wxID_SAVE, wxEmptyString, wxArtProvider::GetIcon(wxART_FILE_SAVE, wxART_TOOLBAR, size));
	tb->AddSeparator();
	wxArrayString assemblers{ L"Keystone", L"NASM", L"MASM" };
	auto cbAsm = new wxComboBox(tb, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxSize(80, -1), assemblers, wxCB_READONLY);
	cbAsm->SetSelection(0);
	cbAsm->Bind(wxEVT_COMBOBOX, [this](auto& e) {
		m_AssemblerIndex = e.GetSelection();
		});
	tb->AddSeparator();
	auto asmIcon = new wxStaticBitmap(tb, wxID_ANY, wxArtProvider::GetIcon(L"BINARY", wxART_TOOLBAR));
	tb->AddControl(asmIcon);
	tb->AddControl(cbAsm);
	tb->AddSeparator();
	tb->AddRadioTool(wxID_16BIT, wxEmptyString, wxArtProvider::GetIcon(L"16BIT", wxART_TOOLBAR));
	auto tool = tb->AddRadioTool(wxID_32BIT, wxEmptyString, wxArtProvider::GetIcon(L"32BIT", wxART_TOOLBAR));
	tb->AddRadioTool(wxID_64BIT, wxEmptyString, wxArtProvider::GetIcon(L"64BIT", wxART_TOOLBAR));
	tool->Toggle(true);
	tb->AddSeparator();
	tb->AddControl(new wxStaticText(tb, wxID_ANY, L"Address:"));
	tb->AddControl(m_AddressText = new wxTextCtrl(tb, wxID_ANY, "0x1000", wxDefaultPosition, wxSize(100, -1), wxTE_RIGHT));
	tb->AddSeparator();
	tb->AddTool(wxID_ASSEMBLE, L"Assemble", wxArtProvider::GetIcon(L"BUILD", wxART_TOOLBAR, size))->Enable(false);
	tb->Realize();

	m_Splitter.Create(this, wxID_ANY);
	m_Splitter.SetMinimumPaneSize(150);
	m_Splitter.SetWindowStyleFlag(wxSP_LIVE_UPDATE | wxSP_BORDER);

	m_AsmSource.Create(&m_Splitter, wxID_ANY);
	m_AsmSource.Init();
	m_AsmSource.Bind(wxEVT_STC_UPDATEUI, [this](auto& e) {
		auto anyText = m_AsmSource.GetTextLength() > 0;
		Enable(wxID_SAVEAS, anyText);
		Enable(wxID_ASSEMBLE, anyText);
		auto modified = m_AsmSource.IsModified();
		Enable(wxID_SAVE, modified);
		if(modified != m_Modified) {
			m_Modified = modified;
			SetTitle(wxString::Format(L"QuickAsm - %s%s", m_FileName, modified ? L"*" : L""));
		}
		Enable(wxID_UNDO, m_AsmSource.CanUndo());
		Enable(wxID_REDO, m_AsmSource.CanRedo());
		Enable(wxID_COPY, m_AsmSource.CanCopy());
		Enable(wxID_PASTE, m_AsmSource.CanPaste());
		Enable(wxID_CLEAR, m_AsmSource.CanCut());
		Enable(wxID_CUT, m_AsmSource.CanCut());
		});

	//
	// build right pane
	//
	m_DisamSource.Create(&m_Splitter, wxID_ANY);
	m_DisamSource.Init();
	m_DisamSource.SetReadOnly(true);


	m_Splitter.SplitVertically(&m_AsmSource, &m_DisamSource);
	m_AsmSource.SetFocus();
}

void MainFrame::Enable(int id, bool enable) {
	GetMenuBar()->Enable(id, enable);
	GetToolBar()->EnableTool(id, enable);
}

void MainFrame::DoSaveAs(wxCommandEvent&) {
	wxFileDialog dlg(this, L"Save Assembly file", wxEmptyString, wxEmptyString,
		L"Assembly files (*.asm)|*.asm|All files (*.*)|*.*", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
	if (dlg.ShowModal() == wxID_OK) {
		m_AsmSource.SaveFile(dlg.GetPath());
		m_FileName = dlg.GetPath();
		m_AsmSource.SetModified(false);
	}
}

void MainFrame::CreateMenu() {
	// File menu
	wxSize size(16, 16);

	auto menuFile = new wxMenu;
	auto item = menuFile->Append(wxID_OPEN, _("&Open...\tCtrl+O"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_OPEN, wxART_MENU, size));
	item = item = menuFile->Append(wxID_SAVE, _("&Save\tCtrl+S"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_SAVE, wxART_MENU, size));
	item = menuFile->Append(wxID_SAVEAS, _("Save &as...\tCtrl+Shift+S"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_SAVE_AS, wxART_MENU, size));
	menuFile->Append(wxID_CLOSE, _("&Close\tCtrl+W"))->Enable(false);
	menuFile->AppendSeparator();
	item = menuFile->Append(wxID_EXIT, _("&Quit\tCtrl+Q"));

	// Edit menu
	auto menuEdit = new wxMenu;
	item = menuEdit->Append(wxID_UNDO, _("&Undo\tCtrl+Z"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_UNDO, wxART_MENU, size));
	item = menuEdit->Append(wxID_REDO, _("&Redo\tCtrl+Shift+Z"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_REDO, wxART_MENU, size));
	menuEdit->AppendSeparator();
	item = menuEdit->Append(wxID_CUT, _("Cu&t\tCtrl+X"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_CUT, wxART_MENU, size));
	item = menuEdit->Append(wxID_COPY, _("&Copy\tCtrl+C"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_COPY, wxART_MENU, size));
	item = menuEdit->Append(wxID_PASTE, _("&Paste\tCtrl+V"));
	item->SetBitmap(wxArtProvider::GetIcon(wxART_PASTE, wxART_MENU, size));
	menuEdit->Append(wxID_CLEAR, _("&Delete\tDel"));
	menuEdit->AppendSeparator();
	menuEdit->Append(wxID_FIND, _("&Find\tCtrl+F"));
	menuEdit->Enable(wxID_FIND, false);

	auto asmMenu = new wxMenu;
	item = asmMenu->Append(wxID_ASSEMBLE, _("&Assemble\tF7"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(L"BUILD", wxART_MENU, size));

	m_MenuBar = new wxMenuBar;
	m_MenuBar->Append(menuFile, L"&File");
	m_MenuBar->Append(menuEdit, L"&Edit");
	m_MenuBar->Append(asmMenu, L"&Assemble");

	SetMenuBar(m_MenuBar);
}

void MainFrame::OnExit(wxCommandEvent& event) {
	Close();
}

void MainFrame::Assemble(const wxString& text) {
	auto assembler = m_Assemblers[m_AssemblerIndex].get();
	assembler->SetValue("mode", 1 << (4 + m_AsmModeIndex));
	assembler->SetValue("address", std::wcstoll(m_AddressText->GetValue().ToStdWstring().c_str(), nullptr, 0));

	AssemblerResults results;
	results = assembler->Assemble(m_AsmSource.GetText().utf8_string());

	if (results.Error) {
		m_DisamSource.SetReadOnly(false);
		m_DisamSource.SetText(results.Output);
		m_DisamSource.SetReadOnly(true);
	}
	else {
		if (results.Bytes.empty())
			results.Bytes = Helpers::ReadFileContents(results.OutputFile);
		Disassemble(results.Bytes.data(), results.Bytes.size());
	}
}

void MainFrame::Disassemble(unsigned char* data, size_t size) {
	csh cs;
	auto mode = 1 << (1 + m_AsmModeIndex);
	if (CS_ERR_OK == cs_open(CS_ARCH_X86, (cs_mode)mode, &cs)) {
		cs_insn* inst = nullptr;
		auto n = cs_disasm(cs, data, size, std::wcstoll(m_AddressText->GetValue().ToStdWstring().c_str(), nullptr, 0), 0, &inst);
		if (n) {
			m_DisamSource.SetReadOnly(false);
			m_DisamSource.ClearAll();
			for (size_t i = 0; i < n; i++) {
				auto& insn = inst[i];
				m_DisamSource.AppendText(wxString::Format("0x%08X %-8s %-24s ; ",
					(DWORD)insn.address, insn.mnemonic, insn.op_str));
				for (int b = 0; b < insn.size; b++) {
					m_DisamSource.AppendText(wxString::Format(L"%02X ", insn.bytes[b]));
				}
				m_DisamSource.AppendText(L"\n");
			}
			m_DisamSource.SetReadOnly(true);
			cs_free(inst, n);
		}
		cs_close(&cs);
	}
}
