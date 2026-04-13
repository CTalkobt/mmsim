#pragma once
#include <wx/wx.h>
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include "libmem/main/memory_bus.h"

#include "cli/main/cli_interpreter.h"

class ConsolePane : public wxPanel {
public:
    ConsolePane(wxWindow* parent);
    void SetContext(ICore* cpu, IBus* bus);
    void SetMachine(MachineDescriptor* machine);
    void SetDebugContext(DebugContext* dbg);

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
