#pragma once
#include <wx/wx.h>
#include <vector>
#include <string>
#include <stack>
#include <cmath>

enum class CalcMode {
    Float,
    Integer
};

enum class CalcBase {
    Dec,
    Hex,
    Bin,
    Oct
};

class DebugContext;

class CalculatorDialog : public wxDialog {
public:
    CalculatorDialog(wxWindow* parent, DebugContext* dbg = nullptr);

private:
    void CreateControls();
    void OnButtonClick(wxCommandEvent& event);
    void OnChar(wxKeyEvent& event);

    void UpdateDisplay();
    void ProcessInput(const wxString& label);
    
    // Math operations
    void PushResult(double val);

    wxTextCtrl* m_display;
    wxStaticText* m_modeStatus;
    DebugContext* m_dbg;
    
    CalcMode m_mode = CalcMode::Float;
    CalcBase m_base = CalcBase::Dec;
    bool m_shift = false;
    
    std::string m_currentInput;
    double m_lastResult = 0;
    std::vector<double> m_memory; // 8 results
    
    struct ButtonInfo {
        int id;
        wxString label;
        wxString shiftLabel;
        wxString shortcut;
    };
    std::vector<ButtonInfo> m_buttons;

    void OnPaint(wxPaintEvent& event);
    DECLARE_EVENT_TABLE()
};
