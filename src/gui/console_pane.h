#pragma once
#include <wx/wx.h>
#include "libcore/machine_desc.h"
#include "libcore/machines/machine_registry.h"
#include "libtoolchain/toolchain_registry.h"
#include "libmem/memory_bus.h"

#include "cli/cli_interpreter.h"

class ConsolePane : public wxPanel {
public:
    ConsolePane(wxWindow* parent);
    void SetContext(ICore* cpu, IBus* bus);

private:
    void OnInput(wxCommandEvent& event);
    void AppendText(const std::string& text);
    void UpdatePrompt();

    CliContext m_ctx;
    CliInterpreter* m_interpreter;
    wxTextCtrl* m_history;
    wxTextCtrl* m_input;
    wxStaticText* m_prompt;
};
