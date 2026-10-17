#pragma once

#define WINVER		0x0A00
#define _WIN32_WINNT	0x0A00
#define _WIN32_IE	0x0A00
#define NOMINMAX

#include <atlbase.h>
#include <atlstr.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlx.h>
#include <atltypes.h>
#include <atlsplit.h>
#include <atltheme.h>
#include <atlcrack.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <memory>
#include <map>
#include <functional>
#include <format>

#include <keystone/keystone.h>
#include <capstone/capstone.h>
#include <unicorn/unicorn.h>
#include <wil/resource.h>

#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
