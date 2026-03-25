#pragma once
#include <wx/wx.h>
#include <cstdint>
#include <string>

class FillMemoryDialog : public wxDialog {
public:
    FillMemoryDialog(wxWindow* parent);
    uint32_t GetAddress() const { return m_address; }
    uint32_t GetLength() const { return m_length; }
    uint8_t GetValue() const { return m_value; }

private:
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_addrCtrl;
    wxTextCtrl* m_lenCtrl;
    wxTextCtrl* m_valCtrl;
    uint32_t m_address = 0, m_length = 0;
    uint8_t m_value = 0;
};

class CopyMemoryDialog : public wxDialog {
public:
    CopyMemoryDialog(wxWindow* parent);
    uint32_t GetSrcAddress() const { return m_srcAddr; }
    uint32_t GetLength() const { return m_length; }
    uint32_t GetDstAddress() const { return m_dstAddr; }

private:
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_srcAddrCtrl;
    wxTextCtrl* m_lenCtrl;
    wxTextCtrl* m_dstAddrCtrl;
    uint32_t m_srcAddr = 0, m_length = 0, m_dstAddr = 0;
};
