#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include "libdebug/main/debug_context.h"

class BreakpointPane : public wxPanel {
public:
    BreakpointPane(wxWindow* parent);
    void SetDebugContext(DebugContext* dbg);
    void RefreshValues();

private:
    void OnAdd(wxCommandEvent&);
    void OnDelete(wxCommandEvent&);
    void OnEnable(wxCommandEvent&);
    void OnDisable(wxCommandEvent&);
    void OnItemActivated(wxListEvent&);
    void OnItemSelected(wxListEvent&);
    void OnItemDeselected(wxListEvent&);
    void UpdateButtonStates();
    int  SelectedId() const;

    DebugContext* m_dbg = nullptr;
    wxListCtrl*   m_list;
    wxButton*     m_btnDelete;
    wxButton*     m_btnEnable;
    wxButton*     m_btnDisable;
};
