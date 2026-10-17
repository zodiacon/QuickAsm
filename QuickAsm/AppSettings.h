#pragma once

#include <Settings.h>
#include <WTLHelper.h>

class AppSettings : public Settings {
public:
	BEGIN_SETTINGS(AppSettings)
		SETTING(Theme, (int)DarkModeKind::System, SettingType::Int32);	// DarkModeKind: Light, Dark or System
		SETTING(ViewStatusBar, 1, SettingType::Bool);
		SETTING(Assembler, 1, SettingType::Int32);	// index: Keystone, NASM
		SETTING(AsmMode, 1, SettingType::Int32);	// 16, 32 or 64 bit
		SETTING_STRING(Address, L"0x1000");
		SETTING_STRING(FontFace, L"Consolas");	// the editors'
		SETTING(FontSize, 12, SettingType::Int32);	// points
		SETTING(AutoComplete, 1, SettingType::Bool);
		SETTING(DisasmSyntax, 0, SettingType::Int32);	// 0: Intel, 1: AT&T
		SETTING(DisasmBytes, 1, SettingType::Bool);
		SETTING(DisasmUpperCase, 0, SettingType::Bool);
		SETTING(MemoryBase, 0ull, SettingType::Int64);	// the emulator's memory
		SETTING(MemorySize, 64, SettingType::Int32);	// KB
		SETTING(StackSize, 4, SettingType::Int32);		// KB, at the top of the memory
	END_SETTINGS

	DEF_SETTING(Theme, int)
	DEF_SETTING(ViewStatusBar, bool)
	DEF_SETTING(Assembler, int)
	DEF_SETTING(AsmMode, int)
	DEF_SETTING_STRING(Address)
	DEF_SETTING_STRING(FontFace)
	DEF_SETTING(FontSize, int)
	DEF_SETTING(AutoComplete, bool)
	DEF_SETTING(DisasmSyntax, int)
	DEF_SETTING(DisasmBytes, bool)
	DEF_SETTING(DisasmUpperCase, bool)
	DEF_SETTING(MemoryBase, uint64_t)
	DEF_SETTING(MemorySize, int)
	DEF_SETTING(StackSize, int)
	DEF_SETTING_MULTI(RecentFiles)		// the last first

	DarkModeKind ThemeKind() const {
		return static_cast<DarkModeKind>(Theme());
	}
	// the theme to apply: System resolved to Dark or Light
	DarkModeKind EffectiveTheme() const {
		auto kind = ThemeKind();
		if (kind == DarkModeKind::System)
			return WTLHelper::IsSystemInDarkMode() ? DarkModeKind::Dark : DarkModeKind::Light;
		return kind;
	}
};
