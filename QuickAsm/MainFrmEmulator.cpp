// the emulator: running, stepping and breakpoints
//

#include "pch.h"
#include "MainFrm.h"

void CMainFrame::InitEmulator(uint8_t const* data, size_t size, bool keepBreakpoints) {
	StopEmulator();
	if (m_CodeHook) {
		m_Emulator.Unhook(m_CodeHook);
		m_CodeHook = nullptr;
	}
	m_Emulator.Close();
	m_CodeRan = false;
	ShowCurrentLine(-1);
	m_CurrentLine = -1;
	if (!keepBreakpoints) {
		{
			std::lock_guard lock(m_BreakpointsLock);
			m_Breakpoints.clear();
		}
		BreakpointsChanged();
	}

	m_AddressToLine.clear();
	for (int i = 0; i < (int)m_Instructions.size(); i++)
		m_AddressToLine[m_Instructions[i].Address] = i;

	if (!m_Instructions.empty() && m_Emulator.Open(CpuArch::x86, (CpuMode)(1 << (1 + (int)m_AsmMode)))) {
		std::ranges::fill(m_Memory, uint8_t(0));
		m_Emulator.MapHostMemory(0, m_Memory.size(), MemProtection::All, m_Memory.data());
		m_Emulator.WriteMemory(m_Instructions[0].Address, data, size);
		// the stack at the top of the memory (a push at 0 would fault)
		switch (m_AsmMode) {
			case AsmMode::Bits16: m_Emulator.WriteReg(x86Register::SP, uint16_t(m_Memory.size() - 2)); break;
			case AsmMode::Bits32: m_Emulator.WriteReg(x86Register::ESP, uint32_t(m_Memory.size())); break;
			case AsmMode::Bits64: m_Emulator.WriteReg(x86Register::RSP, uint64_t(m_Memory.size())); break;
		}
		m_CodeHook = m_Emulator.HookCode(HookType::CODE, [this](auto address, auto) { OnCodeHook(address); },
			m_Instructions[0].Address, m_Instructions[0].Address + size - 1);
	}
	m_RegistersView.SetLive(true);
	m_RegistersView.Refresh();
	m_RegistersView.ResetChanges();
	m_MemoryView.Reset();
	if (!m_Instructions.empty())
		m_MemoryView.GotoAddress(m_Instructions[0].Address);
}

bool CMainFrame::IsEmulatorBusy() const {
	return m_EmulatorState != EmulatorState::Idle;
}

int CMainFrame::LineFromAddress(uint64_t address) const {
	auto it = m_AddressToLine.find(address);
	return it == m_AddressToLine.end() ? -1 : it->second;
}

uint64_t CMainFrame::ReadStackPointer() const {
	switch (m_AsmMode) {
		case AsmMode::Bits16: return m_Emulator.ReadReg<uint16_t>(x86Register::SP);
		case AsmMode::Bits32: return m_Emulator.ReadReg<uint32_t>(x86Register::ESP);
	}
	return m_Emulator.ReadReg<uint64_t>(x86Register::RSP);
}

bool CMainFrame::IsReturn(uint64_t address) const {
	auto line = LineFromAddress(address);
	if (line < 0)
		return false;
	auto id = m_Instructions[line].Id;
	return id == X86_INS_RET || id == X86_INS_RETF || id == X86_INS_RETFQ;
}

void CMainFrame::Run(bool stepInto, uint64_t stopAt, bool stepOut) {
	m_StepInto = stepInto;
	m_StopAt = stopAt;
	m_StepOut = stepOut;
	// (read while the emulator thread waits in the code hook, or before it starts)
	if (stepOut)
		m_StepOutSP = ReadStackPointer();

	if (m_EmulatorState == EmulatorState::Paused) {
		ShowCurrentLine(-1);
		m_EmulatorState = EmulatorState::Running;
		m_RegistersView.SetLive(false);
		m_StatusBar.SetText(1, L"Running");
		::SetEvent(m_ContinueEvent.get());
		return;
	}

	ATLASSERT(m_EmulatorState == EmulatorState::Idle);
	if (m_Instructions.empty() || !m_Emulator.IsOpen())
		return;

	if (m_CodeRan) {
		// a new run: the code (which may have changed itself), the registers and the memory as they were set up
		auto bytes = m_AsmBytes;
		InitEmulator(bytes.data(), bytes.size(), true);
	}

	m_StopRequested = false;
	m_LastWasReturn = false;
	m_ContinueEvent.ResetEvent();
	m_StopEvent.ResetEvent();
	m_CodeRan = true;
	m_EmulatorState = EmulatorState::Running;
	m_RegistersView.SetLive(false);
	m_StatusBar.SetText(1, L"Running");

	if (!m_EmulatorWork) {
		m_EmulatorWork.reset(::CreateThreadpoolWork([](auto, auto context, auto) {
			auto frame = static_cast<CMainFrame*>(context);
			auto start = frame->m_Instructions[0].Address;
			auto ok = frame->m_Emulator.Start(start, start + frame->m_AsmBytes.size());
			frame->PostMessage(WM_EMULATOR_DONE, ok ? UC_ERR_OK : frame->m_Emulator.LastError());
			}, this, nullptr));
	}
	::SubmitThreadpoolWork(m_EmulatorWork.get());
}

// called on the emulator thread before each instruction runs
void CMainFrame::OnCodeHook(uint64_t address) {
	bool stop = m_StepInto || m_StopAt == address;
	// step out: the first instruction after a return that left the function (the stack is above where it was)
	if (!stop && m_StepOut && m_LastWasReturn && ReadStackPointer() > m_StepOutSP)
		stop = true;
	m_LastWasReturn = IsReturn(address);
	if (!stop) {
		std::lock_guard lock(m_BreakpointsLock);
		auto it = m_Breakpoints.find(address);
		stop = it != m_Breakpoints.end() && it->second;
	}
	if (!stop)
		return;

	m_StopAt = 0;
	m_StepOut = false;
	PostMessage(WM_EMULATOR_PAUSED, 0, (LPARAM)address);
	HANDLE events[] = { m_ContinueEvent.get(), m_StopEvent.get() };
	if (::WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		m_Emulator.Stop();
}

void CMainFrame::RequestStop() {
	// both, whatever the state seems to be: the emulator thread may be waiting in the code hook already,
	// before its message (that it paused) arrives
	m_StopRequested = true;
	::SetEvent(m_StopEvent.get());
	m_Emulator.Stop();
}

void CMainFrame::StopEmulator() {
	if (m_EmulatorState == EmulatorState::Idle)
		return;

	RequestStop();
	if (m_EmulatorWork)
		::WaitForThreadpoolWorkCallbacks(m_EmulatorWork.get(), FALSE);
	// (its message, posted when it ended, finds the emulator idle)
	m_EmulatorState = EmulatorState::Idle;
	m_RegistersView.SetLive(true);
	ShowCurrentLine(-1);
	m_StatusBar.SetText(1, L"Idle");
}

void CMainFrame::ShowCurrentLine(int line) {
	m_Disassembly.SetCurrentLine(line);
	m_AsmSource.SetCurrentLine(SourceLineFromDisassemblyLine(line));
}

void CMainFrame::ShowEmulatorState(int line) {
	m_CurrentLine = line;
	ShowCurrentLine(line);
	m_RegistersView.MarkChanges();
	m_RegistersView.SetLive(true);
	m_MemoryView.Refresh();
}

LRESULT CMainFrame::OnEmulatorPaused(UINT, WPARAM, LPARAM lParam, BOOL&) {
	if (m_EmulatorState != EmulatorState::Running)
		return 0;

	m_EmulatorState = EmulatorState::Paused;
	auto address = (uint64_t)lParam;
	auto line = LineFromAddress(address);
	ShowEmulatorState(line);
	CString text;
	text.Format(L"Paused at 0x%llX", address);
	m_StatusBar.SetText(1, text);
	return 0;
}

LRESULT CMainFrame::OnEmulatorDone(UINT, WPARAM wParam, LPARAM, BOOL&) {
	if (m_EmulatorState == EmulatorState::Idle)
		return 0;

	if (m_EmulatorWork)
		::WaitForThreadpoolWorkCallbacks(m_EmulatorWork.get(), FALSE);
	m_EmulatorState = EmulatorState::Idle;

	auto error = (uc_err)wParam;
	if (m_RestartPending) {
		m_RestartPending = false;
		ShowEmulatorState(-1);
		Run();
		return 0;
	}

	if (error != UC_ERR_OK && !m_StopRequested) {
		auto pc = m_Emulator.ReadReg<uint64_t>(m_AsmMode == AsmMode::Bits64 ? x86Register::RIP : x86Register::EIP);
		ShowEmulatorState(LineFromAddress(pc));
		CString text;
		text.Format(L"Execution error at 0x%llX: %S", pc, m_Emulator.GetErrorText(error));
		m_StatusBar.SetText(1, L"Error");
		AtlMessageBox(m_hWnd, (PCWSTR)text, IDR_MAINFRAME, MB_ICONERROR);
		return 0;
	}
	ShowEmulatorState(-1);
	m_StatusBar.SetText(1, m_StopRequested ? L"Stopped" : L"Completed");
	return 0;
}

LRESULT CMainFrame::OnRun(WORD, WORD, HWND, BOOL&) {
	if (m_EmulatorState != EmulatorState::Running)
		Run();
	return 0;
}

LRESULT CMainFrame::OnStop(WORD, WORD, HWND, BOOL&) {
	if (!IsEmulatorBusy())
		return 0;

	RequestStop();
	// (the emulator thread ends, and its message makes the state idle)
	return 0;
}

LRESULT CMainFrame::OnRestart(WORD, WORD, HWND, BOOL&) {
	if (!IsEmulatorBusy()) {
		Run();
		return 0;
	}
	m_RestartPending = true;
	BOOL handled;
	return OnStop(0, 0, nullptr, handled);
}

LRESULT CMainFrame::OnStepInto(WORD, WORD, HWND, BOOL&) {
	if (m_EmulatorState != EmulatorState::Running)
		Run(true);
	return 0;
}

LRESULT CMainFrame::OnStepOut(WORD, WORD, HWND, BOOL&) {
	if (m_EmulatorState == EmulatorState::Paused)
		Run(false, 0, true);
	return 0;
}

LRESULT CMainFrame::OnRunToCursor(WORD, WORD, HWND, BOOL&) {
	if (m_EmulatorState == EmulatorState::Running)
		return 0;

	auto line = GetDisassemblyCaretLine();
	if (line < 0 || line >= (int)m_Instructions.size()) {
		::MessageBeep(MB_ICONWARNING);
		return 0;
	}
	Run(false, m_Instructions[line].Address);
	return 0;
}

LRESULT CMainFrame::OnStepOver(WORD, WORD, HWND, BOOL&) {
	if (m_EmulatorState == EmulatorState::Running)
		return 0;

	// a call runs to the instruction after it; anything else is a single step
	if (m_EmulatorState == EmulatorState::Paused && m_CurrentLine >= 0) {
		auto& insn = m_Instructions[m_CurrentLine];
		if (insn.Id == X86_INS_CALL) {
			Run(false, insn.Address + insn.Bytes.size());
			return 0;
		}
	}
	Run(true);
	return 0;
}

int CMainFrame::GetDisassemblyCaretLine() const {
	return (int)m_Disassembly.LineFromPosition(m_Disassembly.GetCurrentPos());
}

void CMainFrame::ToggleBreakpoint(int line) {
	if (line < 0 || line >= (int)m_Instructions.size()) {
		::MessageBeep(MB_ICONWARNING);
		return;
	}

	auto address = m_Instructions[line].Address;
	{
		std::lock_guard lock(m_BreakpointsLock);
		if (!m_Breakpoints.erase(address))
			m_Breakpoints[address] = true;
	}
	BreakpointsChanged();
}

void CMainFrame::BreakpointsChanged() {
	m_Disassembly.DeleteBreakpointMarkers();
	{
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [address, enabled] : m_Breakpoints)
			if (auto line = LineFromAddress(address); line >= 0)
				m_Disassembly.SetBreakpointMarker(line, true, enabled);
	}
	m_BreakpointsView.Refresh();
}

std::vector<BreakpointInfo> CMainFrame::GetBreakpoints() const {
	std::vector<BreakpointInfo> breakpoints;
	std::lock_guard lock(m_BreakpointsLock);
	for (auto& [address, enabled] : m_Breakpoints) {
		auto line = LineFromAddress(address);
		std::string text;
		if (line >= 0) {
			auto& insn = m_Instructions[line];
			text = insn.Mnemonic + " " + insn.Operands;
		}
		breakpoints.push_back({ address, line, enabled, std::move(text) });
	}
	return breakpoints;
}

void CMainFrame::EnableBreakpoint(uint64_t address, bool enable) {
	{
		std::lock_guard lock(m_BreakpointsLock);
		if (auto it = m_Breakpoints.find(address); it != m_Breakpoints.end())
			it->second = enable;
	}
	BreakpointsChanged();
}

void CMainFrame::EnableAllBreakpoints(bool enable) {
	{
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [address, enabled] : m_Breakpoints)
			enabled = enable;
	}
	BreakpointsChanged();
}

void CMainFrame::DeleteBreakpoint(uint64_t address) {
	{
		std::lock_guard lock(m_BreakpointsLock);
		m_Breakpoints.erase(address);
	}
	BreakpointsChanged();
}

void CMainFrame::DeleteAllBreakpoints() {
	{
		std::lock_guard lock(m_BreakpointsLock);
		m_Breakpoints.clear();
	}
	BreakpointsChanged();
}

void CMainFrame::GoToBreakpoint(uint64_t address) {
	if (auto line = LineFromAddress(address); line >= 0) {
		m_Disassembly.GotoLine(line);
		m_Disassembly.SetFocus();
	}
}

LRESULT CMainFrame::OnToggleBreakpoint(WORD, WORD, HWND, BOOL&) {
	ToggleBreakpoint(GetDisassemblyCaretLine());
	return 0;
}

LRESULT CMainFrame::OnEnableBreakpoint(WORD, WORD, HWND, BOOL&) {
	auto line = GetDisassemblyCaretLine();
	if (line < 0 || line >= (int)m_Instructions.size())
		return 0;

	auto address = m_Instructions[line].Address;
	bool enabled;
	{
		std::lock_guard lock(m_BreakpointsLock);
		auto it = m_Breakpoints.find(address);
		if (it == m_Breakpoints.end()) {
			::MessageBeep(MB_ICONWARNING);
			return 0;
		}
		enabled = it->second;
	}
	EnableBreakpoint(address, !enabled);
	return 0;
}

LRESULT CMainFrame::OnDeleteAllBreakpoints(WORD, WORD, HWND, BOOL&) {
	DeleteAllBreakpoints();
	return 0;
}

LRESULT CMainFrame::OnDisassemblyDoubleClick(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	// an assembly error ("<line>: ...") or an instruction shows its line in the source
	auto line = m_Instructions.empty() ? ParseErrorLine(m_Disassembly.GetLine(scn->line)) : SourceLineFromDisassemblyLine((int)scn->line);
	if (line >= 0 && line < (int)m_AsmSource.Execute(SCI_GETLINECOUNT)) {
		m_AsmSource.GotoLine(line);
		m_AsmSource.SetFocus();
	}
	return 0;
}

LRESULT CMainFrame::OnDisassemblyMarginClick(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	if (scn->margin == 1)
		ToggleBreakpoint((int)m_Disassembly.LineFromPosition(scn->position));
	return 0;
}
