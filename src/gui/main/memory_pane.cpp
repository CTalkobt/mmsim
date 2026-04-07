#include "memory_pane.h"
#include "gui_ids.h"
#include <wx/dcbuffer.h>
#include <iomanip>
#include <sstream>

MemoryPane::MemoryPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxFULL_REPAINT_ON_RESIZE)
{
    m_fixedFont = wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    
    UpdateMetrics();
    
    m_trackPcBtn = new wxToggleButton(this, wxID_ANY, "Track PC");
    m_toolbarHeight = m_trackPcBtn->GetBestSize().GetHeight() + 2;
    m_trackPcBtn->SetSize(0, 0, GetClientSize().GetWidth(), m_toolbarHeight);
    m_trackPcBtn->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent& e) {
        m_trackPc = e.IsChecked();
    });

    SetBackgroundStyle(wxBG_STYLE_PAINT);
    Bind(wxEVT_PAINT, &MemoryPane::OnPaint, this);
    Bind(wxEVT_SIZE, &MemoryPane::OnSize, this);
    Bind(wxEVT_CONTEXT_MENU, &MemoryPane::OnContextMenu, this);
    Bind(wxEVT_LEFT_DOWN, &MemoryPane::OnMouseLeftDown, this);
    
    auto scrollHandler = [this](wxScrollWinEvent& event) {
        if (m_editor) {
            m_editor->Destroy();
            m_editor = nullptr;
        }
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
        SetScrollbar(wxVERTICAL, 0, (GetClientSize().y - m_toolbarHeight) / m_lineHeight, lines);
    }
    RefreshValues();
}

void MemoryPane::RefreshValues() {
    Refresh();
}

void MemoryPane::UpdateMetrics() {
    wxClientDC dc(this);
    dc.SetFont(m_fixedFont);
    m_lineHeight = dc.GetCharHeight();
    m_charWidth = dc.GetCharWidth();
    if (m_lineHeight < 1) m_lineHeight = 1;
    if (m_charWidth < 1) m_charWidth = 1;
}

void MemoryPane::SetAddress(uint32_t addr) {
    if (m_editor) {
        m_editor->Destroy();
        m_editor = nullptr;
    }
    int line = addr / 16;
    SetScrollPos(wxVERTICAL, line);
    Refresh();
}

void MemoryPane::OnScroll(wxScrollWinEvent& event) {
    if (m_editor) {
        m_editor->Destroy();
        m_editor = nullptr;
    }
    Refresh();
    event.Skip();
}

void MemoryPane::OnSize(wxSizeEvent& event) {
    if (m_trackPcBtn)
        m_trackPcBtn->SetSize(0, 0, GetClientSize().GetWidth(), m_toolbarHeight);
    if (m_bus) {
        int lines = (m_bus->config().addrMask + 1) / 16;
        SetScrollbar(wxVERTICAL, GetScrollPos(wxVERTICAL), (GetClientSize().y - m_toolbarHeight) / m_lineHeight, lines);
    }
    Refresh();
    event.Skip();
}

void MemoryPane::OnContextMenu(wxContextMenuEvent& event) {
    wxMenu menu;
    menu.Append(ID_GOTO_ADDR, "Go to Address...");
    menu.AppendSeparator();
    menu.Append(ID_FILL_MEM, "Fill Memory...");
    menu.Append(ID_COPY_MEM, "Copy Memory...");
    menu.Append(ID_SWAP_MEM, "Swap Memory...");
    menu.AppendSeparator();
    menu.Append(ID_SEARCH_MEM, "Search Memory...");

    PopupMenu(&menu);
}

void MemoryPane::UpdatePc(uint32_t pc) {
    if (!m_trackPc || !m_bus) return;
    int pcLine = (int)(pc / 16);
    int scrollPos = GetScrollPos(wxVERTICAL);
    int visibleLines = (GetClientSize().y - m_toolbarHeight) / m_lineHeight;
    if (pcLine < scrollPos || pcLine >= scrollPos + visibleLines) {
        SetScrollPos(wxVERTICAL, pcLine);
        Refresh();
    }
}

void MemoryPane::OnMouseLeftDown(wxMouseEvent& event) {
    auto logger = LogRegistry::instance().getLogger("gui");
    if (m_editor) {
        logger->debug("MemoryPane: Destroying existing editor in OnMouseLeftDown");
        m_editor->Destroy();
        m_editor = nullptr;
    }

    if (!m_bus) {
        logger->debug("MemoryPane::OnMouseLeftDown: No bus attached.");
        return;
    }

    UpdateMetrics();

    uint32_t mask = m_bus->config().addrMask;
    int addrWidth = (mask > 0xFFFF ? 8 : 4);
    int prefixChars = addrWidth + 2; // "HHHH: " or "HHHHHHHH: "

    int x = event.GetX() - 5;
    int y = event.GetY() - m_toolbarHeight;
    if (y < 0) { event.Skip(); return; }

    int line = y / m_lineHeight;
    int col = x / m_charWidth;

    int scrollPos = GetScrollPos(wxVERTICAL);
    uint32_t lineAddr = (scrollPos + line) * 16;
    
    logger->debug("MemoryPane click: x={}, y={}, col={}, line={}, prefix={}, scrollPos={}, lineAddr={:04X}", 
        x, y, col, line, prefixChars, scrollPos, lineAddr);

    if (lineAddr > mask) {
        logger->debug("MemoryPane: click beyond mask ({:04X} > {:04X})", lineAddr, mask);
        return;
    }

    int hexCol = col - prefixChars;
    logger->debug("MemoryPane: hexCol={}", hexCol);

    if (hexCol >= 0 && hexCol < 47) { // 16 * 3 - 1
        int cellIdx = hexCol / 3;
        int cellOff = hexCol % 3;
        logger->debug("MemoryPane: cellIdx={}, cellOff={}", cellIdx, cellOff);
        if (cellOff < 2) {
            uint32_t editAddr = (lineAddr + cellIdx) & mask;
            CallAfter([this, editAddr]() { OpenEditorAt(editAddr); });
        }
    }
}

void MemoryPane::OpenEditorAt(uint32_t addr) {
    auto logger = LogRegistry::instance().getLogger("gui");
    if (!m_bus) return;

    uint32_t mask = m_bus->config().addrMask;
    addr &= mask;

    UpdateMetrics();
    int addrWidth = (mask > 0xFFFF ? 8 : 4);
    int prefixChars = addrWidth + 2;

    int scrollPos = GetScrollPos(wxVERTICAL);
    int visibleLines = (GetClientSize().y - m_toolbarHeight) / m_lineHeight;
    int absLine = (int)(addr / 16);

    // Scroll into view if the target line is off-screen.
    if (absLine < scrollPos || absLine >= scrollPos + visibleLines) {
        scrollPos = absLine;
        SetScrollPos(wxVERTICAL, scrollPos);
        Refresh();
    }

    int line = absLine - scrollPos;
    int cellIdx = (int)(addr % 16);
    int cellX = 5 + (prefixChars + cellIdx * 3) * m_charWidth;
    int cellY = m_toolbarHeight + line * m_lineHeight;

    if (m_editor) {
        m_editor->Destroy();
        m_editor = nullptr;
    }

    m_editAddr = addr;
    logger->debug("MemoryPane: Creating editor at {:04X} (x={}, y={})", m_editAddr, cellX, cellY);

    m_editor = new wxTextCtrl(this, wxID_ANY,
        wxString::Format("%02X", m_bus->peek8(m_editAddr)),
        wxPoint(cellX - 2, cellY),
        wxSize(m_charWidth * 3 + 4, m_lineHeight + 4),
        wxTE_PROCESS_ENTER);

    m_editor->SetMaxLength(2);
    m_editor->SetFont(m_fixedFont);
    m_editor->SetBackgroundColour(*wxRED);
    m_editor->SetForegroundColour(*wxWHITE);
    m_editor->SetSelection(-1, -1);
    m_editor->SetFocus();
    m_editor->Bind(wxEVT_KEY_DOWN, &MemoryPane::OnEditorKeyDown, this);
}

void MemoryPane::OnEditorKeyDown(wxKeyEvent& event) {
    auto logger = LogRegistry::instance().getLogger("gui");
    if (event.GetKeyCode() == WXK_RETURN) {
        wxString valStr = m_editor->GetValue();
        uint32_t nextAddr = (m_editAddr + 1) & m_bus->config().addrMask;
        try {
            uint8_t val = (uint8_t)std::stoul(valStr.ToStdString(), nullptr, 16);
            m_bus->write8(m_editAddr, val);
            logger->debug("MemoryPane: Committed {:02X} to {:04X}", val, m_editAddr);
            Refresh();
        } catch (...) {
            logger->debug("MemoryPane: Invalid hex input '{}'", valStr.ToStdString());
        }
        m_editor->Destroy();
        m_editor = nullptr;
        CallAfter([this, nextAddr]() { OpenEditorAt(nextAddr); });
    } else if (event.GetKeyCode() == WXK_ESCAPE) {
        logger->debug("MemoryPane: Editor cancelled via Escape");
        m_editor->Destroy();
        m_editor = nullptr;
    } else {
        event.Skip();
    }
}

void MemoryPane::OnEditorKillFocus(wxFocusEvent& event) {
    auto logger = LogRegistry::instance().getLogger("gui");
    if (m_editor) {
        logger->debug("MemoryPane: Editor destroyed due to KillFocus");
        m_editor->Destroy();
        m_editor = nullptr;
    }
}

void MemoryPane::OnPaint(wxPaintEvent& event) {
    auto logger = LogRegistry::instance().getLogger("gui");
    wxAutoBufferedPaintDC dc(this);
    dc.Clear();
    
    if (m_editor) {
        logger->debug("MemoryPane::OnPaint: Editor exists at pos ({}, {})", m_editor->GetPosition().x, m_editor->GetPosition().y);
    }

    if (!m_bus) return;
    
    UpdateMetrics();
    dc.SetFont(m_fixedFont);
    
    uint32_t mask = m_bus->config().addrMask;
    int scrollPos = GetScrollPos(wxVERTICAL);
    int visibleLines = (GetClientSize().y - m_toolbarHeight) / m_lineHeight + 1;

    for (int i = 0; i < visibleLines; ++i) {
        uint32_t lineAddr = (scrollPos + i) * 16;
        if (lineAddr > mask) break;
        lineAddr &= mask;
        
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
        
        dc.DrawText(ss.str(), 5, m_toolbarHeight + i * m_lineHeight);
    }
}
