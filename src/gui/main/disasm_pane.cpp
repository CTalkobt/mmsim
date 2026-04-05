#include "disasm_pane.h"
#include <wx/dcbuffer.h>
#include <iomanip>
#include <sstream>
#include <climits>
#include <algorithm>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

bool DisasmPane::isIllegal(const DisasmEntry& entry, int bytes) {
    if (bytes <= 0) return true;
    return entry.isIllegal;
}

// Return the address of the instruction immediately before `addr`.
// Tries 1-3 bytes back; prefers the candidate whose instruction size lands
// exactly on `addr` and that has a non-illegal mnemonic.
uint32_t DisasmPane::findPrevAddr(uint32_t addr) const {
    if (!m_bus || !m_disasm || addr == 0) return 0;
    int maxBack = std::min((int)addr, 3);
    uint32_t best = addr - 1;  // safe fallback
    int bestScore = INT_MIN;
    for (int back = 1; back <= maxBack; back++) {
        uint32_t candidate = addr - back;
        DisasmEntry entry;
        int bytes = m_disasm->disasmEntry(m_bus, candidate, entry);
        if (bytes == back) {
            int score = isIllegal(entry, bytes) ? -100 : 0;
            if (score > bestScore) { bestScore = score; best = candidate; }
        }
    }
    return best;
}

// Return a start address that is `linesAbove` valid instructions before
// `viewAddr`, preferring sequences with no illegal/KIL opcodes.
// Search window: up to linesAbove*3+3 bytes back (6502 max instruction = 3B).
uint32_t DisasmPane::findStartAddr(uint32_t viewAddr, int linesAbove) const {
    if (!m_bus || !m_disasm || viewAddr == 0 || linesAbove <= 0) return viewAddr;
    int maxBack = std::min((int)viewAddr, linesAbove * 3 + 3);

    uint32_t bestStart = viewAddr;
    int bestIllegals   = INT_MAX;
    int bestLines      = 0;

    for (int back = 1; back <= maxBack; back++) {
        uint32_t candidate = viewAddr - back;
        uint32_t addr = candidate;
        int illegals = 0, lines = 0;
        bool hit = false;
        while (addr <= viewAddr && lines <= linesAbove) {
            if (addr == viewAddr) { hit = true; break; }
            DisasmEntry entry;
            int bytes = m_disasm->disasmEntry(m_bus, addr, entry);
            if (bytes <= 0) { illegals += 10; bytes = 1; }
            else if (isIllegal(entry, bytes)) illegals++;
            addr += bytes;
            ++lines;
        }
        if (hit && lines <= linesAbove) {
            if (illegals < bestIllegals ||
                (illegals == bestIllegals && lines > bestLines)) {
                bestIllegals = illegals;
                bestLines    = lines;
                bestStart    = candidate;
            }
        }
    }
    return bestStart;
}

// Scroll the view by `delta` disassembled instructions.
// Positive = forward (higher addresses); negative = backward.
void DisasmPane::scrollByLines(int delta) {
    if (!m_bus || !m_disasm) return;
    if (delta > 0) {
        uint32_t addr = m_viewAddr;
        for (int i = 0; i < delta; ++i) {
            if (addr >= 65535) break;
            DisasmEntry entry;
            int bytes = m_disasm->disasmEntry(m_bus, addr, entry);
            if (bytes <= 0) bytes = 1;
            addr += bytes;
        }
        m_viewAddr = addr;
    } else if (delta < 0) {
        uint32_t addr = m_viewAddr;
        for (int i = 0; i < -delta; ++i) {
            if (addr == 0) break;
            addr = findPrevAddr(addr);
        }
        m_viewAddr = addr;
    }
    m_trackPc = false;
    m_btnTrack->SetValue(false);
}

void DisasmPane::updateScrollBar() {
    m_scrollBar->SetThumbPosition((int)m_viewAddr);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

DisasmPane::DisasmPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    m_fixedFont = wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    auto* outerSizer = new wxBoxSizer(wxVERTICAL);

    // Toolbar
    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    m_btnGotoPc = new wxButton(this, wxID_ANY, "\u2b05 PC",
                               wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_btnTrack  = new wxToggleButton(this, wxID_ANY, "Track",
                                     wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
    m_btnTrack->SetValue(true);
    toolbar->Add(m_btnGotoPc, 0, wxALL, 2);
    toolbar->Add(m_btnTrack,  0, wxALL, 2);
    outerSizer->Add(toolbar, 0, wxEXPAND);

    // Draw panel + vertical scrollbar
    m_drawPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxFULL_REPAINT_ON_RESIZE | wxWANTS_CHARS);
    m_drawPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);

    m_scrollBar = new wxScrollBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSB_VERTICAL);
    // Range 0–65535, thumb size ~16 bytes ≈ 8 instructions
    m_scrollBar->SetScrollbar(0, 16, 65536, 16);

    auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);
    contentSizer->Add(m_drawPanel, 1, wxEXPAND);
    contentSizer->Add(m_scrollBar, 0, wxEXPAND);
    outerSizer->Add(contentSizer, 1, wxEXPAND);

    SetSizer(outerSizer);

    // Measure line height
    wxClientDC dc(m_drawPanel);
    dc.SetFont(m_fixedFont);
    m_lineHeight = dc.GetCharHeight();
    if (m_lineHeight < 1) m_lineHeight = 14;

    // ---------- event bindings ----------

    m_drawPanel->Bind(wxEVT_PAINT,      &DisasmPane::OnPaint,      this);
    m_drawPanel->Bind(wxEVT_MOUSEWHEEL, &DisasmPane::OnMouseWheel, this);

    // Click on draw panel gives it keyboard focus
    m_drawPanel->Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
        m_drawPanel->SetFocus(); e.Skip();
    });

    // Arrow/Page keys when draw panel has focus
    m_drawPanel->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
        int pg = std::max(1, m_drawPanel->GetClientSize().y / m_lineHeight - 1);
        switch (e.GetKeyCode()) {
            case WXK_UP:    scrollByLines(-1);  updateScrollBar(); m_drawPanel->Refresh(); break;
            case WXK_DOWN:  scrollByLines(1);   updateScrollBar(); m_drawPanel->Refresh(); break;
            case WXK_PAGEUP:   scrollByLines(-pg); updateScrollBar(); m_drawPanel->Refresh(); break;
            case WXK_PAGEDOWN: scrollByLines(pg);  updateScrollBar(); m_drawPanel->Refresh(); break;
            default: e.Skip(); break;
        }
    });

    // Ctrl-G: go to address (sets highlight)
    Bind(wxEVT_CHAR_HOOK, [this](wxKeyEvent& e) {
        if (e.GetKeyCode() == 'G' && e.GetModifiers() == wxMOD_CONTROL) {
            OnGoto();
        } else {
            e.Skip();
        }
    });

    // Scrollbar — dragging thumb or using page click snaps directly to address
    m_scrollBar->Bind(wxEVT_SCROLL_THUMBTRACK, [this](wxScrollEvent& e) {
        m_viewAddr = (uint32_t)e.GetPosition();
        m_trackPc = false;
        m_btnTrack->SetValue(false);
        m_drawPanel->Refresh();
    });
    m_scrollBar->Bind(wxEVT_SCROLL_CHANGED, [this](wxScrollEvent&) {
        uint32_t pos = (uint32_t)m_scrollBar->GetThumbPosition();
        if (pos != m_viewAddr) {
            m_viewAddr = pos;
            m_trackPc = false;
            m_btnTrack->SetValue(false);
            m_drawPanel->Refresh();
        }
    });

    m_btnGotoPc->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_trackPc = true;
        m_btnTrack->SetValue(true);
        int half = m_drawPanel->GetClientSize().y / m_lineHeight / 3;
        if (half < 4) half = 4;
        m_viewAddr = findStartAddr(m_pc, half);
        updateScrollBar();
        m_drawPanel->Refresh();
    });

    m_btnTrack->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent& e) {
        m_trackPc = e.IsChecked();
        if (m_trackPc) {
            int half = m_drawPanel->GetClientSize().y / m_lineHeight / 3;
            if (half < 4) half = 4;
            m_viewAddr = findStartAddr(m_pc, half);
            updateScrollBar();
            m_drawPanel->Refresh();
        }
    });
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void DisasmPane::SetBus(IBus* bus) {
    m_bus = bus;
    m_drawPanel->Refresh();
}

void DisasmPane::SetDisassembler(IDisassembler* disasm) {
    m_disasm = disasm;
    m_drawPanel->Refresh();
}

void DisasmPane::RefreshValues(uint32_t pc) {
    m_pc = pc;
    if (m_trackPc) {
        int half = m_drawPanel->GetClientSize().y / m_lineHeight / 3;
        if (half < 4) half = 4;
        m_viewAddr = findStartAddr(pc, half);
        updateScrollBar();
    }
    m_drawPanel->Refresh();
}

void DisasmPane::GoTo(uint32_t addr) {
    m_highlightAddr = addr;
    m_hasHighlight  = true;
    m_trackPc       = false;
    m_btnTrack->SetValue(false);
    // Centre the highlight: show some context above it
    int half = m_drawPanel->GetClientSize().y / m_lineHeight / 3;
    if (half < 4) half = 4;
    m_viewAddr = findStartAddr(addr, half);
    updateScrollBar();
    m_drawPanel->Refresh();
}

void DisasmPane::OnGoto() {
    wxString s = wxGetTextFromUser(
        "Enter address (hex):", "Go to Address",
        wxString::Format("%04X", m_highlightAddr), this);
    if (s.IsEmpty()) return;
    unsigned long addr = 0;
    if (s.ToULong(&addr, 16) && addr <= 0xFFFFFFFFul)
        GoTo((uint32_t)addr);
}

// ---------------------------------------------------------------------------
// Events
// ---------------------------------------------------------------------------

void DisasmPane::OnMouseWheel(wxMouseEvent& e) {
    int lines = e.GetWheelRotation() / e.GetWheelDelta();
    scrollByLines(-lines);   // wheel up → backward (lower address)
    updateScrollBar();
    m_drawPanel->Refresh();
}

void DisasmPane::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_drawPanel);
    dc.Clear();
    dc.SetFont(m_fixedFont);

    if (!m_bus || !m_disasm) return;

    int visibleLines = m_drawPanel->GetClientSize().y / m_lineHeight;
    const int w = m_drawPanel->GetClientSize().x;
    const wxColour hlColour(173, 216, 230);  // light blue for highlight cursor

    uint32_t currentAddr = m_viewAddr;
    for (int i = 0; i < visibleLines; ++i) {
        if (currentAddr >= 65536) break;

        DisasmEntry entry;
        int bytes = m_disasm->disasmEntry(m_bus, currentAddr, entry);
        if (bytes <= 0) bytes = 1;

        bool isPC = (currentAddr == m_pc);
        bool isHL = (m_hasHighlight && currentAddr == m_highlightAddr);

        if (isPC) {
            dc.SetBrush(*wxYELLOW_BRUSH);
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(0, i * m_lineHeight, w, m_lineHeight);
        } else if (isHL) {
            dc.SetBrush(wxBrush(hlColour));
            dc.SetPen(*wxTRANSPARENT_PEN);
            dc.DrawRectangle(0, i * m_lineHeight, w, m_lineHeight);
        }

        dc.SetTextForeground(*wxBLACK);

        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << currentAddr << ": ";
        for (int j = 0; j < 3; ++j) {
            if (j < bytes) ss << std::setw(2) << (int)m_bus->peek8(currentAddr + j) << " ";
            else           ss << "   ";
        }
        ss << "  " << entry.mnemonic << " " << entry.operands;

        dc.DrawText(ss.str(), 5, i * m_lineHeight);
        currentAddr += bytes;
    }
}
