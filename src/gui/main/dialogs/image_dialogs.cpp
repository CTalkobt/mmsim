#include "image_dialogs.h"
#include <iomanip>
#include <sstream>

LoadImageDialog::LoadImageDialog(wxWindow* parent, const std::string& defaultPath)
    : wxDialog(parent, wxID_ANY, "Load Image")
{
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    // Path
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Image File:"), 0, wxALL, 5);
    auto* pathSizer = new wxBoxSizer(wxHORIZONTAL);
    m_pathCtrl = new wxTextCtrl(this, wxID_ANY, defaultPath, wxDefaultPosition, wxSize(300, -1));
    pathSizer->Add(m_pathCtrl, 1, wxALL, 5);
    auto* browseBtn = new wxButton(this, wxID_ANY, "Browse...");
    pathSizer->Add(browseBtn, 0, wxALL, 5);
    mainSizer->Add(pathSizer, 0, wxEXPAND);
    
    // Address
    mainSizer->Add(new wxStaticText(this, wxID_ANY, "Load Address (Hex, optional for .prg):"), 0, wxALL, 5);
    m_addrCtrl = new wxTextCtrl(this, wxID_ANY, "");
    mainSizer->Add(m_addrCtrl, 0, wxEXPAND | wxALL, 5);
    
    // Run Checkbox
    m_runCheck = new wxCheckBox(this, wxID_ANY, "Run after load");
    mainSizer->Add(m_runCheck, 0, wxALL, 5);
    
    mainSizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
    
    browseBtn->Bind(wxEVT_BUTTON, &LoadImageDialog::OnBrowse, this);
    Bind(wxEVT_BUTTON, &LoadImageDialog::OnOK, this, wxID_OK);
}

void LoadImageDialog::OnBrowse(wxCommandEvent&) {
    wxFileDialog openFileDialog(this, "Open Image File", "", "",
                       "All supported|*.prg;*.bin;*.xex;*.crt;*.car|PRG files (*.prg)|*.prg|Binary files (*.bin)|*.bin|Atari XEX (*.xex)|*.xex|Cartridge files (*.crt;*.car)|*.crt;*.car", 
                       wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_OK) {
        m_pathCtrl->SetValue(openFileDialog.GetPath());
    }
}

void LoadImageDialog::OnOK(wxCommandEvent&) {
    m_path = m_pathCtrl->GetValue().ToStdString();
    std::string addrStr = m_addrCtrl->GetValue().ToStdString();
    if (!addrStr.empty()) {
        try {
            m_address = std::stoul(addrStr, nullptr, 16);
        } catch (...) {
            wxMessageBox("Invalid hexadecimal address.", "Error", wxICON_ERROR);
            return;
        }
    } else {
        m_address = 0;
    }
    m_autoStart = m_runCheck->GetValue();
    EndModal(wxID_OK);
}
