#pragma once
#include <wx/wx.h>
#include "libmem/main/ibus.h"

class MemoryPane : public wxPanel {
public:
    MemoryPane(wxWindow* parent);
    void SetBus(IBus* bus);
    void RefreshValues();
    void SetAddress(uint32_t addr);

private:
    void OnScroll(wxScrollWinEvent& event);
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);

    IBus* m_bus = nullptr;
    uint32_t m_baseAddr = 0;
    wxFont m_fixedFont;
    int m_lineHeight = 1;
    int m_charWidth = 1;
};
