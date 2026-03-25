#include "memory_dialogs.h"

FillMemoryDialog::FillMemoryDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Fill Memory", wxDefaultPosition, wxDefaultSize)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* flexSizer = new wxFlexGridSizer(3, 2, 10, 10);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Address (hex):"));
    m_addrCtrl = new wxTextCtrl(this, wxID_ANY, "0000");
    flexSizer->Add(m_addrCtrl, 1, wxEXPAND);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Length (hex):"));
    m_lenCtrl = new wxTextCtrl(this, wxID_ANY, "100");
    flexSizer->Add(m_lenCtrl, 1, wxEXPAND);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Value (hex):"));
    m_valCtrl = new wxTextCtrl(this, wxID_ANY, "00");
    flexSizer->Add(m_valCtrl, 1, wxEXPAND);
    
    sizer->Add(flexSizer, 1, wxEXPAND | wxALL, 15);
    
    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizerAndFit(sizer);
    
    Bind(wxEVT_BUTTON, &FillMemoryDialog::OnOK, this, wxID_OK);
}

void FillMemoryDialog::OnOK(wxCommandEvent& event) {
    try {
        m_address = std::stoul(m_addrCtrl->GetValue().ToStdString(), nullptr, 16);
        m_length = std::stoul(m_lenCtrl->GetValue().ToStdString(), nullptr, 16);
        m_value = (uint8_t)std::stoul(m_valCtrl->GetValue().ToStdString(), nullptr, 16);
        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid hex input!", "Error", wxOK | wxICON_ERROR);
    }
}

CopyMemoryDialog::CopyMemoryDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Copy Memory", wxDefaultPosition, wxDefaultSize)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* flexSizer = new wxFlexGridSizer(3, 2, 10, 10);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Source Address (hex):"));
    m_srcAddrCtrl = new wxTextCtrl(this, wxID_ANY, "0000");
    flexSizer->Add(m_srcAddrCtrl, 1, wxEXPAND);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Length (hex):"));
    m_lenCtrl = new wxTextCtrl(this, wxID_ANY, "100");
    flexSizer->Add(m_lenCtrl, 1, wxEXPAND);
    
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Dest Address (hex):"));
    m_dstAddrCtrl = new wxTextCtrl(this, wxID_ANY, "0000");
    flexSizer->Add(m_dstAddrCtrl, 1, wxEXPAND);
    
    sizer->Add(flexSizer, 1, wxEXPAND | wxALL, 15);
    
    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);
    
    SetSizerAndFit(sizer);
    
    Bind(wxEVT_BUTTON, &CopyMemoryDialog::OnOK, this, wxID_OK);
}

void CopyMemoryDialog::OnOK(wxCommandEvent& event) {
    try {
        m_srcAddr = std::stoul(m_srcAddrCtrl->GetValue().ToStdString(), nullptr, 16);
        m_length = std::stoul(m_lenCtrl->GetValue().ToStdString(), nullptr, 16);
        m_dstAddr = std::stoul(m_dstAddrCtrl->GetValue().ToStdString(), nullptr, 16);
        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid hex input!", "Error", wxOK | wxICON_ERROR);
    }
}
