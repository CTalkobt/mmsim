#include "assemble_dialog.h"
#include <iomanip>
#include <sstream>

AssembleDialog::AssembleDialog(wxWindow* parent, uint32_t currentAddr)
    : wxDialog(parent, wxID_ANY, "Assemble Instruction", wxDefaultPosition, wxDefaultSize)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* flexSizer = new wxFlexGridSizer(2, 2, 10, 10);
    
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << currentAddr;
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Address (hex):"));
    m_addrCtrl = new wxTextCtrl(this, wxID_ANY, ss.str());
    flexSizer->Add(m_addrCtrl, 1, wxEXPAND);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Instruction:"));
    m_instrCtrl = new wxTextCtrl(this, wxID_ANY, "nop");
    flexSizer->Add(m_instrCtrl, 1, wxEXPAND);
    
    sizer->Add(flexSizer, 1, wxEXPAND | wxALL, 15);
    
    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizerAndFit(sizer);
    
    Bind(wxEVT_BUTTON, &AssembleDialog::OnOK, this, wxID_OK);
}

void AssembleDialog::OnOK(wxCommandEvent& event) {
    try {
        m_address = std::stoul(m_addrCtrl->GetValue().ToStdString(), nullptr, 16);
        m_instruction = m_instrCtrl->GetValue().ToStdString();
        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid hex input!", "Error", wxOK | wxICON_ERROR);
    }
}
