#pragma once

#include <Settings.h>
#include <WTLHelper.h>

class AppSettings : public Settings {
public:
	BEGIN_SETTINGS(AppSettings)
		SETTING(Theme, (int)DarkModeKind::System, SettingType::Int32);	// DarkModeKind: Light, Dark or System
		SETTING(ViewStatusBar, 1, SettingType::Bool);
	END_SETTINGS

	DEF_SETTING(Theme, int)
	DEF_SETTING(ViewStatusBar, bool)
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
