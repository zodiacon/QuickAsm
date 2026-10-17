#pragma once

// sent by the main frame to its descendants after the theme changed (WTLHelper::IsDarkMode tells the new one)
const UINT WM_UPDATE_DARKMODE = WM_APP + 56;
