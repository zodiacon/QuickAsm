#pragma once

class AssemblyEditCtrl : public wxStyledTextCtrl {
public:
    AssemblyEditCtrl();
    void Init();

    // common
    void OnCreate(wxWindowCreateEvent& event);
    void OnSize(wxSizeEvent& event);
    // edit
    void OnEditRedo(wxCommandEvent& event);
    void OnEditUndo(wxCommandEvent& event);
    void OnEditClear(wxCommandEvent& event);
    void OnEditCut(wxCommandEvent& event);
    void OnEditCopy(wxCommandEvent& event);
    void OnEditPaste(wxCommandEvent& event);
    // find
    void OnFind(wxCommandEvent& event);
    void OnFindNext(wxCommandEvent& event);
    void OnReplace(wxCommandEvent& event);
    void OnReplaceNext(wxCommandEvent& event);
    void OnBraceMatch(wxCommandEvent& event);
    void OnGoto(wxCommandEvent& event);
    void OnEditIndentInc(wxCommandEvent& event);
    void OnEditIndentRed(wxCommandEvent& event);
    void OnEditSelectAll(wxCommandEvent& event);
    void OnEditSelectLine(wxCommandEvent& event);
    //! view
    void OnHighlightLang(wxCommandEvent& event);
    void OnDisplayEOL(wxCommandEvent& event);
    void OnIndentGuide(wxCommandEvent& event);
    void OnLineNumber(wxCommandEvent& event);
    void OnLongLineOn(wxCommandEvent& event);
    void OnWhiteSpace(wxCommandEvent& event);
    void OnFoldToggle(wxCommandEvent& event);
    void OnSetOverType(wxCommandEvent& event);
    void OnSetReadOnly(wxCommandEvent& event);
    void OnWrapmodeOn(wxCommandEvent& event);
    void OnUseCharset(wxCommandEvent& event);
    // annotations
    void OnAnnotationAdd(wxCommandEvent& event);
    void OnAnnotationRemove(wxCommandEvent& event);
    void OnAnnotationClear(wxCommandEvent& event);
    void OnAnnotationStyle(wxCommandEvent& event);
    // indicators
    void OnIndicatorFill(wxCommandEvent& event);
    void OnIndicatorClear(wxCommandEvent& event);
    void OnIndicatorStyle(wxCommandEvent& event);
    // stc
    void OnMarginClick(wxStyledTextEvent& event);
    void OnCharAdded(wxStyledTextEvent& event);
    void OnCallTipClick(wxStyledTextEvent& event);

private:
};

