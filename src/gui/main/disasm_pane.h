#pragma once
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include "libmem/main/ibus.h"
#include "libtoolchain/main/idisasm.h"

class DisasmPane : public wxPanel {
public:
    DisasmPane(wxWindow* parent);
    void SetBus(IBus* bus);
    void SetDisassembler(IDisassembler* disasm);
    void RefreshValues(uint32_t pc);
    void GoTo(uint32_t addr);

private:
    void OnPaint(wxPaintEvent& event);

    IBus*            m_bus    = nullptr;
    IDisassembler*   m_disasm = nullptr;
    uint32_t         m_pc       = 0;
    uint32_t         m_viewAddr = 0;
    bool             m_trackPc  = true;
    wxPanel*         m_drawPanel  = nullptr;
    wxButton*        m_btnGotoPc  = nullptr;
    wxToggleButton*  m_btnTrack   = nullptr;
    wxFont           m_fixedFont;
    int              m_lineHeight = 1;
};
