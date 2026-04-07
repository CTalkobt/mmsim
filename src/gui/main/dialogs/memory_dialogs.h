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

class SwapMemoryDialog : public wxDialog {
public:
    SwapMemoryDialog(wxWindow* parent);
    uint32_t GetAddress1() const { return m_addr1; }
    uint32_t GetLength() const { return m_length; }
    uint32_t GetAddress2() const { return m_addr2; }

private:
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_addr1Ctrl;
    wxTextCtrl* m_lenCtrl;
    wxTextCtrl* m_addr2Ctrl;
    uint32_t m_addr1 = 0, m_length = 0, m_addr2 = 0;
};

class GotoAddressDialog : public wxDialog {
public:
    GotoAddressDialog(wxWindow* parent, uint32_t currentAddr = 0);
    uint32_t GetAddress() const { return m_address; }
    bool ShouldSetPC() const { return m_setPC; }

private:
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_addrCtrl;
    wxCheckBox* m_setPCCheck;
    uint32_t m_address = 0;
    bool m_setPC = false;
};

class SearchMemoryDialog : public wxDialog {
public:
    SearchMemoryDialog(wxWindow* parent, uint32_t maxAddr = 0xFFFF);
    std::string GetPattern() const { return m_pattern; }
    bool IsHex() const { return m_isHex; }
    uint32_t GetStartAddress() const { return m_startAddr; }
    uint32_t GetLength() const { return m_length; }

private:
    void OnOK(wxCommandEvent& event);
    
    wxTextCtrl* m_patternCtrl;
    wxRadioButton* m_hexRadio;
    wxRadioButton* m_asciiRadio;
    wxTextCtrl* m_startAddrCtrl;
    wxTextCtrl* m_lenCtrl;
    
    std::string m_pattern;
    bool m_isHex = true;
    uint32_t m_startAddr = 0;
    uint32_t m_length = 0;
};
