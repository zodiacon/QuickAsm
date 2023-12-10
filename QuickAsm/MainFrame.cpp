#include "pch.h"
#include "MainFrame.h"
#include "LocalArtProvider.h"
#include "KeystoneAssembler.h"
#include "NasmAssembler.h"
#include "Helpers.h"
#include "RegisterInfo.h"
#include "..\HexView\HexView.h"
#include <wx/nativewin.h>
#include "wxHexView.h"

enum {
	wxID_ASSEMBLE = wxID_HIGHEST + 1,
	wxID_16BIT,
	wxID_32BIT,
	wxID_64BIT,
	wxID_8BITREG,
	wxID_16BITREG,
	wxID_32BITREG,
	wxID_64BITREG,
	wxID_RUN,
};

MainFrame::MainFrame() {
	m_Memory.resize(1 << 16);
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
		wxFileDialog dlg(this, L"Open Assembly/Binary file", wxEmptyString, wxEmptyString,
		L"Assembly files (*.asm)|*.asm|Binary Files (*.bin)|*.bin|All files (*.*)|*.*", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
	if (dlg.ShowModal() == wxID_OK) {
		m_FileName = dlg.GetPath();
		if (m_FileName.Right(4).CmpNoCase(L".asm") != 0) {
			m_AsmBytes = Helpers::ReadFileContents(m_FileName);
			Disassemble(m_AsmBytes);
		}
		else {
			m_AsmSource.LoadFile(m_FileName);
			m_AsmSource.SetModified(false);
		}
		SetTitle(wxString::Format(L"QuickAsm - %s", m_FileName));
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
		if (!m_AsmBytes.empty())
			Disassemble(m_AsmBytes);
		}, wxID_16BIT, wxID_64BIT);

	m_Assemblers.push_back(std::make_unique<KeystoneAssembler>());
	m_Assemblers.push_back(std::make_unique<NasmAssembler>());
}

LRESULT MainFrame::MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) {
	switch (static_cast<EmulatorMessage>(msg)) {
		case EmulatorMessage::RunComplete:
			m_EmulatorState = EmulatorState::Idle;
			m_Emulator.Stop();
			Enable(wxID_RUN, true);
			Enable(wxID_STOP, false);
			Enable(wxID_ASSEMBLE, true);
			UpdateEmulatorState();
			break;
	}
	return wxFrame::MSWWindowProc(msg, wParam, lParam);
}

void MainFrame::OnCreate(wxWindowCreateEvent& event) {
	Unbind(wxEVT_CREATE, &MainFrame::OnCreate, this);

	wxArtProvider::Push(new LocalArtProvider);
	SetIcon(wxICON(0APP));

	CreateMenu();
	CreateStatusBar(2);

	auto tb = CreateToolBar(wxTB_HORZ_LAYOUT | wxTB_HORIZONTAL | wxTB_TEXT);
	wxSize size(24, 24);
	tb->AddTool(wxID_OPEN, L"Open", wxArtProvider::GetIcon(wxART_FILE_OPEN, wxART_TOOLBAR, size));
	tb->AddTool(wxID_SAVE, wxEmptyString, wxArtProvider::GetIcon(wxART_FILE_SAVE, wxART_TOOLBAR, size));
	tb->AddSeparator();
	wxArrayString assemblers;
	assemblers.Add(L"Keystone");
	assemblers.Add(L"NASM");
	auto cbAsm = new wxComboBox(tb, wxID_ANY, wxEmptyString, wxDefaultPosition,
		wxSize(80, -1), assemblers, wxCB_READONLY);
	cbAsm->SetSelection(1);
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
	tb->AddTool(wxID_RUN, L"Run", wxArtProvider::GetIcon(L"RUN", wxART_TOOLBAR, size))->Enable(false);
	tb->AddTool(wxID_STOP, L"", wxArtProvider::GetIcon("STOP", wxART_TOOLBAR, size))->Enable(false);
	tb->Realize();

	m_Splitter.Create(this, wxID_ANY);
	m_Splitter.SetMinimumPaneSize(150);
	m_Splitter.SetWindowStyleFlag(wxSP_LIVE_UPDATE | wxSP_BORDER);

	m_HSplitter.Create(&m_Splitter, wxID_ANY);
	m_HSplitter.SetMinimumPaneSize(150);
	m_HSplitter.SetWindowStyleFlag(wxSP_LIVE_UPDATE | wxSP_BORDER);

	m_AsmSource.Create(&m_Splitter, wxID_ANY);
	m_AsmSource.Init();
	m_AsmSource.Bind(wxEVT_STC_UPDATEUI, [this](auto& e) {
		auto anyText = m_AsmSource.GetTextLength() > 0;
		Enable(wxID_SAVEAS, anyText);
		Enable(wxID_ASSEMBLE, anyText);
		auto modified = m_AsmSource.IsModified();
		Enable(wxID_SAVE, modified);
		if (modified != m_Modified) {
			m_Modified = modified;
			if (!m_FileName.IsEmpty())
				SetTitle(wxString::Format(L"QuickAsm - %s%s", m_FileName, modified ? L"*" : L""));
		}
		Enable(wxID_UNDO, m_AsmSource.CanUndo());
		Enable(wxID_REDO, m_AsmSource.CanRedo());
		Enable(wxID_COPY, m_AsmSource.CanCopy());
		Enable(wxID_PASTE, m_AsmSource.CanPaste());
		Enable(wxID_CLEAR, !m_AsmSource.GetReadOnly());
		Enable(wxID_CUT, m_AsmSource.CanCut());
		});

	Bind(wxEVT_MENU, [this](auto& e) { Run(e); }, wxID_RUN);
	Bind(wxEVT_MENU, [this](auto& e) { Stop(e); }, wxID_STOP);

	//
	// build right pane
	//
	m_DisamSource.Create(&m_HSplitter, wxID_ANY);
	m_DisamSource.Init();
	m_DisamSource.SetReadOnly(true);

	m_Splitter.SplitVertically(&m_AsmSource, &m_HSplitter, 500);
	m_Notebook = new wxNotebook(&m_HSplitter, wxID_ANY);
	m_HSplitter.SplitHorizontally(&m_DisamSource, m_Notebook, 400);
	m_AsmSource.SetFocus();

	// create tabs
	m_ImageList.Create(16, 16);
	PCWSTR names[] = { L"REGISTERS", L"MEMORY", L"BREAKPOINT" };
	for (auto name : names)
		m_ImageList.Add(wxIcon(name, wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
	m_Notebook->SetImageList(&m_ImageList);

	//
	// create registers tab
	//
	auto frame = new wxPanel(m_Notebook);
	tb = new wxToolBar(frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTB_HORZ_LAYOUT | wxTB_HORIZONTAL | wxTB_NODIVIDER);
	tb->AddCheckTool(wxID_8BITREG, wxEmptyString, wxArtProvider::GetIcon(L"8BIT", wxART_TOOLBAR));
	tb->AddCheckTool(wxID_16BITREG, wxEmptyString, wxArtProvider::GetIcon(L"16BIT", wxART_TOOLBAR));
	tb->AddCheckTool(wxID_32BITREG, wxEmptyString, wxArtProvider::GetIcon(L"32BIT", wxART_TOOLBAR))->Toggle(true);
	tb->AddCheckTool(wxID_64BITREG, wxEmptyString, wxArtProvider::GetIcon(L"64BIT", wxART_TOOLBAR));
	tb->AddSeparator();
	tb->AddTool(wxID_EDIT, wxEmptyString, wxArtProvider::GetIcon(wxART_EDIT, wxART_TOOLBAR));
	tb->Realize();

	m_RegistersList.Create(frame, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLC_REPORT | wxLC_SINGLE_SEL);
	auto sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(tb, 0, wxALIGN_TOP | wxEXPAND);
	sizer->Add(&m_RegistersList, 1, wxEXPAND);
	frame->SetSizer(sizer);

	m_RegistersList.InsertColumn(0, L"Name", wxLIST_FORMAT_LEFT, 60);
	m_RegistersList.InsertColumn(1, L"Size", wxLIST_FORMAT_RIGHT, 55);
	m_RegistersList.InsertColumn(2, L"Value", wxLIST_FORMAT_RIGHT, 150);
	m_RegistersList.InsertColumn(3, L"Type", wxLIST_FORMAT_LEFT);
	m_RegistersList.InsertColumn(4, L"Details", wxLIST_FORMAT_LEFT, 200);

	m_RegistersList.Bind(wxEVT_LIST_COL_CLICK, [this](auto& e) {
		auto col = e.GetColumn();
		auto asc = m_RegistersList.GetUpdatedAscendingSortIndicator(col);
		DoSortRegisters(col, asc);
		});

	Bind(wxEVT_MENU, [this](auto& e) {
		if (e.IsChecked())
			m_RegViewFilter |= 1 << (e.GetId() - wxID_8BITREG);
		else
			m_RegViewFilter &= ~(1 << (e.GetId() - wxID_8BITREG));
		ShowRegisters();
		}, wxID_8BITREG, wxID_64BITREG);

	m_Notebook->AddPage(frame, L"Registers", true, 0);

	auto panel = new HexViewPanel(m_Notebook);
	m_MemoryView = new wxHexView(panel);
	panel->SetHexView(m_MemoryView);

	auto font = new wxFont(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, L"Consolas");
	m_MemoryView->SetFont(*font);
	m_MemoryView->InitSharedBuffer(m_Memory.data(), (ULONG)m_Memory.size());

	sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(m_MemoryView, 1, wxEXPAND);
	panel->SetSizer(sizer);
	m_Notebook->AddPage(panel, L"Memory", false, 1);

	m_Notebook->AddPage(new wxPanel(m_Notebook), L"Breakpoints", false, 2);
}

void MainFrame::Enable(int id, bool enable) {
	GetMenuBar()->Enable(id, enable);
	GetToolBar()->EnableTool(id, enable);
}

void MainFrame::ShowRegisters() {
	m_RegistersList.DeleteAllItems();
	if (!m_Emulator.IsOpen())
		return;

	auto mode = m_Emulator.GetMode();
	int i = 0;
	for (auto& ri : AllRegisters) {
		if (((DWORD)ri.Category & (DWORD)mode) && (m_RegViewFilter & (ri.Size >> 3))) {
			auto index = m_RegistersList.InsertItem(m_RegistersList.GetItemCount(), ri.Name);
			m_RegistersList.SetItemData(index, i);
			m_RegistersList.SetItem(index, 1, wxString::Format(L"%d bit", ri.Size));
			SetRegisterValue(index, ri);
			m_RegistersList.SetItem(index, 3, Helpers::RegisterTypeToString(ri.Category));
		}
		i++;
	}
	DoSortRegisters(m_RegistersList.GetSortIndicator(), m_RegistersList.IsAscendingSortIndicator());
}

void MainFrame::RunOnThreadPool() {
	auto ok = m_Emulator.Start(m_Instructions[0].Address, m_Instructions[0].Address + m_AsmBytes.size());
	assert(ok);
	::PostMessage(this->GetHandle(), UINT(EmulatorMessage::RunComplete), 0, 0);
}

void MainFrame::Run(wxCommandEvent& e) {
	assert(m_Emulator.IsOpen());
	Enable(wxID_STOP, true);
	Enable(wxID_RUN, false);
	Enable(wxID_ASSEMBLE, false);
	m_EmulatorState = EmulatorState::Running;
	::TrySubmitThreadpoolCallback([](auto, auto p) {
		((MainFrame*)p)->RunOnThreadPool();
		}, this, nullptr);
}

void MainFrame::Stop(wxCommandEvent& e) {
	m_Emulator.Stop();
	m_EmulatorState = EmulatorState::Idle;
	UpdateEmulatorState();
	Enable(wxID_RUN, true);
	Enable(wxID_STOP, false);
	Enable(wxID_ASSEMBLE, true);
}

void MainFrame::UpdateEmulatorState() {
	for (int i = 0; i < m_RegistersList.GetItemCount(); i++) {
		auto index = m_RegistersList.GetItemData(i);
		auto& ri = AllRegisters[index];
		//auto oldValue = wcstoull(m_RegistersList.GetItemText(i, 2).ToStdWstring().c_str(), nullptr, 0);
		SetRegisterValue(i, ri);
	}
}

void MainFrame::SetRegisterValue(int i, RegisterInfo const& ri) {
	if ((ri.Category & RegisterType::Descriptor) == RegisterType::None) {
		auto value = m_Emulator.ReadReg<size_t>(ri.Id);
		m_RegistersList.SetItem(i, 2, wxString::Format(L"0x%zX", value));
		m_RegistersList.SetItem(i, 4, wxString::Format(L"%zu", value));
	}
	else {
		auto value = m_Emulator.ReadReg<uc_x86_mmr>(ri.Id);
		m_RegistersList.SetItem(i, 2, wxString::Format(L"0x%X:%llX", value.limit, value.base));
	}
	m_RegistersList.RefreshItems(0, 10);
}

void MainFrame::DoSortRegisters(int col, bool asc) {
	if (col < 0)
		return;

	struct Data {
		int Col;
		bool Asc;
		bool Sorted{ true };
	} args{ col, asc };
	m_RegistersList.SortItems([](auto i1, auto i2, auto p) {
		auto args = (Data*)p;
		auto& r1 = AllRegisters[i1];
		auto& r2 = AllRegisters[i2];
		int r = 0;
		switch (args->Col) {
			case 0: r = strcmp(r1.Name.c_str(), r2.Name.c_str()); break;
			case 1: r = r1.Size - r2.Size; break;
			case 3: r = _wcsicmp(Helpers::RegisterTypeToString(r1.Category), Helpers::RegisterTypeToString(r2.Category)); break;
			default: args->Sorted = false; break;
		}
		if (!args->Asc)
			r = -r;
		return r;
		}, (wxIntPtr)&args);
	if (args.Sorted)
		m_RegistersList.ShowSortIndicator(col, asc);

}


void MainFrame::Disassemble(std::vector<uint8_t> const& bytes) {
	Disassemble(bytes.data(), bytes.size());
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
	menuFile->Append(wxID_OPEN)->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_OPEN, wxART_MENU, size));
	auto item = menuFile->Append(wxID_SAVE);
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_SAVE, wxART_MENU, size));
	item = menuFile->Append(wxID_SAVEAS);
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(wxART_FILE_SAVE_AS, wxART_MENU, size));
	menuFile->Append(wxID_CLOSE)->Enable(false);
	menuFile->AppendSeparator();
	item = menuFile->Append(wxID_EXIT);

	// Edit menu
	auto menuEdit = new wxMenu;
	menuEdit->Append(wxID_UNDO)->SetBitmap(wxArtProvider::GetIcon(wxART_UNDO, wxART_MENU, size));
	menuEdit->Append(wxID_REDO)->SetBitmap(wxArtProvider::GetIcon(wxART_REDO, wxART_MENU, size));
	menuEdit->AppendSeparator();
	menuEdit->Append(wxID_CUT)->SetBitmap(wxArtProvider::GetIcon(wxART_CUT, wxART_MENU, size));
	menuEdit->Append(wxID_COPY)->SetBitmap(wxArtProvider::GetIcon(wxART_COPY, wxART_MENU, size));
	menuEdit->Append(wxID_PASTE)->SetBitmap(wxArtProvider::GetIcon(wxART_PASTE, wxART_MENU, size));
	menuEdit->Append(wxID_CLEAR);
	menuEdit->AppendSeparator();
	menuEdit->Append(wxID_FIND);

	auto asmMenu = new wxMenu;
	item = asmMenu->Append(wxID_ASSEMBLE, _("&Assemble\tF7"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(L"BUILD", wxART_MENU, size));
	asmMenu->AppendSeparator();
	item = asmMenu->Append(wxID_RUN, _("&Run\tF5"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(L"RUN", wxART_MENU, size));
	item = asmMenu->Append(wxID_STOP, _("&Stop\tShift+F5"));
	item->Enable(false);
	item->SetBitmap(wxArtProvider::GetIcon(L"STOP", wxART_MENU, size));

	m_MenuBar = new wxMenuBar;
	m_MenuBar->Append(menuFile, L"&File");
	m_MenuBar->Append(menuEdit, L"&Edit");
	m_MenuBar->Append(asmMenu, L"&Assembly");

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
		m_AsmBytes.clear();
		m_Instructions.clear();
		Enable(wxID_RUN, false);
	}
	else {
		if (results.Bytes.empty())
			m_AsmBytes = Helpers::ReadFileContents(results.OutputFile.c_str());
		else
			m_AsmBytes = std::move(results.Bytes);
		Disassemble(m_AsmBytes);
	}
}

void MainFrame::Disassemble(uint8_t const* data, size_t size) {
	auto mode = 1 << (1 + m_AsmModeIndex);
	CapstoneEngine cs;
	if (cs.Open((cs_mode)mode)) {
		m_Instructions = cs.Disassemble(data, size, std::wcstoll(m_AddressText->GetValue().ToStdWstring().c_str(), nullptr, 0));
		m_DisamSource.SetReadOnly(false);
		m_DisamSource.ClearAll();
		for (auto& insn : m_Instructions) {
			m_DisamSource.AppendText(wxString::Format("0x%08X %-8s %-24s ;",
				(DWORD)insn.Address, insn.Mnemonic, insn.Operands));
			for (auto b : insn.Bytes) {
				m_DisamSource.AppendText(wxString::Format(L" %02X", b));
			}
			m_DisamSource.AppendText(L"\n");
		}
		m_DisamSource.SetReadOnly(true);
		if (!m_Instructions.empty()) {
			Enable(wxID_RUN, true);
			m_Emulator.Open(CpuArch::x86, (CpuMode)mode);
			m_Emulator.MapHostMemory(0, m_Memory.size(), MemProtection::All, m_Memory.data());
			m_Emulator.WriteMemory(m_Instructions[0].Address, data, size);
			m_MemoryView->Refresh();
			m_MemoryView->ScrollTop(m_Instructions[0].Address);

			ShowRegisters();
			
		}
	}
}

HexViewPanel::HexViewPanel(wxWindow* parent) : wxPanel(parent) {
}

bool HexViewPanel::MSWOnNotify(int idCtrl, WXLPARAM lParam, WXLPARAM* result) {
	auto hdr = (LPNMHDR)lParam;
	if (hdr->hwndFrom == m_pHexView->GetHWND()) {
		switch (hdr->code) {
			case HVN_CURSOR_CHANGE:
				return true;
		}
	}
	return wxPanel::MSWOnNotify(idCtrl, lParam, result);
}

LRESULT HexViewPanel::MSWWindowProc(WXUINT msg, WXWPARAM wParam, WXLPARAM lParam) {
	switch (msg) {
		case WM_CONTEXTMENU:
			wxMenu menu;
			menu.Append(wxID_COPY)->SetBitmap(wxArtProvider::GetIcon(wxART_COPY, wxART_MENU));
			menu.Append(wxID_PASTE);
			PopupMenu(&menu);
			return 0;
	}
	return wxPanel::MSWWindowProc(msg, wParam, lParam);
}

void HexViewPanel::SetHexView(wxHexView* pHexView) {
	m_pHexView = pHexView;
}
