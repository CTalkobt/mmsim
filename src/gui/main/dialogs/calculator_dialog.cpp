#include "calculator_dialog.h"
#include "libdebug/main/expression_evaluator.h"
#include <wx/grid.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <bitset>

enum {
    ID_CALC_BTN_START = 10000,
    ID_CALC_DISPLAY
};

BEGIN_EVENT_TABLE(CalculatorDialog, wxDialog)
    EVT_PAINT(CalculatorDialog::OnPaint)
END_EVENT_TABLE()

CalculatorDialog::CalculatorDialog(wxWindow* parent, DebugContext* dbg)
    : wxDialog(parent, wxID_ANY, "Engineering Calculator", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE),
      m_dbg(dbg)
{
    m_memory.resize(8, 0.0);
    CreateControls();
    
    Bind(wxEVT_CHAR_HOOK, &CalculatorDialog::OnChar, this);
    
    UpdateDisplay();
}

void CalculatorDialog::CreateControls()
{
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    
    m_display = new wxTextCtrl(this, ID_CALC_DISPLAY, "", wxDefaultPosition, wxDefaultSize, wxTE_RIGHT | wxTE_PROCESS_ENTER);
    m_display->SetFont(wxFont(18, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD));
    m_display->SetMinSize(wxSize(400, -1));
    mainSizer->Add(m_display, 0, wxEXPAND | wxALL, 10);
    
    m_display->Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent&) {
        ProcessInput("=");
    });

    m_modeStatus = new wxStaticText(this, wxID_ANY, "Float | Dec");
    wxBoxSizer* statusSizer = new wxBoxSizer(wxHORIZONTAL);
    statusSizer->Add(m_modeStatus, 1, wxALIGN_LEFT | wxLEFT | wxRIGHT, 10);
    mainSizer->Add(statusSizer, 0, wxEXPAND | wxBOTTOM, 5);
    
    wxGridSizer* gridSizer = new wxGridSizer(7, 4, 5, 5);
    
    // Initialize 28 buttons
    for (int i = 0; i < 28; ++i) {
        m_buttons.push_back({ID_CALC_BTN_START + i, "", "", ""});
        wxButton* btn = new wxButton(this, ID_CALC_BTN_START + i, "", wxDefaultPosition, wxSize(80, 40));
        gridSizer->Add(btn, 1, wxEXPAND);
        Bind(wxEVT_BUTTON, &CalculatorDialog::OnButtonClick, this, ID_CALC_BTN_START + i);
    }
    
    mainSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 10);
    
    SetSizerAndFit(mainSizer);
}

void CalculatorDialog::OnButtonClick(wxCommandEvent& event)
{
    int id = event.GetId();
    int idx = id - ID_CALC_BTN_START;
    if (idx >= 0 && idx < (int)m_buttons.size()) {
        wxString label = m_shift ? m_buttons[idx].shiftLabel : m_buttons[idx].label;
        ProcessInput(label);
    }
}

void CalculatorDialog::OnChar(wxKeyEvent& event)
{
    int keyCode = event.GetKeyCode();
    wxWindow* focus = wxWindow::FindFocus();
    
    // If focus is in display, let it handle normal characters unless it's a special shortcut
    if (focus == m_display) {
        if (keyCode == WXK_ESCAPE) {
            ProcessInput("AC");
            return;
        }
        event.Skip();
        return;
    }

    if (keyCode >= '0' && keyCode <= '9') {
        ProcessInput(wxString::Format("%c", (char)keyCode));
    } else if (keyCode == '+') ProcessInput("+");
    else if (keyCode == '-') ProcessInput("-");
    else if (keyCode == '*') ProcessInput("*");
    else if (keyCode == '/') ProcessInput("/");
    else if (keyCode == '(') ProcessInput("(");
    else if (keyCode == ')') ProcessInput(")");
    else if (keyCode == '^') ProcessInput("^");
    else if (keyCode == '.') ProcessInput(".");
    else if (keyCode == 'A' || keyCode == 'a') ProcessInput("A");
    else if (keyCode == 'B' || keyCode == 'b') ProcessInput("B");
    else if (keyCode == 'C' || keyCode == 'c') ProcessInput("C");
    else if (keyCode == 'D' || keyCode == 'd') ProcessInput("D");
    else if (keyCode == 'E' || keyCode == 'e') ProcessInput("E");
    else if (keyCode == 'F' || keyCode == 'f') ProcessInput("F");
    else if (keyCode == 'S' || keyCode == 's') ProcessInput("Shift");
    else if (keyCode == 'M' || keyCode == 'm') ProcessInput("Mode");
    else if (keyCode == WXK_RETURN || keyCode == WXK_NUMPAD_ENTER) ProcessInput("=");
    else if (keyCode == WXK_ESCAPE) ProcessInput("AC");
    else if (keyCode == WXK_BACK) {
        wxString val = m_display->GetValue();
        if (!val.empty()) {
            val.RemoveLast();
            m_display->SetValue(val);
            m_display->SetInsertionPointEnd();
        }
    } else {
        event.Skip();
    }
}

void CalculatorDialog::UpdateDisplay()
{
    // If m_currentInput was just updated by a calculation or button click, sync display
    // but don't overwrite if the user is in the middle of typing? 
    // Actually, buttons update m_currentInput, typing updates m_display text.
    // For consistency, let's just make m_currentInput always follow m_display for buttons.
    
    wxString modeStr = (m_mode == CalcMode::Float) ? "Float" : "Integer";
    wxString baseStr;
    if (m_base == CalcBase::Dec) baseStr = "Dec";
    else if (m_base == CalcBase::Hex) baseStr = "Hex";
    else if (m_base == CalcBase::Bin) baseStr = "Bin";
    else if (m_base == CalcBase::Oct) baseStr = "Oct";
    
    m_modeStatus->SetLabel(wxString::Format("%s | %s%s", modeStr, baseStr, m_shift ? " | SHIFT" : ""));
    
    struct LayoutEntry { wxString label; wxString shift; wxString shortcut; };
    std::vector<LayoutEntry> layout(28);
    
    layout[0] = {"Shift", "Shift", "s"};
    layout[1] = {"Mode", "Prec", "m"};
    layout[2] = {"Base", "Base", "b"};
    layout[3] = {"AC", "AC", "Esc"};
    
    if (m_base == CalcBase::Hex) {
        layout[4] = {"A", "sin", "a"};    layout[5] = {"B", "cos", "b"};    layout[6] = {"C", "tan", "c"};    layout[7] = {"/", "<<", "/"};
        layout[8] = {"D", "asin", "d"};   layout[9] = {"E", "acos", "e"};   layout[10] = {"F", "atan", "f"};  layout[11] = {"*", ">>", "*"};
        layout[12] = {"7", "x^2", "7"};   layout[13] = {"8", "sqrt", "8"};  layout[14] = {"9", "x^n", "9"};   layout[15] = {"-", "mod", "-"};
        layout[16] = {"4", "1's", "4"};   layout[17] = {"5", "2's", "5"};   layout[18] = {"6", "neg", "6"};   layout[19] = {"+", "abs", "+"};
        layout[20] = {"1", "Rcl0", "1"};  layout[21] = {"2", "Rcl1", "2"};  layout[22] = {"3", "Rcl2", "3"};  layout[23] = {"^", "Rcl3", "^"};
        layout[24] = {"0", "Rcl4", "0"};  layout[25] = {".", "Rcl5", "."};  layout[26] = {"(", "Rcl6", "("};  layout[27] = {"=", "Ans", "Enter"};
    } else if (m_base == CalcBase::Bin) {
        layout[4] = {"sin", "asin", ""};  layout[5] = {"cos", "acos", ""};  layout[6] = {"tan", "atan", ""};  layout[7] = {"/", "sqrt", "/"};
        layout[8] = {"log", "exp", ""};   layout[9] = {"mod", "mod", "%"};  layout[10] = {"<<", "<<", ""};    layout[11] = {"*", ">>", "*"};
        layout[12] = {"neg", "abs", ""};  layout[13] = {"1's", "1's", ""};  layout[14] = {"2's", "2's", ""};  layout[15] = {"-", "Rcl0", "-"};
        layout[16] = {"Rcl1", "Rcl4", ""}; layout[17] = {"Rcl2", "Rcl5", ""}; layout[18] = {"Rcl3", "Rcl6", ""}; layout[19] = {"+", "Rcl7", "+"};
        layout[20] = {"1", "1", "1"};     layout[21] = {")", ")", ")"};     layout[22] = {"^", "Ans", "^"};   layout[23] = {"=", "Ans", "Enter"};
        layout[24] = {"0", "0", "0"};     layout[25] = {".", ".", "."};     layout[26] = {"(", "(", "("};     layout[27] = {"=", "Ans", "Enter"};
    } else { // Dec or Oct
        layout[4] = {"sin", "asin", ""};  layout[5] = {"cos", "acos", ""};  layout[6] = {"tan", "atan", ""};  layout[7] = {"/", "sqrt", "/"};
        layout[8] = {"log", "exp", ""};   layout[9] = {"<<", "<<", ""};     layout[10] = {">>", ">>", ""};    layout[11] = {"*", "mod", "*"};
        layout[12] = {"7", "Rcl0", "7"};  layout[13] = {"8", "Rcl1", "8"};  layout[14] = {"9", "Rcl2", "9"};  layout[15] = {"-", "neg", "-"};
        layout[16] = {"4", "Rcl3", "4"};  layout[17] = {"5", "Rcl4", "5"};  layout[18] = {"6", "Rcl5", "6"};  layout[19] = {"+", "abs", "+"};
        layout[20] = {"1", "Rcl6", "1"};  layout[21] = {"2", "Rcl7", "2"};  layout[22] = {"3", "Ans", "3"};   layout[23] = {"(", "(", "("};
        layout[24] = {"0", "0", "0"};     layout[25] = {".", ".", "."};     layout[26] = {")", ")", ")"};     layout[27] = {"=", "Ans", "Enter"};
    }

    // Ensure Enter is always at 27
    layout[27] = {"=", "Ans", "Enter"};

    for (int i = 0; i < 28; ++i) {
        m_buttons[i].label = layout[i].label;
        m_buttons[i].shiftLabel = layout[i].shift;
        m_buttons[i].shortcut = layout[i].shortcut;
        
        wxButton* btn = wxDynamicCast(FindWindow(ID_CALC_BTN_START + i), wxButton);
        if (btn) {
            btn->SetLabel(m_shift ? m_buttons[i].shiftLabel : m_buttons[i].label);
            btn->SetToolTip("Shift: " + m_buttons[i].shiftLabel + " (" + m_buttons[i].shortcut + ")");
            btn->Enable(!m_buttons[i].label.empty());
        }
    }
}

static std::string formatValue(double val, CalcMode mode, CalcBase base) {
    if (mode == CalcMode::Integer || base != CalcBase::Dec) {
        long long iv = (long long)val;
        std::stringstream ss;
        if (base == CalcBase::Hex) ss << "$" << std::uppercase << std::hex << iv;
        else if (base == CalcBase::Oct) ss << "0" << std::oct << iv;
        else if (base == CalcBase::Bin) ss << "%" << std::bitset<32>(iv).to_string().substr(std::bitset<32>(iv).to_string().find_first_not_of('0'));
        else ss << iv;
        std::string s = ss.str();
        if (s == "%" || s == "$") s += "0";
        return s;
    } else {
        std::stringstream ss;
        ss << std::setprecision(10) << val;
        return ss.str();
    }
}

void CalculatorDialog::ProcessInput(const wxString& label)
{
    if (label.empty()) return;

    if (label == "Shift") {
        m_shift = !m_shift;
        UpdateDisplay();
        return;
    }
    
    if (label == "AC") {
        m_display->Clear();
    } else if (label == "Mode") {
        m_mode = (m_mode == CalcMode::Float) ? CalcMode::Integer : CalcMode::Float;
    } else if (label == "Base") {
        if (m_base == CalcBase::Dec) m_base = CalcBase::Hex;
        else if (m_base == CalcBase::Hex) m_base = CalcBase::Bin;
        else if (m_base == CalcBase::Bin) m_base = CalcBase::Oct;
        else m_base = CalcBase::Dec;
    } else if (label == "=") {
        double res;
        wxString expr = m_display->GetValue();
        int base = 10;
        if (m_base == CalcBase::Hex) base = 16;
        else if (m_base == CalcBase::Bin) base = 2;
        else if (m_base == CalcBase::Oct) base = 8;

        if (ExpressionEvaluator::evaluate(expr.ToStdString(), m_dbg, res, base)) {
            PushResult(res);
            m_display->SetValue(formatValue(res, m_mode, m_base));
        } else {
            m_display->SetValue("Error");
        }
        m_display->SetInsertionPointEnd();
    } else if (label == "Ans") {
        m_display->WriteText(std::to_string(m_lastResult));
    } else if (label.StartsWith("Rcl")) {
        int idx = wxAtoi(label.substr(3));
        if (idx >= 0 && idx < 8) {
            m_display->WriteText(std::to_string(m_memory[idx]));
        }
    } else if (label == "sin") m_display->WriteText("sin(");
    else if (label == "cos") m_display->WriteText("cos(");
    else if (label == "tan") m_display->WriteText("tan(");
    else if (label == "asin") m_display->WriteText("asin(");
    else if (label == "acos") m_display->WriteText("acos(");
    else if (label == "atan") m_display->WriteText("atan(");
    else if (label == "sqrt") m_display->WriteText("sqrt(");
    else if (label == "log") m_display->WriteText("log(");
    else if (label == "exp") m_display->WriteText("exp(");
    else if (label == "abs") m_display->WriteText("abs(");
    else if (label == "x^2") m_display->WriteText("^2");
    else if (label == "x^n") m_display->WriteText("^");
    else if (label == "<<") m_display->WriteText("<<");
    else if (label == ">>") m_display->WriteText(">>");
    else if (label == "1's") {
        double val;
        int base = 10;
        if (m_base == CalcBase::Hex) base = 16;
        else if (m_base == CalcBase::Bin) base = 2;
        else if (m_base == CalcBase::Oct) base = 8;
        if (ExpressionEvaluator::evaluate(m_display->GetValue().ToStdString(), m_dbg, val, base)) {
            m_display->SetValue(std::to_string(~(long long)val));
        } else { m_display->SetValue("Error"); }
    } else if (label == "2's") {
        double val;
        int base = 10;
        if (m_base == CalcBase::Hex) base = 16;
        else if (m_base == CalcBase::Bin) base = 2;
        else if (m_base == CalcBase::Oct) base = 8;
        if (ExpressionEvaluator::evaluate(m_display->GetValue().ToStdString(), m_dbg, val, base)) {
            m_display->SetValue(std::to_string((~(long long)val) + 1));
        } else { m_display->SetValue("Error"); }
    } else if (label == "neg") {
        wxString val = m_display->GetValue();
        if (val.empty()) m_display->SetValue("-");
        else if (val[0] == '-') m_display->SetValue(val.substr(1));
        else m_display->SetValue("-" + val);
    } else if (label == "mod") {
        m_display->WriteText("%");
    } else {
        m_display->WriteText(label);
    }
    
    if (m_shift) {
        m_shift = false;
    }
    UpdateDisplay();
}

void CalculatorDialog::PushResult(double val)
{
    m_lastResult = val;
    for (int i = 7; i > 0; --i) m_memory[i] = m_memory[i-1];
    m_memory[0] = val;
}

void CalculatorDialog::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
}
