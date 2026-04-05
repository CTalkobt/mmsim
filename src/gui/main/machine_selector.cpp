#include "machine_selector.h"
#include "libcore/main/machines/machine_registry.h"

MachineSelectorDialog::MachineSelectorDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "Select Machine", wxDefaultPosition, wxSize(300, 400))
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_machineList = new wxListBox(this, wxID_ANY);
    MachineRegistry::instance().enumerate(m_ids);
    for (const auto& id : m_ids) {
        m_machineList->Append(id);
    }
    
    if (!m_ids.empty()) m_machineList->SetSelection(0);
    
    sizer->Add(m_machineList, 1, wxEXPAND | wxALL, 10);
    
    auto* btnSizer = new wxStdDialogButtonSizer();
    auto* okBtn = new wxButton(this, wxID_OK);
    auto* cancelBtn = new wxButton(this, wxID_CANCEL);
    btnSizer->AddButton(okBtn);
    btnSizer->AddButton(cancelBtn);
    btnSizer->Realize();
    okBtn->SetDefault();

    sizer->Add(btnSizer, 0, wxALIGN_RIGHT | wxALL, 10);

    SetSizer(sizer);

    Bind(wxEVT_BUTTON, &MachineSelectorDialog::OnOK, this, wxID_OK);
    Bind(wxEVT_BUTTON, &MachineSelectorDialog::OnCancel, this, wxID_CANCEL);
    m_machineList->Bind(wxEVT_LISTBOX_DCLICK, [this](wxCommandEvent& e){ OnOK(e); });
}

void MachineSelectorDialog::OnOK(wxCommandEvent& event) {
    int sel = m_machineList->GetSelection();
    if (sel != wxNOT_FOUND) {
        m_selectedId = m_ids[sel];
        EndModal(wxID_OK);
    }
}

void MachineSelectorDialog::OnCancel(wxCommandEvent& event) {
    EndModal(wxID_CANCEL);
}
