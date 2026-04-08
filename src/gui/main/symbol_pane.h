#pragma once

#include <wx/wx.h>
#include <wx/listctrl.h>
#include "libdebug/main/debug_context.h"
#include <functional>

class SymbolPane : public wxPanel {
public:
    SymbolPane(wxWindow* parent);

    void SetDebugContext(DebugContext* dbg);
    void RefreshValues();

    /** Callback for when user wants to navigate to a symbol address. */
    void SetGotoCallback(std::function<void(uint32_t)> cb);

private:
    void OnAdd(wxCommandEvent& event);
    void OnDelete(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnLoad(wxCommandEvent& event);
    void OnSearch(wxCommandEvent& event);
    void OnItemActivated(wxListEvent& event);

    DebugContext* m_dbg = nullptr;
    wxListCtrl*   m_list;
    wxTextCtrl*   m_searchCtrl;
    std::function<void(uint32_t)> m_gotoCallback;
};
