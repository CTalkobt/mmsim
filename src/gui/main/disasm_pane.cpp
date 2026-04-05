#include "disasm_pane.h"
#include <wx/dcbuffer.h>
#include <iomanip>
#include <sstream>

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

    // Draw panel
    m_drawPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                              wxFULL_REPAINT_ON_RESIZE);
    m_drawPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
    outerSizer->Add(m_drawPanel, 1, wxEXPAND);

    SetSizer(outerSizer);

    // Measure line height against the draw panel
    wxClientDC dc(m_drawPanel);
    dc.SetFont(m_fixedFont);
    m_lineHeight = dc.GetCharHeight();
    if (m_lineHeight < 1) m_lineHeight = 14;

    m_drawPanel->Bind(wxEVT_PAINT, &DisasmPane::OnPaint, this);

    m_btnGotoPc->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        m_viewAddr = m_pc;
        m_trackPc  = true;
        m_btnTrack->SetValue(true);
        m_drawPanel->Refresh();
    });

    m_btnTrack->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent& e) {
        m_trackPc = e.IsChecked();
        if (m_trackPc) {
            m_viewAddr = m_pc;
            m_drawPanel->Refresh();
        }
    });
}

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
    if (m_trackPc) m_viewAddr = pc;
    m_drawPanel->Refresh();
}

void DisasmPane::GoTo(uint32_t addr) {
    m_viewAddr = addr;
    m_trackPc  = false;
    m_btnTrack->SetValue(false);
    m_drawPanel->Refresh();
}

void DisasmPane::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(m_drawPanel);
    dc.Clear();
    dc.SetFont(m_fixedFont);

    if (!m_bus || !m_disasm) return;

    int visibleLines = m_drawPanel->GetClientSize().y / m_lineHeight;
    int half = visibleLines / 2;

    // Back-track from m_viewAddr to give some context above it.
    uint32_t startAddr = (m_viewAddr > (uint32_t)(3 * half)) ? m_viewAddr - 3 * half : 0;

    uint32_t currentAddr = startAddr;
    for (int i = 0; i < visibleLines; ++i) {
        if (currentAddr >= 65536) break;

        DisasmEntry entry;
        int bytes = m_disasm->disasmEntry(m_bus, currentAddr, entry);

        if (currentAddr == m_pc) {
            dc.SetBrush(*wxYELLOW_BRUSH);
            dc.DrawRectangle(0, i * m_lineHeight, m_drawPanel->GetClientSize().x, m_lineHeight);
            dc.SetTextForeground(*wxBLACK);
        } else {
            dc.SetTextForeground(*wxBLACK);
        }

        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << currentAddr << ": ";
        for (int j = 0; j < 3; ++j) {
            if (j < bytes) ss << std::setw(2) << (int)m_bus->peek8(currentAddr + j) << " ";
            else ss << "   ";
        }
        ss << "  " << entry.mnemonic << " " << entry.operands;

        dc.DrawText(ss.str(), 5, i * m_lineHeight);
        currentAddr += bytes;
    }
}
