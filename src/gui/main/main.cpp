#include <wx/wx.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/artprov.h>
#include <wx/dnd.h>
#include "machine_selector.h"
#include "register_pane.h"
#include "memory_pane.h"
#include "disasm_pane.h"
#include "console_pane.h"
#include "cartridge_pane.h"
#include "breakpoint_pane.h"
#include "stack_pane.h"
#include "machine_inspector_pane.h"
#include "libdebug/main/debug_context.h"
#include "dialogs/memory_dialogs.h"
#include "dialogs/assemble_dialog.h"
#include "dialogs/image_dialogs.h"
#include "plugin_loader/main/plugin_loader.h"
#include "plugin_pane_manager.h"
#include "ikeyboard_capture_pane.h"
#include "audio_output.h"
#include "libcore/main/machine_desc.h"
#include "libcore/main/machines/machine_registry.h"
#include "libcore/main/image_loader.h"
#include "libdevices/main/iaudio_output.h"
#include "libdevices/main/io_registry.h"
#include "libtoolchain/main/toolchain_registry.h"
#include "gui_ids.h"
#include "screen_pane.h"
#include "include/util/logging.h"
#include <fstream>
#include <cstdio>

// mmemu - Multi Machine Emulator

// ---------------------------------------------------------------------------
// Key mapping helper — must be defined before MmemuApp::FilterEvent.
// Returns the VIC-20 key name for a wxWidgets virtual key code, or "" if
// the key has no VIC-20 equivalent.  Handles both upper and lower case
// letter codes since wxGTK may return either.
// ---------------------------------------------------------------------------
// Maps PC key code to emulated key name for unshifted keys.
// Shift key is handled separately in KeyboardCaptureFilter.
static std::string wxKeyToVic20Name(int code) {
    if (code >= 'A' && code <= 'Z') return std::string(1, (char)('a' + code - 'A'));
    if (code >= 'a' && code <= 'z') return std::string(1, (char)code);
    if (code >= '0' && code <= '9') return std::string(1, (char)code);
    switch (code) {
        case WXK_SPACE:            return "space";
        case WXK_RETURN:           return "return";
        case WXK_CONTROL:          return "control";
        case WXK_UP:               return "up";
        case WXK_DOWN:             return "down";
        case WXK_LEFT:             return "left";
        case WXK_RIGHT:            return "right";
        case WXK_HOME:             return "home";
        case WXK_DELETE:
        case WXK_BACK:             return "delete";
        case WXK_ESCAPE:           return "run_stop";
        case WXK_TAB:              return "tab";
        // PC F1/F3/F5/F7 map to VIC-20 physical function keys.
        case WXK_F1:               return "f1";
        case WXK_F3:               return "f3";
        case WXK_F5:               return "f5";
        case WXK_F7:               return "f7";
        // Punctuation — unshifted only; shifted combos handled by shiftedPetKey()
        case ';':                  return "semicolon";
        case '=':                  return "kpeq";       // PET = key (row 9)
        case '-':                  return "minus";
        case ',':                  return "comma";
        case '.':                  return "period";
        case '/':                  return "slash";
        case '@':                  return "at";
        case '[':                  return "bracket_left";
        case ']':                  return "bracket_right";
        case '\'':                 return "squote";     // PET dedicated ' key
        // Numpad — map to same matrix positions as main keys
        case WXK_NUMPAD0:          return "0";
        case WXK_NUMPAD1:          return "1";
        case WXK_NUMPAD2:          return "2";
        case WXK_NUMPAD3:          return "3";
        case WXK_NUMPAD4:          return "4";
        case WXK_NUMPAD5:          return "5";
        case WXK_NUMPAD6:          return "6";
        case WXK_NUMPAD7:          return "7";
        case WXK_NUMPAD8:          return "8";
        case WXK_NUMPAD9:          return "9";
        case WXK_NUMPAD_ADD:       return "plus";
        case WXK_NUMPAD_SUBTRACT:  return "minus";
        case WXK_NUMPAD_MULTIPLY:  return "asterisk";
        case WXK_NUMPAD_DIVIDE:    return "slash";
        case WXK_NUMPAD_DECIMAL:   return "period";
        case WXK_NUMPAD_ENTER:     return "return";
        default:                   return {};
    }
}

class MmemuFrame : public wxFrame {
public:
    MmemuFrame();
    ~MmemuFrame() { m_timer.Stop(); delete m_audio; delete m_dbg; }

    /** Toggle keyboard-capture mode and sync all UI + app key handler. */
    void setKeyCapture(bool active);

private:
    void OnLoadMachine(wxCommandEvent& event);
    void OnStep(wxCommandEvent& event);
    void OnRun(wxCommandEvent& event);
    void OnPause(wxCommandEvent& event);
    void OnReset(wxCommandEvent& event);
    void OnFillMem(wxCommandEvent& event);
    void OnCopyMem(wxCommandEvent& event);
    void OnSwapMem(wxCommandEvent& event);
    void OnAssemble(wxCommandEvent& event);
    void OnGotoAddr(wxCommandEvent& event);
    void OnSearchMem(wxCommandEvent& event);
    void OnFindNext(wxCommandEvent& event);
    void OnFindPrior(wxCommandEvent& event);
    void OnLoadImage(wxCommandEvent& event);
    void OnAttachCart(wxCommandEvent& event);
    void OnEjectCart(wxCommandEvent& event);
    void OnKbdFocus(wxCommandEvent& event);
    void OnShowBpPane(wxCommandEvent& event);
    void OnShowStackPane(wxCommandEvent& event);
    void OnShowMachinePane(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    void OnCtrlShiftK(wxKeyEvent& event);
    void ShowBreakpointPane();
    void ShowStackPane();
    void ShowMachineInspectorPane();

    MachineDescriptor* m_machine = nullptr;
    ICore* m_cpu = nullptr;
    IBus* m_bus = nullptr;
    IDisassembler* m_disasm = nullptr;
    DebugContext* m_dbg = nullptr;
    AudioOutput* m_audio = nullptr;

    RegisterPane*   m_regPane;
    MemoryPane*     m_memPane;
    DisasmPane*     m_disasmPane;
    ConsolePane*    m_consolePane;
    CartridgePane*  m_cartPane;
    BreakpointPane* m_bpPane    = nullptr;
    StackPane*      m_stackPane = nullptr;
    MachineInspectorPane* m_machineInspectorPane = nullptr;
    wxAuiNotebook*  m_notebook  = nullptr;

    wxTimer m_timer;
    bool m_running = false;
    bool m_kbdFocus = false;
    IKeyboardCapturePane* m_capturePane = nullptr;

    // Search state
    std::vector<uint8_t> m_lastSearchPattern;
    uint32_t             m_lastSearchFoundAddr = 0xFFFFFFFF;
};

/**
 * Global event filter that intercepts keyboard events when capture is active.
 * wxEventFilter::FilterEvent is guaranteed to be called by the wxWidgets event
 * loop for every event before any window sees it — unlike wxApp::FilterEvent,
 * which is port-specific and unreliable on GTK.
 *
 * We intercept wxEVT_CHAR_HOOK (the first keyboard event fired, before
 * accelerators) for key press, and wxEVT_KEY_UP for key release.
 * Ctrl+Shift+K is explicitly passed through so the frame toggle still works.
 *
 * Shift suppression: the PET has dedicated unshifted keys for many symbols that
 * require Shift on a PC keyboard (e.g. PC Shift+1 = PET '!' key, no shift).
 * We buffer the shift key and only forward it to the emulator if the next key
 * press does NOT map to one of those dedicated PET keys.
 */
class KeyboardCaptureFilter : public wxEventFilter {
public:
    int FilterEvent(wxEvent& event) override {
        if (!m_handler) return Event_Skip;

        const wxEventType t = event.GetEventType();
        if (t != wxEVT_CHAR_HOOK && t != wxEVT_KEY_UP)
            return Event_Skip;

        auto& key = static_cast<wxKeyEvent&>(event);
        // Always pass Ctrl+Shift+K through so OnCtrlShiftK can handle it.
        if (key.GetKeyCode() == 'K' && key.ControlDown() && key.ShiftDown())
            return Event_Skip;

        bool down = (t == wxEVT_CHAR_HOOK);
        int  code = key.GetKeyCode();

        // --- Shift key: buffer rather than forward immediately ---
        if (code == WXK_SHIFT) {
            if (down) {
                m_shiftPending = true;
            } else {
                // On release: always clear pending flag and send shift-up
                // (idempotent if shift was never forwarded).
                m_shiftPending = false;
                logKey("left_shift", false);
                m_handler("left_shift", false);
            }
            return Event_Processed;
        }

        // --- Check if Shift+key maps to a PET dedicated (unshifted) key ---
        std::string dedicated;
        if (m_shiftPending || key.ShiftDown())
            dedicated = shiftedPetKey(code);

        if (!dedicated.empty()) {
            // Consume the buffered shift — the PET dedicated key needs no shift.
            m_shiftPending = false;
            logKey(dedicated, down);
            m_handler(dedicated, down);
            return Event_Processed;
        }

        // --- Regular key: flush any pending shift first, then forward ---
        std::string name = wxKeyToVic20Name(code);
        if (!name.empty()) {
            if (m_shiftPending && down) {
                logKey("left_shift", true);
                m_handler("left_shift", true);
                m_shiftPending = false;
            }
            logKey(name, down);
            m_handler(name, down);
            return Event_Processed;
        }

        return Event_Skip;
    }

    std::function<bool(const std::string&, bool)> m_handler;
    std::shared_ptr<spdlog::logger> m_logger;

private:
    bool m_shiftPending = false;

    void logKey(const std::string& name, bool down) {
        if (m_logger) {
            char buf[80];
            snprintf(buf, sizeof(buf), "key%s: %s", down ? "Down" : "Up", name.c_str());
            m_logger->debug(buf);
        }
    }

    // Returns the PET dedicated-key name for a PC Shift+code combo,
    // or "" if the shift should be forwarded normally.
    //
    // GTK delivers the VIRTUAL key code (unshifted) for digit keys but may
    // deliver the COMPOSED Unicode character for punctuation keys.  Both the
    // base key codes (e.g. '\'' = 39) and the composed codes (e.g. '"' = 34)
    // are listed so the mapping works regardless of GTK behaviour.
    static std::string shiftedPetKey(int code) {
        switch (code) {
            // Base key codes (GTK delivers unshifted virtual key)
            case '1':  return "exclaim";    // Shift+1  → !
            case '2':  return "at";         // Shift+2  → @ (PET dedicated key)
            case '3':  return "hash";       // Shift+3  → #
            case '4':  return "dollar";     // Shift+4  → $
            case '5':  return "percent";    // Shift+5  → %
            case '6':  return "uparrow";    // Shift+6  → ^ (PET ↑ char)
            case '7':  return "ampersand";  // Shift+7  → &
            case '8':  return "asterisk";   // Shift+8  → *
            case '9':  return "lparen";     // Shift+9  → (
            case '0':  return "rparen";     // Shift+0  → )
            case '=':  return "plus";       // Shift+=  → +
            case '-':  return "leftarrow";  // Shift+-  → _ (PET ← char)
            case '\'': return "dquote";     // Shift+'  → "
            case ';':  return "colon";      // Shift+;  → :
            case '/':  return "question";   // Shift+/  → ?
            case ',':  return "less";       // Shift+,  → <
            // Composed character codes (GTK may deliver the shifted character)
            case '!':  return "exclaim";
            case '@':  return "at";
            case '#':  return "hash";
            case '$':  return "dollar";
            case '%':  return "percent";
            case '^':  return "uparrow";
            case '&':  return "ampersand";
            case '*':  return "asterisk";
            case '(':  return "lparen";
            case ')':  return "rparen";
            case '+':  return "plus";
            case '_':  return "leftarrow";
            case '"':  return "dquote";     // Shift+' delivered as " (34)
            case ':':  return "colon";
            case '?':  return "question";
            case '<':  return "less";
            default:   return {};
        }
    }
};

class MmemuDropTarget : public wxFileDropTarget {
public:
    MmemuDropTarget(MmemuFrame* frame) : m_frame(frame) {}
    
    bool OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames) override {
        if (filenames.IsEmpty()) return false;
        std::string path = filenames[0].ToStdString();
        
        // Auto-detect type
        auto* loader = ImageLoaderRegistry::instance().findLoader(path);
        if (loader) {
            // It's a program/bin
            wxCommandEvent evt(wxEVT_MENU, ID_LOAD_IMAGE);
            evt.SetString(path);
            m_frame->GetEventHandler()->AddPendingEvent(evt);
            return true;
        }
        
        auto cart = ImageLoaderRegistry::instance().createCartridgeHandler(path);
        if (cart) {
            // It's a cartridge
            wxCommandEvent evt(wxEVT_MENU, ID_ATTACH_CART);
            evt.SetString(path);
            m_frame->GetEventHandler()->AddPendingEvent(evt);
            return true;
        }
        
        wxMessageBox("Unsupported file format.", "Error", wxICON_ERROR);
        return false;
    }

private:
    MmemuFrame* m_frame;
};

static void* createScreenPane(void* parentHandle, void* /*ctx*/) {
    auto* parent = static_cast<wxWindow*>(parentHandle);
    auto* pane = new ScreenPane(parent);
    return static_cast<void*>(pane);
}

static void onScreenMachineLoad(void* paneHandle, MachineDescriptor* desc, void* /*ctx*/) {
    auto* pane = static_cast<ScreenPane*>(paneHandle);
    IVideoOutput* vid = nullptr;
    if (desc && desc->ioRegistry) {
        std::vector<IOHandler*> handlers;
        desc->ioRegistry->enumerate(handlers);
        for (auto* h : handlers) {
            if ((vid = dynamic_cast<IVideoOutput*>(h))) break;
        }
    }
    pane->SetVideoOutput(vid);
}

static void refreshScreenPane(void* paneHandle, uint64_t /*cycles*/, void* /*ctx*/) {
    static_cast<ScreenPane*>(paneHandle)->RefreshFrame();
}

static PluginPaneInfo s_builtInScreenPane = {
    "screen", "Screen", nullptr, nullptr,
    createScreenPane, nullptr, refreshScreenPane, onScreenMachineLoad, nullptr
};

class MmemuApp : public wxApp {
public:
    bool OnInit() override {
        LogRegistry::instance().init();
        m_keyFilter.m_logger = LogRegistry::instance().getLogger("gui.keyboard");
        wxEvtHandler::AddFilter(&m_keyFilter);
        PluginLoader::instance().setPaneRegisterFn([](const PluginPaneInfo* info) {
            PluginPaneManager::instance().registerPane(info);
        });
        PluginPaneManager::instance().registerPane(&s_builtInScreenPane);
        PluginLoader::instance().loadFromDir("./lib");
        auto *frame = new MmemuFrame();
        frame->Show(true);
        return true;
    }

    int OnExit() override {
        wxEvtHandler::RemoveFilter(&m_keyFilter);
        return wxApp::OnExit();
    }

    KeyboardCaptureFilter m_keyFilter;
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
    menuControl->AppendSeparator();
    menuControl->Append(ID_LOAD_IMAGE, "Load Image...\tCtrl-I");
    menuControl->Append(ID_ATTACH_CART, "Attach Cartridge...");
    menuControl->Append(ID_EJECT_CART, "Eject Cartridge");

    auto* menuDebug = new wxMenu;
    menuDebug->Append(ID_ASSEMBLE, "Assemble...\tCtrl-A");
    menuDebug->AppendSeparator();
    menuDebug->Append(ID_GOTO_ADDR, "Go to Address...\tCtrl-G");
    menuDebug->Append(ID_SEARCH_MEM, "Search Memory...\tCtrl-F");
    menuDebug->Append(ID_FIND_NEXT,  "Find Next\tF3");
    menuDebug->Append(ID_FIND_PRIOR, "Find Prior\tShift-F3");
    menuDebug->AppendSeparator();
    menuDebug->Append(ID_FILL_MEM, "Fill Memory...");
    menuDebug->Append(ID_COPY_MEM, "Copy Memory...");
    menuDebug->Append(ID_SWAP_MEM, "Swap Memory...");
    menuDebug->AppendSeparator();
    menuDebug->Append(ID_SHOW_BP_PANE,    "Breakpoints\tCtrl-B");
    menuDebug->Append(ID_SHOW_STACK_PANE, "Stack Trace\tCtrl-T");
    menuDebug->Append(ID_SHOW_MACHINE_PANE, "Machine Explorer\tCtrl-M");
    
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
    m_machineInspectorPane = new MachineInspectorPane(m_notebook);
    m_notebook->AddPage(m_machineInspectorPane, "Machine");
    m_cartPane = new CartridgePane(m_notebook);
    m_notebook->AddPage(m_cartPane, "Cartridge");
    m_bpPane = new BreakpointPane(m_notebook);
    m_notebook->AddPage(m_bpPane, "Breakpoints");
    m_stackPane = new StackPane(m_notebook);
    m_notebook->AddPage(m_stackPane, "Stack");
    
    centerSplitter->SplitVertically(notebookSplitter, m_notebook, 600);
    
    m_consolePane = new ConsolePane(leftSplitter);
    leftSplitter->SplitHorizontally(centerSplitter, m_consolePane, 550);
    
    // Right: Register Pane
    m_regPane = new RegisterPane(mainSplitter);
    
    mainSplitter->SplitVertically(leftSplitter, m_regPane, 750);
    mainSplitter->SetMinimumPaneSize(200);
    
    SetDropTarget(new MmemuDropTarget(this));
    
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
    Bind(wxEVT_MENU, &MmemuFrame::OnSwapMem, this, ID_SWAP_MEM);
    Bind(wxEVT_MENU, &MmemuFrame::OnAssemble, this, ID_ASSEMBLE);
    Bind(wxEVT_MENU, &MmemuFrame::OnGotoAddr, this, ID_GOTO_ADDR);
    Bind(wxEVT_TOOL, &MmemuFrame::OnGotoAddr, this, ID_GOTO_ADDR);
    Bind(wxEVT_MENU, &MmemuFrame::OnSearchMem, this, ID_SEARCH_MEM);
    Bind(wxEVT_MENU, &MmemuFrame::OnFindNext,  this, ID_FIND_NEXT);
    Bind(wxEVT_MENU, &MmemuFrame::OnFindPrior, this, ID_FIND_PRIOR);
    Bind(wxEVT_MENU, &MmemuFrame::OnLoadImage, this, ID_LOAD_IMAGE);
    Bind(wxEVT_MENU, &MmemuFrame::OnAttachCart, this, ID_ATTACH_CART);
    Bind(wxEVT_MENU, &MmemuFrame::OnEjectCart, this, ID_EJECT_CART);
    Bind(wxEVT_MENU, &MmemuFrame::OnKbdFocus,   this, ID_KBD_FOCUS);
    Bind(wxEVT_MENU, &MmemuFrame::OnShowBpPane,    this, ID_SHOW_BP_PANE);
    Bind(wxEVT_MENU, &MmemuFrame::OnShowStackPane, this, ID_SHOW_STACK_PANE);
    Bind(wxEVT_MENU, &MmemuFrame::OnShowMachinePane, this, ID_SHOW_MACHINE_PANE);
    Bind(wxEVT_TOOL, &MmemuFrame::OnKbdFocus, this, ID_KBD_FOCUS);
    Bind(wxEVT_TIMER, &MmemuFrame::OnTimer, this, ID_GUI_TIMER);
    
    // Ctrl+Shift+K toggles capture; FilterEvent routes all other keys.
    Bind(wxEVT_CHAR_HOOK, &MmemuFrame::OnCtrlShiftK, this);

    m_timer.Start(33); // ~30 FPS
}

void MmemuFrame::OnLoadMachine(wxCommandEvent& event) {
    (void)event;
    MachineSelectorDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        std::string id = dialog.GetSelectedMachineId();
        MachineDescriptor* md = MachineRegistry::instance().createMachine(id);
        if (md) {
            if (m_machine) delete m_machine;
            m_machine = md;
            m_cpu = m_machine->cpus[0].cpu;
            // Use CPU data bus for memory pane if available, otherwise fallback to machine bus
            m_bus = m_machine->cpus[0].dataBus ? m_machine->cpus[0].dataBus : m_machine->buses[0].bus;
            if (m_disasm) delete m_disasm;
            m_disasm = ToolchainRegistry::instance().createDisassembler(m_cpu->isaName());
            
            delete m_dbg;
            m_dbg = new DebugContext(m_cpu, m_machine->buses[0].bus);
            m_cpu->setObserver(m_dbg);
            m_machine->buses[0].bus->setObserver(m_dbg);

            m_regPane->SetCPU(m_cpu);
            m_memPane->SetBus(m_bus);
            m_disasmPane->SetBus(m_bus);
            m_disasmPane->SetDisassembler(m_disasm);
            m_consolePane->SetContext(m_cpu, m_bus);
            m_consolePane->SetDebugContext(m_dbg);
            m_bpPane->SetDebugContext(m_dbg);
            m_stackPane->SetDebugContext(m_dbg);
            m_stackPane->SetGotoCallback([this](uint32_t addr){ m_disasmPane->GoTo(addr); });
            m_cartPane->SetBus(m_bus);
            m_machineInspectorPane->setMachine(m_machine);
            
            if (m_machine->onReset) m_machine->onReset(*m_machine);

            // Discover and start audio output for any IAudioOutput device in
            // the new machine's IO registry.
            delete m_audio;
            m_audio = nullptr;
            if (m_machine->ioRegistry) {
                std::vector<IOHandler*> handlers;
                m_machine->ioRegistry->enumerate(handlers);
                for (auto* h : handlers) {
                    if (auto* ao = dynamic_cast<IAudioOutput*>(h)) {
                        m_audio = new AudioOutput();
                        if (!m_audio->start(ao)) {
                            delete m_audio;
                            m_audio = nullptr;
                        }
                        break;
                    }
                }
            }

            // Release capture before switching machine.
            m_capturePane = nullptr;
            setKeyCapture(false);

            PluginPaneManager::instance().onMachineSwitch(id, m_notebook, m_notebook, m_machine);

            // Wire the screen pane's Capture button (if present) to setKeyCapture().
            wxWindow* screenWin = PluginPaneManager::instance().getPaneWindow("screen");
            if (screenWin) {
                m_capturePane = dynamic_cast<IKeyboardCapturePane*>(screenWin);
                if (m_capturePane)
                    m_capturePane->SetCaptureCallback([this](bool active) { setKeyCapture(active); });
            }

            std::string statusMsg = "Loaded machine: " + id;
            if (!m_machine->onKey) statusMsg += " (no keyboard)";
            SetTitle("mmemu - " + m_machine->displayName);
            SetStatusText(statusMsg);
        }
    }
}

void MmemuFrame::OnStep(wxCommandEvent& event) {
    (void)event;
    if (m_cpu) {
        m_cpu->step();
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
        m_stackPane->RefreshValues();
    }
}

void MmemuFrame::OnRun(wxCommandEvent& event) {
    (void)event;
    if (m_dbg) m_dbg->resume();
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

void MmemuFrame::OnSwapMem(wxCommandEvent& event) {
    (void)event;
    if (!m_bus) return;
    SwapMemoryDialog dialog(this);
    if (dialog.ShowModal() == wxID_OK) {
        uint32_t addr1 = dialog.GetAddress1();
        uint32_t len = dialog.GetLength();
        uint32_t addr2 = dialog.GetAddress2();
        for (uint32_t i = 0; i < len; ++i) {
            uint8_t v1 = m_bus->read8(addr1 + i);
            uint8_t v2 = m_bus->read8(addr2 + i);
            m_bus->write8(addr1 + i, v2);
            m_bus->write8(addr2 + i, v1);
        }
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
    uint32_t mask = m_bus->config().addrMask;
    SearchMemoryDialog dialog(this, mask);
    if (dialog.ShowModal() == wxID_OK) {
        std::string pattern = dialog.GetPattern();
        bool isHex = dialog.IsHex();
        uint32_t startAddr = dialog.GetStartAddress() & mask;
        uint32_t length = dialog.GetLength();
        
        m_lastSearchPattern.clear();
        if (isHex) {
            std::string cleanHex;
            for (char c : pattern) { if (isxdigit(c)) cleanHex += c; }
            for (size_t i = 0; i + 1 < cleanHex.length(); i += 2) {
                try {
                    m_lastSearchPattern.push_back((uint8_t)std::stoul(cleanHex.substr(i, 2), nullptr, 16));
                } catch (...) {}
            }
        } else {
            for (char c : pattern) m_lastSearchPattern.push_back((uint8_t)c);
        }
        
        if (m_lastSearchPattern.empty()) return;
        
        uint32_t endAddr = (startAddr + length > mask + 1) ? (mask + 1) : (startAddr + length);
        uint32_t foundAddr = 0xFFFFFFFF;
        
        if (endAddr >= m_lastSearchPattern.size()) {
            for (uint32_t i = startAddr; i <= endAddr - m_lastSearchPattern.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < m_lastSearchPattern.size(); ++j) {
                    if (m_bus->peek8((i + j) & mask) != m_lastSearchPattern[j]) {
                        match = false; break;
                    }
                }
                if (match) { foundAddr = i; break; }
            }
        }
        
        m_lastSearchFoundAddr = foundAddr;
        if (foundAddr != 0xFFFFFFFF) {
            m_memPane->SetAddress(foundAddr);
            m_memPane->RefreshValues();
            SetStatusText(wxString::Format("Pattern found at $%X", foundAddr));
        } else {
            wxMessageBox("Pattern not found", "Search", wxOK | wxICON_INFORMATION);
        }
    }
}

void MmemuFrame::OnFindNext(wxCommandEvent& event) {
    if (!m_bus || m_lastSearchPattern.empty()) return;
    uint32_t mask = m_bus->config().addrMask;
    uint32_t startAddr = (m_lastSearchFoundAddr == 0xFFFFFFFF) ? 0 : (m_lastSearchFoundAddr + 1) & mask;
    
    uint32_t foundAddr = 0xFFFFFFFF;
    // Search forward from startAddr to end of memory, then wrap around once
    for (uint32_t i = 0; i < mask + 1; ++i) {
        uint32_t curr = (startAddr + i) & mask;
        if (curr + m_lastSearchPattern.size() > mask + 1) continue;
        
        bool match = true;
        for (size_t j = 0; j < m_lastSearchPattern.size(); ++j) {
            if (m_bus->peek8((curr + j) & mask) != m_lastSearchPattern[j]) {
                match = false; break;
            }
        }
        if (match) { foundAddr = curr; break; }
    }

    if (foundAddr != 0xFFFFFFFF) {
        m_lastSearchFoundAddr = foundAddr;
        m_memPane->SetAddress(foundAddr);
        m_memPane->RefreshValues();
        SetStatusText(wxString::Format("Next pattern found at $%X", foundAddr));
    } else {
        SetStatusText("No further occurrences found.");
    }
}

void MmemuFrame::OnFindPrior(wxCommandEvent& event) {
    if (!m_bus || m_lastSearchPattern.empty()) return;
    uint32_t mask = m_bus->config().addrMask;
    uint32_t startAddr = (m_lastSearchFoundAddr == 0xFFFFFFFF) ? mask : (m_lastSearchFoundAddr - 1) & mask;
    
    uint32_t foundAddr = 0xFFFFFFFF;
    // Search backward from startAddr
    for (uint32_t i = 0; i < mask + 1; ++i) {
        uint32_t curr = (startAddr - i) & mask;
        if (curr + m_lastSearchPattern.size() > mask + 1) continue;
        
        bool match = true;
        for (size_t j = 0; j < m_lastSearchPattern.size(); ++j) {
            if (m_bus->peek8((curr + j) & mask) != m_lastSearchPattern[j]) {
                match = false; break;
            }
        }
        if (match) { foundAddr = curr; break; }
    }

    if (foundAddr != 0xFFFFFFFF) {
        m_lastSearchFoundAddr = foundAddr;
        m_memPane->SetAddress(foundAddr);
        m_memPane->RefreshValues();
        SetStatusText(wxString::Format("Prior pattern found at $%X", foundAddr));
    } else {
        SetStatusText("No prior occurrences found.");
    }
}

void MmemuFrame::OnLoadImage(wxCommandEvent& event) {
    if (!m_bus) return;
    std::string path = event.GetString().ToStdString();
    LoadImageDialog dialog(this, path);
    if (dialog.ShowModal() == wxID_OK) {
        path = dialog.GetPath();
        uint32_t addr = dialog.GetAddress();
        auto* loader = ImageLoaderRegistry::instance().findLoader(path);
        if (loader) {
            if (loader->load(path, m_bus, m_machine, addr)) {
                SetStatusText("Loaded " + path);
                if (dialog.GetAutoStart()) {
                    uint32_t startAddr = addr;
                    if (startAddr == 0 && std::string(loader->name()).find("PRG") != std::string::npos) {
                        std::ifstream f(path, std::ios::binary);
                        uint8_t h[2];
                        f.read((char*)h, 2);
                        startAddr = h[0] | (h[1] << 8);
                    }
                    m_cpu->setPc(startAddr);
                    m_running = true;
                    m_regPane->RefreshValues();
                    m_disasmPane->RefreshValues(m_cpu->pc());
                }
                m_memPane->RefreshValues();
            } else {
                wxMessageBox("Failed to load image.", "Error", wxICON_ERROR);
            }
        }
    }
}

void MmemuFrame::OnAttachCart(wxCommandEvent& event) {
    if (!m_bus) return;
    std::string path = event.GetString().ToStdString();
    if (path.empty()) {
        wxFileDialog openFileDialog(this, "Attach Cartridge", "", "",
                           "Cartridge files (*.crt;*.car)|*.crt;*.car", 
                           wxFD_OPEN | wxFD_FILE_MUST_EXIST);
        if (openFileDialog.ShowModal() == wxID_OK) {
            path = openFileDialog.GetPath().ToStdString();
        } else {
            return;
        }
    }
    
    auto handler = ImageLoaderRegistry::instance().createCartridgeHandler(path);
    if (handler) {
        if (handler->attach(m_bus, m_machine)) {
            ImageLoaderRegistry::instance().setActiveCartridge(m_bus, std::move(handler));
            m_cartPane->RefreshMetadata();
            // Reset machine
            if (m_machine->onReset) m_machine->onReset(*m_machine);
            m_regPane->RefreshValues();
            m_disasmPane->RefreshValues(m_cpu->pc());
            m_memPane->RefreshValues();
            SetStatusText("Attached cartridge " + path);
        } else {
            wxMessageBox("Failed to attach cartridge.", "Error", wxICON_ERROR);
        }
    } else {
        wxMessageBox("Unsupported cartridge format.", "Error", wxICON_ERROR);
    }
}

void MmemuFrame::OnEjectCart(wxCommandEvent&) {
    if (!m_bus) return;
    auto* cart = ImageLoaderRegistry::instance().getActiveCartridge(m_bus);
    if (cart) {
        cart->eject(m_bus);
        ImageLoaderRegistry::instance().setActiveCartridge(m_bus, nullptr);
        m_cartPane->RefreshMetadata();
        if (m_machine->onReset) m_machine->onReset(*m_machine);
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
        m_memPane->RefreshValues();
        SetStatusText("Cartridge ejected.");
    }
}

void MmemuFrame::setKeyCapture(bool active) {
    m_kbdFocus = active;
    GetToolBar()->ToggleTool(ID_KBD_FOCUS, active);
    if (m_capturePane) m_capturePane->SetCaptureActive(active);
    auto* app = static_cast<MmemuApp*>(wxTheApp);
    if (active && m_machine && m_machine->onKey) {
        app->m_keyFilter.m_handler = m_machine->onKey;
    } else {
        app->m_keyFilter.m_handler = nullptr;
        // Release any modifier keys that may have been left pressed (e.g., the
        // Ctrl and Shift that were pressed as part of the Ctrl+Shift+K shortcut).
        if (m_machine && m_machine->onKey) {
            for (const char* mod : {"control", "left_shift", "right_shift"})
                m_machine->onKey(mod, false);
        }
    }
    SetStatusText(active
        ? "Keyboard captured \u2014 Ctrl+Shift+K or button to release."
        : "Keyboard released.");
}

void MmemuFrame::ShowBreakpointPane() {
    // Find the pane in the notebook (it may have been closed by the user).
    for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
        if (m_notebook->GetPage(i) == m_bpPane) {
            m_notebook->SetSelection(i);
            return;
        }
    }
    // Not present — re-add it.
    m_notebook->AddPage(m_bpPane, "Breakpoints", true);
}

void MmemuFrame::OnShowBpPane(wxCommandEvent&) {
    ShowBreakpointPane();
}

void MmemuFrame::ShowStackPane() {
    for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
        if (m_notebook->GetPage(i) == m_stackPane) {
            m_notebook->SetSelection(i);
            return;
        }
    }
    m_notebook->AddPage(m_stackPane, "Stack", true);
}

void MmemuFrame::OnShowStackPane(wxCommandEvent&) {
    ShowStackPane();
}

void MmemuFrame::OnKbdFocus(wxCommandEvent& event) {
    setKeyCapture(event.IsChecked());
}

void MmemuFrame::ShowMachineInspectorPane() {
    for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
        if (m_notebook->GetPage(i) == m_machineInspectorPane) {
            m_notebook->SetSelection(i);
            return;
        }
    }
    m_notebook->AddPage(m_machineInspectorPane, "Machine", true);
}

void MmemuFrame::OnShowMachinePane(wxCommandEvent&) {
    ShowMachineInspectorPane();
}

void MmemuFrame::OnCtrlShiftK(wxKeyEvent& event) {
    if (event.GetKeyCode() == 'K' && event.ControlDown() && event.ShiftDown()) {
        setKeyCapture(!m_kbdFocus);
        return; // consumed
    }
    event.Skip();
}

void MmemuFrame::OnTimer(wxTimerEvent& event) {
    (void)event;
    if (m_running && m_machine && m_machine->schedulerStep) {
        // ~1 MHz VIC-20 at 30 fps needs ~33 333 cycles per frame.
        // Use schedulerStep so the IO registry (VIC, VIA) is ticked each instruction.
        const int CYCLES_PER_FRAME = 33333;
        int ran = 0;
        while (ran < CYCLES_PER_FRAME) {
            ran += m_machine->schedulerStep(*m_machine);
            if (m_dbg && m_dbg->isPaused()) {
                m_running = false;
                SetStatusText(m_dbg->lastHitMessage());
                if (m_bpPane) m_bpPane->RefreshValues();
                break;
            }
        }
    }

    if (m_cpu) {
        m_regPane->RefreshValues();
        m_disasmPane->RefreshValues(m_cpu->pc());
        m_memPane->RefreshValues();
        m_stackPane->RefreshValues();
        PluginPaneManager::instance().tickAll(m_cpu->cycles());
    }
}
