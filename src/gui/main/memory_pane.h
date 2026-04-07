#pragma once
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include "libmem/main/ibus.h"
#include "include/util/logging.h"

class MemoryPane : public wxPanel {
public:
    MemoryPane(wxWindow* parent);
    void SetBus(IBus* bus);
    void RefreshValues();
    void SetAddress(uint32_t addr);
    void UpdatePc(uint32_t pc);

private:
    void OnScroll(wxScrollWinEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnContextMenu(wxContextMenuEvent& event);
    void OnMouseLeftDown(wxMouseEvent& event);
    void OnEditorKeyDown(wxKeyEvent& event);
    void OnEditorKillFocus(wxFocusEvent& event);
    void OpenEditorAt(uint32_t addr);

    void UpdateMetrics();

    IBus* m_bus = nullptr;
    uint32_t m_baseAddr = 0;
    wxFont m_fixedFont;
    int m_lineHeight = 1;
    int m_charWidth = 1;

    wxToggleButton* m_trackPcBtn = nullptr;
    int m_toolbarHeight = 0;
    wxTextCtrl* m_editor = nullptr;
    uint32_t m_editAddr = 0;
    bool m_trackPc = false;
};
