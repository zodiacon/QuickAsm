#pragma once

#ifdef _DEBUG
#define wxUSE_DEBUG_CONTEXT 1
#define wxUSE_MEMORY_TRACING 1
#define wxUSE_GLOBAL_MEMORY_OPERATORS 1
#define wxUSE_DEBUG_NEW_ALWAYS 1
#endif

#include <wx/setup.h>
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/artprov.h>
#include <wx/sysopt.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <memory>

#include <keystone/keystone.h>
#include <capstone/capstone.h>
#include <unicorn/unicorn.h>
