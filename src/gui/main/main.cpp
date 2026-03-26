#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/artprov.h>
#include "machine_selector.h"
#include "register_pane.h"
#include "memory_pane.h"
#include "disasm_pane.h"
#include "console_pane.h"
#include "dialogs/memory_dialogs.h"
#include "dialogs/assemble_dialog.h"
#include "plugin_loader/main/plugin_loader.h"
#include "plugin_pane_manager.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libtoolchain/main/toolchain_registry.h"

// mmemu - Multi Machine Emulator

enum {
    ID_LOAD_MACHINE = wxID_HIGHEST + 1,
    ID_STEP,
    ID_RUN,
    ID_PAUSE,
    ID_RESET,
    ID_FILL_MEM,
    ID_COPY_MEM,
    ID_ASSEMBLE,
    ID_GOTO_ADDR,
    ID_SEARCH_MEM,
    ID_KBD_FOCUS,
    ID_GUI_TIMER
};

class MmemuFrame : public wxFrame {
public:
    MmemuFrame();
    ~MmemuFrame() { m_timer.Stop(); }

private:
    void OnLoadMachine(wxCommandEvent& event);
    void OnStep(wxCommandEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    void OnFillMem(wxCommandEvent& event);
    void OnCopyMem(wxCommandEvent& event);
    void OnAssemble(wxCommandEvent& event);
    void OnGotoAddr(wxCommandEvent& event);
    void OnSearchMem(wxCommandEvent& event);
    void OnKbdFocus(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    MachineDescriptor* m_machine = nullptr;
    ICore* m_cpu = nullptr;
    IBus* m_bus = nullptr;
    IDisassembler* m_disasm = nullptr;
    
    RegisterPane* m_regPane;
    MemoryPane* m_memPane;
    DisasmPane* m_disasmPane;
    ConsolePane* m_consolePane;
    wxAuiNotebook*   m_notebook = nullptr;

    wxTimer m_timer;
    bool m_running = false;
    bool m_kbdFocus = false;
};

class MmemuApp : public wxApp {
public:
    bool OnInit() override {
        PluginLoader::instance().setPaneRegisterFn([](const PluginPaneInfo* info) {
            PluginPaneManager::instance().registerPane(info);
        });
        PluginLoader::instance().loadFromDir("./lib");
        auto *frame = new MmemuFrame();
        frame->Show(true);
        return true;
    }
};

wxIMPLEMENT_APP(MmemuApp);

MmemuFrame::MmemuFrame()
    : wxFrame(nullptr, wxID_ANY, "mmemu - Multi Machine Emulator",
              wxDefaultPosition, wxSize(1024, 768)),
      m_timer(this, ID_GUI_TIMER)
{
    // Menu
    auto* menuFile = new wxMenu;
    menuFile->Append(ID_LOAD_MACHINE, "Load Machine...\tCtrl-L");
    menuFile->AppendSeparator();
    menuFile->Append(wxID_EXIT);
    
    auto* menuControl = new wxMenu;
    menuControl->Append(ID_STEP, "Step\tF11");
    menuControl->Append(ID_RUN, "Run\tF5");
    menuControl->Append(ID_PAUSE, "Pause\tF6");
    menuControl->AppendSeparator();
    menuControl->Append(ID_RESET, "Reset\tCtrl-R");

    auto* menuDebug = new wxMenu;
    menuDebug->Append(ID_ASSEMBLE, "Assemble...\tCtrl-A");
    menuDebug->AppendSeparator();
    menuDebug->Append(ID_GOTO_ADDR, "Go to Address...\tCtrl-G");
    menuDebug->Append(ID_SEARCH_MEM, "Search Memory...\tCtrl-F");
    menuDebug->AppendSeparator();
    menuDebug->Append(ID_FILL_MEM, "Fill Memory...");
    menuDebug->Append(ID_COPY_MEM, "Copy Memory...");
    
    auto* menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuControl, "&Control");
    menuBar->Append(menuDebug, "&Debug");
    SetMenuBar(menuBar);
    
    // ToolBar
    auto* toolBar = CreateToolBar();
    toolBar->AddTool(ID_LOAD_MACHINE, "Machine", wxArtProvider::GetBitmap(wxART_NEW));
    toolBar->AddSeparator();
    toolBar->AddTool(ID_STEP, "Step", wxArtProvider::GetBitmap(wxART_GO_FORWARD));
    toolBar->AddTool(ID_RUN, "Run", wxArtProvider::GetBitmap(wxART_GO_FORWARD));
    toolBar->AddTool(ID_PAUSE, "Pause", wxArtProvider::GetBitmap(wxART_QUIT));
    toolBar->AddSeparator();
    toolBar->AddTool(ID_GOTO_ADDR, "Go to", wxArtProvider::GetBitmap(wxART_FIND));
    toolBar->AddSeparator();
    toolBar->AddCheckTool(ID_KBD_FOCUS, "Keyboard Focus (Ctrl-Shift-K)", wxArtProvider::GetBitmap(wxART_LIST_VIEW));
    toolBar->Realize();
    
    // Status Bar
    CreateStatusBar();
    
    // Main layout: Splitter
    auto* mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    // Left: Disasm and Memory (vertical split)
    auto* leftSplitter = new wxSplitterWindow(mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    auto* centerSplitter = new wxSplitterWindow(leftSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    auto* notebookSplitter = new wxSplitterWindow(centerSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    m_disasmPane = new DisasmPane(notebookSplitter);
    m_memPane = new MemoryPane(notebookSplitter);
    notebookSplitter->SplitHorizontally(m_disasmPane, m_memPane, 300);
    
    m_notebook = new wxAuiNotebook(centerSplitter, wxID_ANY);
    centerSplitter->SplitVertically(notebookSplitter, m_notebook, 600);
    
    m_consolePane = new ConsolePane(leftSplitter);
    leftSplitter->SplitHorizontally(centerSplitter, m_consolePane, 550);
    
    // Right: Register Pane
    m_regPane = new RegisterPane(mainSplitter);
    
    mainSplitter->SplitVertically(leftSplitter, m_regPane, 750);
    mainSplitter->SetMinimumPaneSize(200);
    
    // Bind events
    Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);
    Bind(wxEVT_MENU, &MmemuFrame::OnLoadMachine, this, ID_LOAD_MACHINE);
    Bind(wxEVT_TOOL, &MmemuFrame::OnLoadMachine, this, ID_LOAD_MACHINE);
    Bind(wxEVT_MENU, &MmemuFrame::OnStep, this, ID_STEP);
    Bind(wxEVT_TOOL, &MmemuFrame::OnStep, this, ID_STEP);
    Bind(wxEVT_MENU, &MmemuFrame::OnRun, this, ID_RUN);
    Bind(wxEVT_TOOL, &MmemuFrame::OnRun, this, ID_RUN);
    Bind(wxEVT_MENU, &MmemuFrame::OnPause, this, ID_PAUSE);
    Bind(wxEVT_TOOL, &MmemuFrame::OnPause, this, ID_PAUSE);
    Bind(wxEVT_MENU, &MmemuFrame::OnReset, this, ID_RESET);
    Bind(wxEVT_MENU, &MmemuFrame::OnFillMem, this, ID_FILL_MEM);
    Bind(wxEVT_MENU, &MmemuFrame::OnCopyMem, this, ID_COPY_MEM);
    Bind(wxEVT_MENU, &MmemuFrame::OnAssemble, this, ID_ASSEMBLE);
    Bind(wxEVT_MENU, &MmemuFrame::OnGotoAddr, this, ID_GOTO_ADDR);
    Bind(wxEVT_TOOL, &MmemuFrame::OnGotoAddr, this, ID_GOTO_ADDR);
    Bind(wxEVT_MENU, &MmemuFrame::OnSearchMem, this, ID_SEARCH_MEM);
    Bind(wxEVT_MENU, &MmemuFrame::OnKbdFocus, this, ID_KBD_FOCUS);
    Bind(wxEVT_TOOL, &MmemuFrame::OnKbdFocus, this, ID_KBD_FOCUS);
    Bind(wxEVT_TIMER, &MmemuFrame::OnTimer, this, ID_GUI_TIMER);
    
    Bind(wxEVT_CHAR_HOOK, &MmemuFrame::OnKeyDown, this);
    Bind(wxEVT_KEY_UP, &MmemuFrame::OnKeyUp, this);

    m_timer.Start(33); // ~30 FPS
}

void MmemuFrame::OnLoadMachine(wxCommandEvent& event) {
    (void)event;
    MachineSelectorDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        std::string id = dialog.GetSelectedMachineId();
        m_machine = MachineRegistry::instance().createMachine(id);
        if (m_machine) {
            m_cpu = m_machine->cpus[0].cpu;
            m_bus = m_machine->buses[0].bus;
            m_disasm = ToolchainRegistry::instance().createDisassembler(m_cpu->isaName());
            
            m_regPane->SetCPU(m_cpu);
            m_memPane->SetBus(m_bus);
            m_disasmPane->SetBus(m_bus);
            m_disasmPane->SetDisassembler(m_disasm);
            m_consolePane->SetContext(m_cpu, m_bus);
            
            if (m_machine->onReset) m_machine->onReset(*m_machine);

            PluginPaneManager::instance().onMachineSwitch(id, this, m_notebook, m_machine);

            SetTitle("mmemu - " + m_machine->displayName);
            SetStatusText("Loaded machine: " + id);
        }
    }
}

void MmemuFrame::OnStep(wxCommandEvent& event) {
    (void)event;
    if (m_cpu) {
        m_cpu->step();
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
    }
}

void MmemuFrame::OnRun(wxCommandEvent& event) {
    (void)event;
    m_running = true;
    SetStatusText("Running...");
}

void MmemuFrame::OnPause(wxCommandEvent& event) {
    (void)event;
    m_running = false;
    SetStatusText("Paused.");
}

void MmemuFrame::OnReset(wxCommandEvent& event) {
    (void)event;
    if (m_machine && m_machine->onReset) {
        m_machine->onReset(*m_machine);
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
    }
}

void MmemuFrame::OnFillMem(wxCommandEvent& event) {
    (void)event;
    if (!m_bus) return;
    FillMemoryDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        uint32_t addr = dialog.GetAddress();
        uint32_t len = dialog.GetLength();
        uint8_t val = dialog.GetValue();
        for (uint32_t i = 0; i < len; ++i) m_bus->write8(addr + i, val);
        m_memPane->RefreshValues();
    }
}

void MmemuFrame::OnCopyMem(wxCommandEvent& event) {
    (void)event;
    if (!m_bus) return;
    CopyMemoryDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        uint32_t src = dialog.GetSrcAddress();
        uint32_t len = dialog.GetLength();
        uint32_t dst = dialog.GetDstAddress();
        std::vector<uint8_t> tmp(len);
        for (uint32_t i = 0; i < len; ++i) tmp[i] = m_bus->read8(src + i);
        for (uint32_t i = 0; i < len; ++i) m_bus->write8(dst + i, tmp[i]);
        m_memPane->RefreshValues();
    }
}

void MmemuFrame::OnAssemble(wxCommandEvent& event) {
    (void)event;
    if (!m_cpu || !m_bus) return;
    AssembleDialog dialog(this, m_cpu->pc());
    if (dialog.ShowModal() == wxID_OK) {
        std::string instr = dialog.GetInstruction();
        uint32_t addr = dialog.GetAddress();
        
        IAssembler* assem = ToolchainRegistry::instance().createAssembler(m_cpu->isaName());
        if (assem) {
            uint8_t opcodes[16];
            int sz = assem->assembleLine(instr, opcodes, sizeof(opcodes));
            if (sz > 0) {
                for (int i = 0; i < sz; ++i) m_bus->write8(addr + i, opcodes[i]);
                m_memPane->RefreshValues();
                m_disasmPane->RefreshValues(m_cpu->pc());
            } else {
                wxMessageBox("Assembly failed!", "Error", wxOK | wxICON_ERROR);
            }
            delete assem;
        }
    }
}

void MmemuFrame::OnGotoAddr(wxCommandEvent& event) {
    (void)event;
    if (!m_cpu) return;
    GotoAddressDialog dialog(this, m_cpu->pc());
    if (dialog.ShowModal() == wxID_OK) {
        uint32_t addr = dialog.GetAddress();
        if (dialog.ShouldSetPC()) {
            m_cpu->setPc(addr);
            m_disasmPane->RefreshValues(m_cpu->pc());
        }
        m_memPane->SetAddress(addr);
        m_memPane->RefreshValues();
    }
}

void MmemuFrame::OnSearchMem(wxCommandEvent& event) {
    (void)event;
    if (!m_bus) return;
    SearchMemoryDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        std::string pattern = dialog.GetPattern();
        bool isHex = dialog.IsHex();
        uint32_t startAddr = dialog.GetStartAddress();
        
        std::vector<uint8_t> bytes;
        if (isHex) {
            std::stringstream ss(pattern);
            std::string byteStr;
            while (ss >> byteStr) {
                try {
                    bytes.push_back((uint8_t)std::stoul(byteStr, nullptr, 16));
                } catch (...) {}
            }
        } else {
            for (char c : pattern) bytes.push_back((uint8_t)c);
        }
        
        if (bytes.empty()) return;
        
        // Simple search (64KB max for now as per VIC-20/C64)
        uint32_t foundAddr = 0xFFFFFFFF;
        for (uint32_t i = startAddr; i < 0x10000 - bytes.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < bytes.size(); ++j) {
                if (m_bus->peek8(i + j) != bytes[j]) {
                    match = false;
                    break;
                }
            }
            if (match) {
                foundAddr = i;
                break;
            }
        }
        
        if (foundAddr != 0xFFFFFFFF) {
            m_memPane->SetAddress(foundAddr);
            m_memPane->RefreshValues();
            SetStatusText(wxString::Format("Pattern found at $%04X", foundAddr));
        } else {
            wxMessageBox("Pattern not found", "Search", wxOK | wxICON_INFORMATION);
        }
    }
}

void MmemuFrame::OnKbdFocus(wxCommandEvent& event) {
    m_kbdFocus = event.IsChecked();
    if (m_kbdFocus) {
        SetStatusText("Keyboard focus active. Press Ctrl-Shift-K to exit.");
    } else {
        SetStatusText("Keyboard focus released.");
    }
}

void MmemuFrame::OnTimer(wxTimerEvent& event) {
    (void)event;
    if (m_running && m_machine && m_machine->schedulerStep) {
        // ~1 MHz VIC-20 at 30 fps needs ~33 333 cycles per frame.
        // Use schedulerStep so the IO registry (VIC, VIA) is ticked each instruction.
        const int CYCLES_PER_FRAME = 33333;
        int ran = 0;
        while (ran < CYCLES_PER_FRAME)
            ran += m_machine->schedulerStep(*m_machine);
    }
    
    if (m_cpu) {
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
        m_memPane->RefreshValues();
        PluginPaneManager::instance().tickAll(m_cpu->cycles());
    }
}

void MmemuFrame::OnKeyDown(wxKeyEvent& event) {
    if (event.GetKeyCode() == 'K' && event.ControlDown() && event.ShiftDown()) {
        m_kbdFocus = !m_kbdFocus;
        GetToolBar()->ToggleTool(ID_KBD_FOCUS, m_kbdFocus);
        if (m_kbdFocus) SetStatusText("Keyboard focus active. Press Ctrl-Shift-K to exit.");
        else SetStatusText("Keyboard focus released.");
        return;
    }

    if (m_kbdFocus && m_machine && m_machine->onKey) {
        // Map common keys to CBM matrix names
        int code = event.GetKeyCode();
        std::string name;
        if (code >= 'A' && code <= 'Z') name = (char)std::tolower(code);
        else if (code >= '0' && code <= '9') name = (char)code;
        else if (code == WXK_SPACE) name = "space";
        else if (code == WXK_RETURN) name = "return";
        else if (code == WXK_CONTROL) name = "control";
        else if (code == WXK_SHIFT) name = "left_shift";
        else if (code == WXK_UP) name = "up";
        else if (code == WXK_DOWN) name = "down";
        else if (code == WXK_LEFT) name = "left";
        else if (code == WXK_RIGHT) name = "right";

        if (!name.empty()) {
            m_machine->onKey(name, true);
        }
    } else {
        event.Skip();
    }
}

void MmemuFrame::OnKeyUp(wxKeyEvent& event) {
    if (m_kbdFocus && m_machine && m_machine->onKey) {
        int code = event.GetKeyCode();
        std::string name;
        if (code >= 'A' && code <= 'Z') name = (char)std::tolower(code);
        else if (code >= '0' && code <= '9') name = (char)code;
        else if (code == WXK_SPACE) name = "space";
        else if (code == WXK_RETURN) name = "return";
        else if (code == WXK_CONTROL) name = "control";
        else if (code == WXK_SHIFT) name = "left_shift";
        else if (code == WXK_UP) name = "up";
        else if (code == WXK_DOWN) name = "down";
        else if (code == WXK_LEFT) name = "left";
        else if (code == WXK_RIGHT) name = "right";

        if (!name.empty()) {
            m_machine->onKey(name, false);
        }
    } else {
        event.Skip();
    }
}
