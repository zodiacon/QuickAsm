#pragma once

#include "resource.h"
#include <SimpleHexControl.h>

// the emulator's memory (mapped from the host), which has a fixed size: bytes are overwritten, never inserted or deleted
class CFixedMemoryBuffer : public IBufferManager {
public:
	explicit CFixedMemoryBuffer(std::vector<uint8_t>& memory) : m_Memory(memory) {}

	std::vector<uint8_t> const& GetMemory() const {
		return m_Memory;
	}

	uint32_t GetData(int64_t offset, uint8_t* buffer, uint32_t count) override;
	bool Insert(int64_t, const uint8_t*, uint32_t) override {
		return false;
	}
	bool Delete(int64_t, size_t) override {
		return false;
	}
	bool SetData(int64_t offset, const uint8_t* data, uint32_t count) override;
	bool SetData(int64_t offset, uint8_t value, uint32_t count) override;
	int64_t GetSize() const override {
		return (int64_t)m_Memory.size();
	}
	uint8_t* GetRawData(int64_t offset) override {
		return m_Memory.data() + offset;
	}
	bool IsReadOnly() const override {
		return false;
	}

private:
	std::vector<uint8_t>& m_Memory;
};

class CMemoryView : public CWindowImpl<CMemoryView> {
public:
	DECLARE_WND_CLASS_EX(L"QuickAsmMemoryView", 0, COLOR_WINDOW)

	explicit CMemoryView(std::vector<uint8_t>& memory) : m_Buffer(memory) {}

	CHexControl& GetHexControl() {
		return m_Hex;
	}
	BOOL PreTranslateMessage(MSG* pMsg);
	// the memory (after it may have changed)
	void Refresh();
	// the memory was set up again: what was changed before (and could be undone) is gone
	void Reset();
	// the memory to compare with at the next stop: before the code runs
	void TakeSnapshot();
	// the bytes the code changed since the snapshot are highlighted: at a stop
	void MarkChanges();
	void GotoAddress(uint64_t address);
	// the colors of the current theme (light or dark)
	void ApplyTheme();

	BEGIN_MSG_MAP(CMemoryView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
		MESSAGE_HANDLER(WM_GOTO_PENDING, OnGotoPending)
		COMMAND_RANGE_HANDLER(ID_MEMORY_1BYTE, ID_MEMORY_8BYTES, OnDataSize)
	END_MSG_MAP()

private:
	static const UINT WM_GOTO_PENDING = WM_APP + 1;

	void GotoPending();
	void HighlightChanges();

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnGotoPending(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDataSize(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	CFixedMemoryBuffer m_Buffer;
	CHexControl m_Hex;
	// shown once the view has a size (scrolling to it before does nothing)
	int64_t m_PendingAddress{ -1 };
	std::vector<uint8_t> m_Snapshot;
	std::vector<std::pair<int64_t, int64_t>> m_Changes;	// offset, length
	CToolBarCtrl m_ToolBar;
};
