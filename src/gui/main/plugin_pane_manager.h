#pragma once

#include "mmemu_plugin_api.h"
#include <wx/wx.h>
#include <wx/aui/auibook.h>
#include <string>
#include <vector>
#include <map>

/**
 * Singleton manager for plugin-provided GUI panes.
 */
class PluginPaneManager {
public:
    static PluginPaneManager& instance();

    /**
     * Registers a new pane info.
     */
    void registerPane(const PluginPaneInfo* info);

    /**
     * Called when a machine is loaded or switched.
     */
    void onMachineSwitch(const std::string& machineId, wxWindow* parent, wxAuiNotebook* notebook);

    /**
     * Periodic refresh for all live panes.
     */
    void tickAll(uint64_t cycles);

    /**
     * Integration with host menus.
     */
    void populateMenu(wxMenuBar* menuBar);

private:
    PluginPaneManager() = default;

    struct LivePane {
        PluginPaneInfo info;
        wxWindow* window = nullptr;
    };

    std::vector<PluginPaneInfo> m_registeredPanes;
    std::map<std::string, LivePane> m_livePanes; // paneId -> LivePane
    std::string m_currentMachineId;
    wxAuiNotebook* m_notebook = nullptr;
};
