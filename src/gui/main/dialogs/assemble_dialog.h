#pragma once
#include <wx/wx.h>
#include <cstdint>
#include <string>

class AssembleDialog : public wxDialog {
public:
    AssembleDialog(wxWindow* parent, uint32_t currentAddr,
                   const std::string& currentAssembler = "");
    uint32_t GetAddress() const { return m_address; }
    std::string GetInstruction() const { return m_instruction; }
    std::string GetSelectedAssembler() const { return m_selectedAssembler; }

private:
    void OnOK(wxCommandEvent& event);

    wxTextCtrl* m_addrCtrl;
    wxTextCtrl* m_instrCtrl;
    wxChoice*   m_asmPicker;
    uint32_t m_address = 0;
    std::string m_instruction;
    std::string m_selectedAssembler;
};
