#pragma once
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include <wx/scrolbar.h>
#include "libmem/main/ibus.h"
#include "libtoolchain/main/idisasm.h"

class DisasmPane : public wxPanel {
public:
    DisasmPane(wxWindow* parent);
    void SetBus(IBus* bus);
    void SetDisassembler(IDisassembler* disasm);
    void RefreshValues(uint32_t pc);
    void GoTo(uint32_t addr);   // sets highlight cursor and scrolls into view

private:
    void OnPaint(wxPaintEvent&);
    void OnMouseWheel(wxMouseEvent&);
    void OnGoto();
    void scrollByLines(int delta);
    void updateScrollBar();
    uint32_t findPrevAddr(uint32_t addr) const;
    uint32_t findStartAddr(uint32_t addr, int linesAbove) const;
    static bool isIllegal(const DisasmEntry& entry, int bytes);

    IBus*            m_bus           = nullptr;
    IDisassembler*   m_disasm        = nullptr;
    uint32_t         m_pc            = 0;
    uint32_t         m_viewAddr      = 0;    // address of first instruction shown
    uint32_t         m_highlightAddr = 0;    // cursor / GoTo destination
    bool             m_hasHighlight  = false;
    bool             m_trackPc       = true;
    wxPanel*         m_drawPanel     = nullptr;
    wxButton*        m_btnGotoPc     = nullptr;
    wxToggleButton*  m_btnTrack      = nullptr;
    wxScrollBar*     m_scrollBar     = nullptr;
    wxFont           m_fixedFont;
    int              m_lineHeight    = 1;
};
