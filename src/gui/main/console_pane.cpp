#include "console_pane.h"
#include <iomanip>
#include <sstream>
#include <vector>

ConsolePane::ConsolePane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    m_interpreter = new CliInterpreter(m_ctx, [this](const std::string& out) {
        AppendText(out);
    });

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    m_history = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    m_history->SetDefaultStyle(wxTextAttr(*wxBLACK, *wxWHITE, wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL)));
    
    auto* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    m_prompt = new wxStaticText(this, wxID_ANY, "> ");
    m_prompt->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    
    m_input = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    m_input->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL));
    
    inputSizer->Add(m_prompt, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 5);
    inputSizer->Add(m_input, 1, wxEXPAND);
    
    sizer->Add(m_history, 1, wxEXPAND);
    sizer->Add(inputSizer, 0, wxEXPAND);
    
    SetSizer(sizer);
    
    Bind(wxEVT_TEXT_ENTER, &ConsolePane::OnInput, this);
    
    AppendText("mmemu console ready.\nType 'help' for commands.\n");
}

void ConsolePane::SetContext(ICore* cpu, IBus* bus) {
    m_ctx.cpu = cpu;
    m_ctx.bus = bus;
    // Also try to find toolchain
    if (cpu) {
        m_ctx.disasm = ToolchainRegistry::instance().createDisassembler(cpu->isaName());
        m_ctx.assem = ToolchainRegistry::instance().createAssembler(cpu->isaName());
    }
}

void ConsolePane::SetDebugContext(DebugContext* dbg) {
    m_ctx.dbg = dbg;
}

void ConsolePane::OnInput(wxCommandEvent& event) {
    (void)event;
    std::string line = m_input->GetValue().ToStdString();
    m_input->Clear();
    
    if (line.empty() && !m_interpreter->isAssemblyMode()) return;
    
    AppendText(m_prompt->GetLabel().ToStdString() + line + "\n");
    m_interpreter->processLine(line);
    UpdatePrompt();
}

void ConsolePane::AppendText(const std::string& text) {
    m_history->AppendText(text);
    m_history->ScrollLines(1);
}

void ConsolePane::UpdatePrompt() {
    if (m_interpreter->isAssemblyMode()) {
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << m_interpreter->getAsmAddr() << "> ";
        m_prompt->SetLabel(ss.str());
    } else {
        m_prompt->SetLabel("> ");
    }
    Layout();
}
