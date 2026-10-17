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
	m_Disassembly.SetCurrentLine(-1);
	m_CurrentLine = -1;
	if (!keepBreakpoints) {
		std::lock_guard lock(m_BreakpointsLock);
		m_Breakpoints.clear();
		m_Disassembly.MarkerDeleteAll(CAsmEditor::BreakpointMarker);
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
	m_MemoryView.Refresh();
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

void CMainFrame::Run(bool stepInto, uint64_t stopAt) {
	m_StepInto = stepInto;
	m_StopAt = stopAt;

	if (m_EmulatorState == EmulatorState::Paused) {
		m_Disassembly.SetCurrentLine(-1);
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
	if (!stop) {
		std::lock_guard lock(m_BreakpointsLock);
		stop = m_Breakpoints.contains(address);
	}
	if (!stop)
		return;

	m_StopAt = 0;
	PostMessage(WM_EMULATOR_PAUSED, 0, (LPARAM)address);
	HANDLE events[] = { m_ContinueEvent.get(), m_StopEvent.get() };
	if (::WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		m_Emulator.Stop();
}

void CMainFrame::StopEmulator() {
	if (m_EmulatorState == EmulatorState::Idle)
		return;

	m_StopRequested = true;
	if (m_EmulatorState == EmulatorState::Paused)
		::SetEvent(m_StopEvent.get());
	else
		m_Emulator.Stop();
	if (m_EmulatorWork)
		::WaitForThreadpoolWorkCallbacks(m_EmulatorWork.get(), FALSE);
	// (its message, posted when it ended, finds the emulator idle)
	m_EmulatorState = EmulatorState::Idle;
	m_RegistersView.SetLive(true);
	m_Disassembly.SetCurrentLine(-1);
	m_StatusBar.SetText(1, L"Idle");
}

void CMainFrame::ShowEmulatorState(int line) {
	m_CurrentLine = line;
	m_Disassembly.SetCurrentLine(line);
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
	ShowEmulatorState(-1);

	auto error = (uc_err)wParam;
	if (m_RestartPending) {
		m_RestartPending = false;
		Run();
		return 0;
	}

	if (error != UC_ERR_OK && !m_StopRequested) {
		auto pc = m_Emulator.ReadReg<uint64_t>(m_AsmMode == AsmMode::Bits64 ? x86Register::RIP : x86Register::EIP);
		CString text;
		text.Format(L"Execution error at 0x%llX: %S", pc, m_Emulator.GetErrorText(error));
		m_StatusBar.SetText(1, L"Error");
		AtlMessageBox(m_hWnd, (PCWSTR)text, IDR_MAINFRAME, MB_ICONERROR);
		ShowEmulatorState(LineFromAddress(pc));
		return 0;
	}
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

	m_StopRequested = true;
	if (m_EmulatorState == EmulatorState::Paused)
		::SetEvent(m_StopEvent.get());
	else
		m_Emulator.Stop();
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

void CMainFrame::ToggleBreakpoint(int line) {
	if (line < 0 || line >= (int)m_Instructions.size()) {
		::MessageBeep(MB_ICONWARNING);
		return;
	}

	auto address = m_Instructions[line].Address;
	auto set = m_Disassembly.ToggleBreakpoint(line);
	std::lock_guard lock(m_BreakpointsLock);
	if (set)
		m_Breakpoints.insert(address);
	else
		m_Breakpoints.erase(address);
}

LRESULT CMainFrame::OnToggleBreakpoint(WORD, WORD, HWND, BOOL&) {
	ToggleBreakpoint((int)m_Disassembly.LineFromPosition(m_Disassembly.GetCurrentPos()));
	return 0;
}

LRESULT CMainFrame::OnDeleteAllBreakpoints(WORD, WORD, HWND, BOOL&) {
	m_Disassembly.MarkerDeleteAll(CAsmEditor::BreakpointMarker);
	std::lock_guard lock(m_BreakpointsLock);
	m_Breakpoints.clear();
	return 0;
}

LRESULT CMainFrame::OnDisassemblyMarginClick(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	if (scn->margin == 1)
		ToggleBreakpoint((int)m_Disassembly.LineFromPosition(scn->position));
	return 0;
}
