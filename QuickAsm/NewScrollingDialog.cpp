#include "NewScrollingDialog.h"

//(*InternalHeaders(NewScrollingDialog)
#include <wx/intl.h>
#include <wx/string.h>
//*)

//(*IdInit(NewScrollingDialog)
//*)

BEGIN_EVENT_TABLE(NewScrollingDialog,wxScrollingDialog)
	//(*EventTable(NewScrollingDialog)
	//*)
END_EVENT_TABLE()

NewScrollingDialog::NewScrollingDialog(wxWindow* parent,wxWindowID id)
{
	//(*Initialize(NewScrollingDialog)
	Create(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE, _T("id"));
	//*)
}

NewScrollingDialog::~NewScrollingDialog()
{
	//(*Destroy(NewScrollingDialog)
	//*)
}

