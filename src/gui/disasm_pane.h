#pragma once
#include <wx/wx.h>
#include "libmem/ibus.h"
#include "libtoolchain/idisasm.h"

class DisasmPane : public wxPanel {
public:
    DisasmPane(wxWindow* parent);
    void SetBus(IBus* bus);
    void SetDisassembler(IDisassembler* disasm);
    void RefreshValues(uint32_t pc);

private:
    void OnPaint(wxPaintEvent& event);

    IBus* m_bus = nullptr;
    IDisassembler* m_disasm = nullptr;
    uint32_t m_pc = 0;
    wxFont m_fixedFont;
    int m_lineHeight = 1;
};
