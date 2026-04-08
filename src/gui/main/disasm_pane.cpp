#include "disasm_pane.h"
#include <wx/dcbuffer.h>
#include <wx/clipbrd.h>
#include <wx/numdlg.h>
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
    m_drawPanel->Bind(wxEVT_RIGHT_DOWN, &DisasmPane::OnContextMenu, this);

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

void DisasmPane::SetDebugContext(DebugContext* dbg) {
    m_dbg = dbg;
}

void DisasmPane::SetMemoryPaneCallback(std::function<void(uint32_t)> cb) {
    m_memCallback = cb;
}

uint32_t DisasmPane::getAddrAtY(int y) const {
    if (!m_bus || !m_disasm) return 0;
    int line = y / m_lineHeight;
    uint32_t addr = m_viewAddr;
    for (int i = 0; i < line; ++i) {
        DisasmEntry entry;
        int bytes = m_disasm->disasmEntry(m_bus, addr, entry);
        if (bytes <= 0) bytes = 1;
        addr += bytes;
        if (addr >= 65536) break;
    }
    return addr;
}

void DisasmPane::OnContextMenu(wxMouseEvent& e) {
    if (!m_bus || !m_disasm) return;

    uint32_t addr = getAddrAtY(e.GetPosition().y);
    m_highlightAddr = addr;
    m_hasHighlight = true;
    m_drawPanel->Refresh();

    wxMenu menu;
    
    // Breakpoint section
    wxMenu* bpMenu = new wxMenu();
    bool hasBp = false;
    bool bpEnabled = false;
    int bpId = -1;
    if (m_dbg) {
        for (const auto& bp : m_dbg->breakpoints().breakpoints()) {
            if (bp.addr == addr) {
                hasBp = true;
                bpEnabled = bp.enabled;
                bpId = bp.id;
                break;
            }
        }
    }

    bpMenu->Append(1001, "Toggle Active (EXEC)");
    bpMenu->Append(1002, "Add EXEC Breakpoint");
    bpMenu->Append(1003, "Add READ Watchpoint");
    bpMenu->Append(1004, "Add WRITE Watchpoint");
    bpMenu->AppendSeparator();
    bpMenu->Append(1005, "Enable Breakpoint")->Enable(hasBp && !bpEnabled);
    bpMenu->Append(1006, "Disable Breakpoint")->Enable(hasBp && bpEnabled);
    menu.AppendSubMenu(bpMenu, "Breakpoint");

    // Navigate section
    wxMenu* navMenu = new wxMenu();
    navMenu->Append(2001, "Go to Address...\tCtrl-G");
    navMenu->Append(2002, "Go to PC");
    
    DisasmEntry entry;
    m_disasm->disasmEntry(m_bus, addr, entry);
    bool hasTarget = (entry.targetAddr != 0xFFFFFFFF && entry.targetAddr != 0);
    navMenu->Append(2003, wxString::Format("Follow Operand ($%04X)", entry.targetAddr))->Enable(hasTarget);
    navMenu->Append(2004, "Set PC here");
    
    bool canGoUp = (m_dbg && m_dbg->stackTrace().depth() > 0);
    navMenu->Append(2005, "Go up one stack frame")->Enable(canGoUp);
    menu.AppendSubMenu(navMenu, "Navigate");

    // Watch section
    menu.Append(3001, "Toggle Watch at address (R/W)");

    menu.AppendSeparator();
    menu.Append(4001, "Show address in memory pane");

    menu.AppendSeparator();
    wxMenu* clipMenu = new wxMenu();
    clipMenu->Append(5001, "Copy Address");
    clipMenu->Append(5002, "Copy Disassembly (next <n> lines)...");
    menu.AppendSubMenu(clipMenu, "Clipboard");

    int id = GetPopupMenuSelectionFromUser(menu);
    if (id == wxID_NONE) return;

    switch (id) {
        case 1001: // Toggle Active
            if (m_dbg) {
                if (hasBp) m_dbg->breakpoints().remove(bpId);
                else m_dbg->breakpoints().add(addr, BreakpointType::EXEC);
            }
            break;
        case 1002: // Add EXEC
            if (m_dbg) m_dbg->breakpoints().add(addr, BreakpointType::EXEC);
            break;
        case 1003: // Add READ
            if (m_dbg) m_dbg->breakpoints().add(addr, BreakpointType::READ_WATCH);
            break;
        case 1004: // Add WRITE
            if (m_dbg) m_dbg->breakpoints().add(addr, BreakpointType::WRITE_WATCH);
            break;
        case 1005: // Enable
            if (m_dbg && hasBp) m_dbg->breakpoints().setEnabled(bpId, true);
            break;
        case 1006: // Disable
            if (m_dbg && hasBp) m_dbg->breakpoints().setEnabled(bpId, false);
            break;

        case 2001: OnGoto(); break;
        case 2002: {
            m_trackPc = true;
            m_btnTrack->SetValue(true);
            int half = m_drawPanel->GetClientSize().y / m_lineHeight / 3;
            if (half < 4) half = 4;
            m_viewAddr = findStartAddr(m_pc, half);
            updateScrollBar();
            m_drawPanel->Refresh();
            break;
        }
        case 2003: GoTo(entry.targetAddr); break;
        case 2004: if (m_dbg) m_dbg->cpu()->setPc(addr); break;
        case 2005: if (m_dbg) {
            auto recent = m_dbg->stackTrace().recent();
            for (const auto& se : recent) {
                if (se.type == StackPushType::CALL) {
                    GoTo(se.pushedByPc);
                    break;
                }
            }
        } break;

        case 3001: // Toggle Watch (R/W)
            if (m_dbg) {
                bool hasWatch = false;
                int watchId = -1;
                for (const auto& bp : m_dbg->breakpoints().breakpoints()) {
                    if (bp.addr == addr && (bp.type == BreakpointType::READ_WATCH || bp.type == BreakpointType::WRITE_WATCH)) {
                        hasWatch = true; watchId = bp.id; break;
                    }
                }
                if (hasWatch) m_dbg->breakpoints().remove(watchId);
                else m_dbg->breakpoints().add(addr, BreakpointType::WRITE_WATCH); // Default to write
            }
            break;

        case 4001: if (m_memCallback) m_memCallback(addr); break;

        case 5001: // Copy Address
            if (wxTheClipboard->Open()) {
                wxTheClipboard->SetData(new wxTextDataObject(wxString::Format("%04X", addr)));
                wxTheClipboard->Close();
            }
            break;
        case 5002: { // Copy Disassembly
            long n = wxGetNumberFromUser("Number of lines to copy:", "Lines:", "Copy Disassembly", 10, 1, 1000, this);
            if (n > 0) {
                wxString text;
                uint32_t a = addr;
                for (int i = 0; i < (int)n; ++i) {
                    DisasmEntry de;
                    int b = m_disasm->disasmEntry(m_bus, a, de);
                    if (b <= 0) b = 1;
                    
                    text << wxString::Format("%04X: ", a);
                    for (int j = 0; j < 3; ++j) {
                        if (j < b) text << wxString::Format("%02X ", (int)m_bus->peek8(a + j));
                        else text << "   ";
                    }
                    text << "  " << de.mnemonic << " " << de.operands << "\n";
                    a += b;
                    if (a >= 65536) break;
                }
                if (wxTheClipboard->Open()) {
                    wxTheClipboard->SetData(new wxTextDataObject(text));
                    wxTheClipboard->Close();
                }
            }
            break;
        }
    }
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
