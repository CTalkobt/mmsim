#include "disasm_pane.h"
#include <wx/dcbuffer.h>
#include <iomanip>
#include <sstream>

DisasmPane::DisasmPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE)
{
    m_fixedFont = wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    
    wxClientDC dc(this);
    dc.SetFont(m_fixedFont);
    m_lineHeight = dc.GetCharHeight();
    
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &DisasmPane::OnPaint, this);
}

void DisasmPane::SetBus(IBus* bus) {
    m_bus = bus;
    Refresh();
}

void DisasmPane::SetDisassembler(IDisassembler* disasm) {
    m_disasm = disasm;
    Refresh();
}

void DisasmPane::RefreshValues(uint32_t pc) {
    m_pc = pc;
    Refresh();
}

void DisasmPane::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    dc.SetFont(m_fixedFont);
    
    if (!m_bus || !m_disasm) return;
    
    int visibleLines = GetClientSize().y / m_lineHeight;
    int half = visibleLines / 2;
    
    // We want m_pc to be in the middle.
    // However, finding the instruction start 'half' lines before PC is tricky.
    // For now, let's just disassemble starting from some address before PC,
    // or just start at PC and show instructions following it.
    // Let's try to backtrack a bit. 6502 instructions are 1-3 bytes.
    // A simple backtrack: start 3*half bytes before PC and find instructions.
    
    uint32_t startAddr = (m_pc > (uint32_t)(3 * half)) ? m_pc - 3 * half : 0;
    
    // Better way: search forward from a known safe point, but we don't have many.
    // Let's just disassemble visibleLines instructions starting from startAddr.
    
    uint32_t currentAddr = startAddr;
    for (int i = 0; i < visibleLines; ++i) {
        if (currentAddr >= 65536) break;
        
        DisasmEntry entry;
        int bytes = m_disasm->disasmEntry(m_bus, currentAddr, entry);
        
        if (currentAddr == m_pc) {
            dc.SetBrush(*wxYELLOW_BRUSH);
            dc.DrawRectangle(0, i * m_lineHeight, GetClientSize().x, m_lineHeight);
            dc.SetTextForeground(*wxBLACK);
        } else {
            dc.SetTextForeground(*wxBLACK);
        }
        
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << currentAddr << ": ";
        
        // Bytes
        for (int j = 0; j < 3; ++j) {
            if (j < bytes) ss << std::setw(2) << (int)m_bus->peek8(currentAddr + j) << " ";
            else ss << "   ";
        }
        
        ss << "  " << entry.mnemonic << " " << entry.operands;
        
        dc.DrawText(ss.str(), 5, i * m_lineHeight);
        currentAddr += bytes;
    }
}
