#pragma once
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <functional>
#include "libdebug/main/debug_context.h"

class StackPane : public wxPanel {
public:
    StackPane(wxWindow* parent);
    void SetDebugContext(DebugContext* dbg);
    void SetGotoCallback(std::function<void(uint32_t)> cb);
    void RefreshValues();

private:
    void OnClear(wxCommandEvent&);
    void OnItemActivated(wxListEvent& event);

    DebugContext* m_dbg = nullptr;
    wxListCtrl*   m_list;
    wxStaticText* m_depthLabel;
    std::function<void(uint32_t)> m_gotoCallback;
};
