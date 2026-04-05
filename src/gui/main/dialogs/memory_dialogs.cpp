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
    if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
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
    if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
    Bind(wxEVT_BUTTON, &CopyMemoryDialog::OnOK, this, wxID_OK);
}
void CopyMemoryDialog::OnOK(wxCommandEvent& event) {
    (void)event;
    m_srcAddr = std::stoul(m_srcAddrCtrl->GetValue().ToStdString(), nullptr, 16);
    m_length = std::stoul(m_lenCtrl->GetValue().ToStdString(), nullptr, 16);
    m_dstAddr = std::stoul(m_dstAddrCtrl->GetValue().ToStdString(), nullptr, 16);
    EndModal(wxID_OK);
}

// --- GotoAddressDialog ---

GotoAddressDialog::GotoAddressDialog(wxWindow* parent, uint32_t currentAddr)
    : wxDialog(parent, wxID_ANY, "Go to Address")
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 2, 5, 5);
    grid->Add(new wxStaticText(this, wxID_ANY, "Address ($hex):"), 0, wxALIGN_CENTER_VERTICAL);
    m_addrCtrl = new wxTextCtrl(this, wxID_ANY, wxString::Format("%04X", currentAddr));
    grid->Add(m_addrCtrl, 1, wxEXPAND);

    sizer->Add(grid, 0, wxEXPAND | wxALL, 10);

    m_setPCCheck = new wxCheckBox(this, wxID_ANY, "Set PC to this address");
    sizer->Add(m_setPCCheck, 0, wxEXPAND | wxALL, 10);

    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(sizer);
    if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
    Bind(wxEVT_BUTTON, &GotoAddressDialog::OnOK, this, wxID_OK);
}

void GotoAddressDialog::OnOK(wxCommandEvent& event) {
    (void)event;
    try {
        m_address = std::stoul(m_addrCtrl->GetValue().ToStdString(), nullptr, 16);
        m_setPC = m_setPCCheck->GetValue();
        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid address format", "Error", wxOK | wxICON_ERROR);
    }
}

// --- SearchMemoryDialog ---

SearchMemoryDialog::SearchMemoryDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Search Memory")
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    auto* grid = new wxFlexGridSizer(2, 2, 5, 5);
    grid->Add(new wxStaticText(this, wxID_ANY, "Pattern:"), 0, wxALIGN_CENTER_VERTICAL);
    m_patternCtrl = new wxTextCtrl(this, wxID_ANY, "");
    grid->Add(m_patternCtrl, 1, wxEXPAND);

    grid->Add(new wxStaticText(this, wxID_ANY, "Start Address ($hex):"), 0, wxALIGN_CENTER_VERTICAL);
    m_startAddrCtrl = new wxTextCtrl(this, wxID_ANY, "0000");
    grid->Add(m_startAddrCtrl, 1, wxEXPAND);

    sizer->Add(grid, 0, wxEXPAND | wxALL, 10);

    auto* radioSizer = new wxBoxSizer(wxHORIZONTAL);
    m_hexRadio = new wxRadioButton(this, wxID_ANY, "Hex", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    m_asciiRadio = new wxRadioButton(this, wxID_ANY, "ASCII");
    radioSizer->Add(m_hexRadio, 0, wxALL, 5);
    radioSizer->Add(m_asciiRadio, 0, wxALL, 5);
    sizer->Add(radioSizer, 0, wxCENTER);

    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxEXPAND | wxALL, 10);

    SetSizerAndFit(sizer);
    if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
    Bind(wxEVT_BUTTON, &SearchMemoryDialog::OnOK, this, wxID_OK);
}

void SearchMemoryDialog::OnOK(wxCommandEvent& event) {
    (void)event;
    m_pattern = m_patternCtrl->GetValue().ToStdString();
    m_isHex = m_hexRadio->GetValue();
    try {
        m_startAddr = std::stoul(m_startAddrCtrl->GetValue().ToStdString(), nullptr, 16);
        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid start address", "Error", wxOK | wxICON_ERROR);
    }
}
