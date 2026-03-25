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
#include "plugin_loader/plugin_loader.h"
#include "libcore/machine_desc.h"
#include "libcore/machines/machine_registry.h"
#include "libtoolchain/toolchain_registry.h"

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
    void OnTimer(wxTimerEvent& event);

    MachineDescriptor* m_machine = nullptr;
    ICore* m_cpu = nullptr;
    IBus* m_bus = nullptr;
    IDisassembler* m_disasm = nullptr;
    
    RegisterPane* m_regPane;
    MemoryPane* m_memPane;
    DisasmPane* m_disasmPane;
    ConsolePane* m_consolePane;
    
    wxTimer m_timer;
    bool m_running = false;
};

class MmemuApp : public wxApp {
public:
    bool OnInit() override {
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
    toolBar->Realize();
    
    // Status Bar
    CreateStatusBar();
    
    // Main layout: Splitter
    auto* mainSplitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    // Left: Disasm and Memory (vertical split)
    auto* leftSplitter = new wxSplitterWindow(mainSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    
    auto* centerSplitter = new wxSplitterWindow(leftSplitter, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE);
    m_disasmPane = new DisasmPane(centerSplitter);
    m_memPane = new MemoryPane(centerSplitter);
    centerSplitter->SplitHorizontally(m_disasmPane, m_memPane, 300);
    
    m_consolePane = new ConsolePane(leftSplitter);
    leftSplitter->SplitHorizontally(centerSplitter, m_consolePane, 550);
    
    // Right: Register Pane
    m_regPane = new RegisterPane(mainSplitter);
    
    mainSplitter->SplitVertically(leftSplitter, m_regPane, 750);
    mainSplitter->SetMinimumPaneSize(200);
    
    // Bind events
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
    Bind(wxEVT_TIMER, &MmemuFrame::OnTimer, this, ID_GUI_TIMER);
    
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
    if (m_machine) {
        m_cpu->reset();
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

void MmemuFrame::OnTimer(wxTimerEvent& event) {
    (void)event;
    if (m_running && m_cpu) {
        for (int i = 0; i < 1000; ++i) m_cpu->step(); // Run some cycles
    }
    
    if (m_cpu) {
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
        m_memPane->RefreshValues();
    }
}
