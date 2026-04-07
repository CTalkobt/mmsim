#include "memory_pane.h"
#include <wx/dcbuffer.h>
#include <iomanip>
#include <sstream>

MemoryPane::MemoryPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxFULL_REPAINT_ON_RESIZE)
{
    m_fixedFont = wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    
    wxClientDC dc(this);
    dc.SetFont(m_fixedFont);
    m_lineHeight = dc.GetCharHeight();
    m_charWidth = dc.GetCharWidth();
    
    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &MemoryPane::OnPaint, this);
    Bind(wxEVT_SIZE, &MemoryPane::OnSize, this);
    
    auto scrollHandler = [this](wxScrollWinEvent& event) {
        Refresh();
        event.Skip();
    };
    
    Bind(wxEVT_SCROLLWIN_TOP, scrollHandler);
    Bind(wxEVT_SCROLLWIN_BOTTOM, scrollHandler);
    Bind(wxEVT_SCROLLWIN_LINEUP, scrollHandler);
    Bind(wxEVT_SCROLLWIN_LINEDOWN, scrollHandler);
    Bind(wxEVT_SCROLLWIN_PAGEUP, scrollHandler);
    Bind(wxEVT_SCROLLWIN_PAGEDOWN, scrollHandler);
    Bind(wxEVT_SCROLLWIN_THUMBTRACK, scrollHandler);
    Bind(wxEVT_SCROLLWIN_THUMBRELEASE, scrollHandler);
}

void MemoryPane::SetBus(IBus* bus) {
    m_bus = bus;
    if (m_bus) {
        int lines = (m_bus->config().addrMask + 1) / 16;
        SetScrollbar(wxVERTICAL, 0, GetClientSize().y / m_lineHeight, lines);
    }
    RefreshValues();
}

void MemoryPane::RefreshValues() {
    Refresh();
}

void MemoryPane::SetAddress(uint32_t addr) {
    int line = addr / 16;
    SetScrollPos(wxVERTICAL, line);
    Refresh();
}

void MemoryPane::OnScroll(wxScrollWinEvent& event) {
    Refresh();
    event.Skip();
}

void MemoryPane::OnSize(wxSizeEvent& event) {
    if (m_bus) {
        int lines = (m_bus->config().addrMask + 1) / 16;
        SetScrollbar(wxVERTICAL, GetScrollPos(wxVERTICAL), GetClientSize().y / m_lineHeight, lines);
    }
    Refresh();
    event.Skip();
}

void MemoryPane::OnPaint(wxPaintEvent& event) {
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    dc.SetFont(m_fixedFont);
    
    if (!m_bus) return;
    
    uint32_t mask = m_bus->config().addrMask;
    int scrollPos = GetScrollPos(wxVERTICAL);
    int visibleLines = GetClientSize().y / m_lineHeight + 1;
    
    for (int i = 0; i < visibleLines; ++i) {
        uint32_t lineAddr = (scrollPos + i) * 16;
        if (lineAddr > mask) break;
        
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(mask > 0xFFFF ? 8 : 4) << lineAddr << ": ";
        
        // Hex part
        for (int j = 0; j < 16; ++j) {
            uint32_t addr = (lineAddr + j) & mask;
            ss << std::setw(2) << (int)m_bus->peek8(addr) << " ";
        }
        
        // ASCII part
        ss << "  ";
        for (int j = 0; j < 16; ++j) {
            uint32_t addr = (lineAddr + j) & mask;
            uint8_t c = m_bus->peek8(addr);
            if (c >= 32 && c <= 126) ss << (char)c;
            else ss << ".";
        }
        
        dc.DrawText(ss.str(), 5, i * m_lineHeight);
    }
}
