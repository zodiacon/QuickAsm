#pragma once

// sent by the main frame to its descendants after the theme changed (WTLHelper::IsDarkMode tells the new one)
const UINT WM_UPDATE_DARKMODE = WM_APP + 56;

struct BreakpointInfo {
	uint64_t Address;
	int Line;			// in the disassembly (0 based)
	bool Enabled;
	std::string Instruction;
};

// the breakpoints, as the main frame keeps them (for the breakpoints view)
struct IBreakpointsHost {
	// sorted by address
	virtual std::vector<BreakpointInfo> GetBreakpoints() const = 0;
	virtual void EnableBreakpoint(uint64_t address, bool enable) = 0;
	virtual void EnableAllBreakpoints(bool enable) = 0;
	virtual void DeleteBreakpoint(uint64_t address) = 0;
	virtual void DeleteAllBreakpoints() = 0;
	// shows its line in the disassembly
	virtual void GoToBreakpoint(uint64_t address) = 0;
};
