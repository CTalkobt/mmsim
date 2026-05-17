#include "assemble_dialog.h"
#include "libtoolchain/main/toolchain_registry.h"
#include <iomanip>
#include <sstream>

AssembleDialog::AssembleDialog(wxWindow* parent, uint32_t currentAddr,
                               const std::string& currentAssembler)
    : wxDialog(parent, wxID_ANY, "Assemble Instruction", wxDefaultPosition, wxDefaultSize)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* flexSizer = new wxFlexGridSizer(3, 2, 10, 10);
    flexSizer->AddGrowableCol(1, 1);

    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << currentAddr;

    // Assembler picker
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Assembler:"), 0, wxALIGN_CENTER_VERTICAL);
    m_asmPicker = new wxChoice(this, wxID_ANY);

    auto names = ToolchainRegistry::instance().getAssemblerNames();
    m_asmPicker->Append("(default)");
    int sel = 0;
    for (size_t i = 0; i < names.size(); ++i) {
        m_asmPicker->Append(names[i]);
        if (names[i] == currentAssembler) sel = (int)(i + 1);
    }
    m_asmPicker->SetSelection(sel);
    flexSizer->Add(m_asmPicker, 1, wxEXPAND);

    // Address field
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Address (hex):"), 0, wxALIGN_CENTER_VERTICAL);
    m_addrCtrl = new wxTextCtrl(this, wxID_ANY, ss.str());
    flexSizer->Add(m_addrCtrl, 1, wxEXPAND);

    // Instruction field
    flexSizer->Add(new wxStaticText(this, wxID_ANY, "Instruction:"), 0, wxALIGN_CENTER_VERTICAL);
    m_instrCtrl = new wxTextCtrl(this, wxID_ANY, "nop");
    flexSizer->Add(m_instrCtrl, 1, wxEXPAND);

    sizer->Add(flexSizer, 1, wxEXPAND | wxALL, 15);

    auto* btnSizer = CreateButtonSizer(wxOK | wxCANCEL);
    sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    SetSizerAndFit(sizer);
    if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
    Bind(wxEVT_BUTTON, &AssembleDialog::OnOK, this, wxID_OK);
}

void AssembleDialog::OnOK(wxCommandEvent& event) {
    try {
        m_address = std::stoul(m_addrCtrl->GetValue().ToStdString(), nullptr, 16);
        m_instruction = m_instrCtrl->GetValue().ToStdString();

        int sel = m_asmPicker->GetSelection();
        if (sel > 0) {
            m_selectedAssembler = m_asmPicker->GetString(sel).ToStdString();
        } else {
            m_selectedAssembler.clear();  // Use default resolution
        }

        EndModal(wxID_OK);
    } catch (...) {
        wxMessageBox("Invalid hex input!", "Error", wxOK | wxICON_ERROR);
    }
}
