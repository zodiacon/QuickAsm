#ifndef NEWSCROLLINGDIALOG_H
#define NEWSCROLLINGDIALOG_H

//(*Headers(NewScrollingDialog)
#include "scrollingdialog.h"
//*)

class NewScrollingDialog: public wxScrollingDialog
{
	public:

		NewScrollingDialog(wxWindow* parent,wxWindowID id=wxID_ANY);
		virtual ~NewScrollingDialog();

		//(*Declarations(NewScrollingDialog)
		//*)

	protected:

		//(*Identifiers(NewScrollingDialog)
		//*)

	private:

		//(*Handlers(NewScrollingDialog)
		//*)

		DECLARE_EVENT_TABLE()
};

#endif
