#include "register_pane.h"
#include <wx/settings.h>
#include <iomanip>
#include <sstream>

RegisterPane::RegisterPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    m_fixedFont = wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_grid = new wxGrid(this, wxID_ANY);
    m_grid->CreateGrid(0, 2);
    m_grid->SetColLabelValue(0, "Register");
    m_grid->SetColLabelValue(1, "Value");
    m_grid->SetRowLabelSize(0);
    m_grid->HideRowLabels();
    m_grid->SetColFormatCustom(1, wxGRID_VALUE_STRING);
    m_grid->SetDefaultCellFont(m_fixedFont);
    m_grid->DisableDragColSize();
    m_grid->DisableDragRowSize();
    m_grid->EnableEditing(false);
    
    sizer->Add(m_grid, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

void RegisterPane::SetCPU(ICore* cpu) {
    m_cpu = cpu;
    if (m_grid->GetNumberRows() > 0) m_grid->DeleteRows(0, m_grid->GetNumberRows());
    
    if (m_cpu) {
        int count = m_cpu->regCount();
        m_grid->AppendRows(count);
        m_prevValues.assign(count, 0);
        
        wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        wxColour bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        for (int i = 0; i < count; ++i) {
            const auto* desc = m_cpu->regDescriptor(i);
            m_grid->SetCellValue(i, 0, desc->name);
            m_grid->SetCellTextColour(i, 0, fg);
            m_grid->SetCellBackgroundColour(i, 0, bg);
            m_prevValues[i] = m_cpu->regRead(i);
        }
    }
    RefreshValues();
}

void RegisterPane::RefreshValues() {
    if (!m_cpu) return;
    
    int count = m_cpu->regCount();
    for (int i = 0; i < count; ++i) {
        const auto* desc = m_cpu->regDescriptor(i);
        uint32_t val = m_cpu->regRead(i);
        
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0');
        if (desc->width == RegWidth::R16) ss << "$" << std::setw(4) << val;
        else ss << "$" << std::setw(2) << val;
        
        m_grid->SetCellValue(i, 1, ss.str());

        wxColour fg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
        wxColour bg;
        if (val != m_prevValues[i]) {
            // Blend a red tint into the window background so it stays legible
            // in both light and dark themes.
            wxColour winBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
            bg = wxColour(
                (winBg.Red()   * 2 + 255) / 3,
                (winBg.Green() * 2 + 80)  / 3,
                (winBg.Blue()  * 2 + 80)  / 3);
            m_prevValues[i] = val;
        } else {
            bg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
        }
        m_grid->SetCellTextColour(i, 1, fg);
        m_grid->SetCellBackgroundColour(i, 1, bg);
    }
    m_grid->AutoSizeColumns();
    m_grid->ForceRefresh();
}
