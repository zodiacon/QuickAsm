// the emulator: running, stepping and breakpoints
//

#include "pch.h"
#include "MainFrm.h"
#include "BreakpointDlg.h"

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
	UpdateStackView();
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
	// (one that came too late, after the code ended or paused anyway, is forgotten)
	m_BreakRequested = false;
	// (read while the emulator thread waits in the code hook, or before it starts)
	if (stepOut)
		m_StepOutSP = ReadStackPointer();

	if (m_EmulatorState == EmulatorState::Paused) {
		ShowCurrentLine(-1);
		m_EmulatorState = EmulatorState::Running;
		m_RegistersView.SetLive(false);
		m_MemoryView.TakeSnapshot();
		UpdateStackView();
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

	{
		// (the hit counts are of a run)
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [address, bp] : m_Breakpoints)
			bp.Hits = 0;
	}
	m_StopRequested = false;
	m_LastWasReturn = false;
	m_ContinueEvent.ResetEvent();
	m_StopEvent.ResetEvent();
	m_CodeRan = true;
	m_EmulatorState = EmulatorState::Running;
	m_RegistersView.SetLive(false);
	m_MemoryView.TakeSnapshot();
	UpdateStackView();
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
	bool stop = m_StepInto || m_StopAt == address || m_BreakRequested.exchange(false);
	// step out: the first instruction after a return that left the function (the stack is above where it was)
	if (!stop && m_StepOut && m_LastWasReturn && ReadStackPointer() > m_StepOutSP)
		stop = true;
	m_LastWasReturn = IsReturn(address);
	// (a breakpoint counts its hit even when a step stops here anyway)
	bool error = false;
	{
		std::lock_guard lock(m_BreakpointsLock);
		if (auto it = m_Breakpoints.find(address); it != m_Breakpoints.end() && CheckBreakpoint(it->second, error))
			stop = true;
	}
	if (!stop)
		return;

	m_StopAt = 0;
	m_StepOut = false;
	PostMessage(WM_EMULATOR_PAUSED, error, (LPARAM)address);
	HANDLE events[] = { m_ContinueEvent.get(), m_StopEvent.get() };
	if (::WaitForMultipleObjects(_countof(events), events, FALSE, INFINITE) == WAIT_OBJECT_0 + 1)
		m_Emulator.Stop();
}

bool CMainFrame::CheckBreakpoint(Breakpoint& bp, bool& error) {
	if (!bp.Enabled)
		return false;

	if (bp.Expr) {
		// the registers and the memory as they are before the instruction runs
		struct Context : IExpressionContext {
			CMainFrame& Frame;
			explicit Context(CMainFrame& frame) : Frame(frame) {}

			bool ReadRegister(x86Register reg, uint64_t& value) override {
				return Frame.m_Emulator.ReadReg(reg, &(value = 0));
			}
			bool ReadMemory(uint64_t address, int size, uint64_t& value) override {
				if (size == 0)
					size = 2 << (int)Frame.m_AsmMode;
				if (address >= Frame.m_Memory.size() || Frame.m_Memory.size() - address < (uint64_t)size)
					return false;
				value = 0;
				::memcpy(&value, Frame.m_Memory.data() + address, size);
				return true;
			}
		} context(*this);

		uint64_t value;
		if (!bp.Expr->Evaluate(context, value)) {
			error = true;
			return true;
		}
		if (value == 0)
			return false;
	}
	return ++bp.Hits >= bp.HitTarget;
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

void CMainFrame::UpdateStackView() {
	// (nothing while the code runs: the stack pointer can't be read then)
	if (!m_Emulator.IsOpen() || m_EmulatorState == EmulatorState::Running) {
		m_StackView.SetEntries({}, 4);
		return;
	}

	int size = 2 << (int)m_AsmMode;
	auto sp = ReadStackPointer();
	uint64_t bp;
	PCWSTR spName, bpName;
	switch (m_AsmMode) {
		case AsmMode::Bits16: bp = m_Emulator.ReadReg<uint16_t>(x86Register::BP); spName = L"SP"; bpName = L"BP"; break;
		case AsmMode::Bits32: bp = m_Emulator.ReadReg<uint32_t>(x86Register::EBP); spName = L"ESP"; bpName = L"EBP"; break;
		default: bp = m_Emulator.ReadReg<uint64_t>(x86Register::RBP); spName = L"RSP"; bpName = L"RBP"; break;
	}

	std::vector<StackEntry> entries;
	for (auto address = sp; address + size <= m_Memory.size() && entries.size() < 256; address += size) {
		StackEntry entry{ address, 0 };
		::memcpy(&entry.Value, m_Memory.data() + address, size);
		CString notes;
		if (address == sp)
			notes = spName;
		if (address == bp)
			notes += CString(notes.IsEmpty() ? L"" : L", ") + bpName;
		// an address in the code: after a call, where it returns to
		if (auto line = LineFromAddress(entry.Value); line >= 0) {
			CString what;
			if (line > 0 && m_Instructions[line - 1].Id == X86_INS_CALL)
				what.Format(L"Return address (line %d)", line + 1);
			else
				what.Format(L"Code (line %d)", line + 1);
			notes += CString(notes.IsEmpty() ? L"" : L", ") + what;
		}
		entry.Notes = notes;
		entries.push_back(std::move(entry));
	}
	m_StackView.SetEntries(std::move(entries), size);
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
	UpdateStackView();
	m_MemoryView.MarkChanges();
	// (the hit counts)
	m_BreakpointsView.Refresh();
}

LRESULT CMainFrame::OnEmulatorPaused(UINT, WPARAM wParam, LPARAM lParam, BOOL&) {
	if (m_EmulatorState != EmulatorState::Running)
		return 0;

	m_EmulatorState = EmulatorState::Paused;
	auto address = (uint64_t)lParam;
	auto line = LineFromAddress(address);
	ShowEmulatorState(line);
	CString text;
	text.Format(L"Paused at 0x%llX", address);
	if (wParam)
		text += L" (the breakpoint's condition couldn't be evaluated)";
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

LRESULT CMainFrame::OnBreak(WORD, WORD, HWND, BOOL&) {
	// (the code hook pauses at the next instruction)
	if (m_EmulatorState == EmulatorState::Running)
		m_BreakRequested = true;
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

	auto line = GetTargetLine();
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
			m_Breakpoints[address] = {};
	}
	BreakpointsChanged();
}

void CMainFrame::BreakpointsChanged() {
	m_Disassembly.DeleteBreakpointMarkers();
	m_AsmSource.DeleteBreakpointMarkers();
	// (in the source too, while its lines are the code's)
	bool source = !m_AddressToSourceLine.empty() && m_AsmSource.GetAllText() == m_AssembledSource;
	{
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [address, bp] : m_Breakpoints) {
			bool conditional = bp.Expr || bp.HitTarget > 1;
			if (auto line = LineFromAddress(address); line >= 0)
				m_Disassembly.SetBreakpointMarker(line, true, bp.Enabled, conditional);
			if (auto it = m_AddressToSourceLine.find(address); source && it != m_AddressToSourceLine.end())
				m_AsmSource.SetBreakpointMarker(it->second, true, bp.Enabled, conditional);
		}
	}
	m_BreakpointsView.Refresh();
}

int CMainFrame::DisassemblyLineFromSourceLine(int sourceLine) {
	if (m_AddressToSourceLine.empty() || m_AsmSource.GetAllText() != m_AssembledSource)
		return -1;
	// the first instruction of the line (a macro's line has several)
	int first = -1;
	for (auto& [address, line] : m_AddressToSourceLine)
		if (line == sourceLine)
			if (auto dline = LineFromAddress(address); dline >= 0 && (first < 0 || dline < first))
				first = dline;
	return first;
}

int CMainFrame::GetTargetLine() {
	if (::GetFocus() != m_AsmSource)
		return GetDisassemblyCaretLine();

	auto line = DisassemblyLineFromSourceLine((int)m_AsmSource.LineFromPosition(m_AsmSource.GetCurrentPos()));
	if (line < 0)
		m_StatusBar.SetText(0, m_AsmSource.GetAllText() == m_AssembledSource ? L"No code at this line" : L"The source changed: assemble it again");
	return line;
}

std::vector<BreakpointInfo> CMainFrame::GetBreakpoints() const {
	std::vector<BreakpointInfo> breakpoints;
	std::lock_guard lock(m_BreakpointsLock);
	for (auto& [address, bp] : m_Breakpoints) {
		auto line = LineFromAddress(address);
		std::string text;
		if (line >= 0) {
			auto& insn = m_Instructions[line];
			text = insn.Mnemonic + " " + insn.Operands;
		}
		breakpoints.push_back({ address, line, bp.Enabled, std::move(text), bp.Condition, bp.HitTarget, bp.Hits });
	}
	return breakpoints;
}

void CMainFrame::EnableBreakpoint(uint64_t address, bool enable) {
	{
		std::lock_guard lock(m_BreakpointsLock);
		if (auto it = m_Breakpoints.find(address); it != m_Breakpoints.end())
			it->second.Enabled = enable;
	}
	BreakpointsChanged();
}

void CMainFrame::EnableAllBreakpoints(bool enable) {
	{
		std::lock_guard lock(m_BreakpointsLock);
		for (auto& [address, bp] : m_Breakpoints)
			bp.Enabled = enable;
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
	ToggleBreakpoint(GetTargetLine());
	return 0;
}

LRESULT CMainFrame::OnEnableBreakpoint(WORD, WORD, HWND, BOOL&) {
	auto line = GetTargetLine();
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
		enabled = it->second.Enabled;
	}
	EnableBreakpoint(address, !enabled);
	return 0;
}

LRESULT CMainFrame::OnBreakpointCondition(WORD, WORD, HWND, BOOL&) {
	auto line = GetTargetLine();
	if (line < 0 || line >= (int)m_Instructions.size()) {
		::MessageBeep(MB_ICONWARNING);
		return 0;
	}
	EditBreakpoint(m_Instructions[line].Address, true);
	return 0;
}

void CMainFrame::EditBreakpoint(uint64_t address) {
	EditBreakpoint(address, false);
}

void CMainFrame::EditBreakpoint(uint64_t address, bool create) {
	CBreakpointDlg dlg;
	{
		std::lock_guard lock(m_BreakpointsLock);
		auto it = m_Breakpoints.find(address);
		if (it == m_Breakpoints.end() && !create)
			return;
		if (it != m_Breakpoints.end()) {
			dlg.Condition = it->second.Condition;
			dlg.HitTarget = it->second.HitTarget;
		}
	}
	dlg.Title.Format(L"Breakpoint at 0x%llX (line %d)", address, LineFromAddress(address) + 1);
	if (dlg.DoModal(m_hWnd) != IDOK)
		return;

	{
		// (the dialog parsed it: it's valid)
		std::lock_guard lock(m_BreakpointsLock);
		auto& bp = m_Breakpoints[address];
		bp.Condition = dlg.Condition;
		bp.Expr = std::move(dlg.Expr);
		bp.HitTarget = dlg.HitTarget;
	}
	BreakpointsChanged();
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

LRESULT CMainFrame::OnSourceMarginClick(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	if (scn->margin != 1)
		return 0;
	auto line = DisassemblyLineFromSourceLine((int)m_AsmSource.LineFromPosition(scn->position));
	if (line < 0) {
		::MessageBeep(MB_ICONWARNING);
		m_StatusBar.SetText(0, m_AsmSource.GetAllText() == m_AssembledSource ? L"No code at this line" : L"The source changed: assemble it again");
		return 0;
	}
	ToggleBreakpoint(line);
	return 0;
}

LRESULT CMainFrame::OnDisassemblyMarginClick(int, LPNMHDR pnmh, BOOL&) {
	auto scn = (SCNotification*)pnmh;
	if (scn->margin == 1)
		ToggleBreakpoint((int)m_Disassembly.LineFromPosition(scn->position));
	return 0;
}
