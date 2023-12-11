#pragma once

#include <wx/setup.h>
#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/stc/stc.h>
#include <wx/artprov.h>
#include <wx/sysopt.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/msgdlg.h> 
#include <wx/config.h> 

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <cassert>
#include <memory>

#include <keystone/keystone.h>
#include <capstone/capstone.h>
#include <unicorn/unicorn.h>
#include <wil/resource.h>
