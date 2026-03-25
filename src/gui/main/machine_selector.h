#pragma once
#include <wx/wx.h>
#include <vector>
#include <string>

class MachineSelectorDialog : public wxDialog {
public:
    MachineSelectorDialog(wxWindow* parent);
    std::string GetSelectedMachineId() const { return m_selectedId; }

private:
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

    wxListBox* m_machineList;
    std::string m_selectedId;
    std::vector<std::string> m_ids;
};
