#pragma once
#include <wx/wx.h>
#include <cstdint>
#include <string>

class LoadImageDialog : public wxDialog {
public:
    LoadImageDialog(wxWindow* parent, const std::string& defaultPath = "");
    
    std::string GetPath() const { return m_path; }
    uint32_t GetAddress() const { return m_address; }
    bool GetAutoStart() const { return m_autoStart; }

private:
    void OnBrowse(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_pathCtrl;
    wxTextCtrl* m_addrCtrl;
    wxCheckBox* m_runCheck;
    
    std::string m_path;
    uint32_t m_address = 0;
    bool m_autoStart = false;
};
