#include "plugin_pane_manager.h"
#include <iostream>

PluginPaneManager& PluginPaneManager::instance() {
    static PluginPaneManager inst;
    return inst;
}

void PluginPaneManager::registerPane(const PluginPaneInfo* info) {
    if (!info || !info->paneId) return;
    m_registeredPanes.push_back(*info);
}

static bool isRelevant(const PluginPaneInfo& info, const std::string& machineId) {
    if (!info.machineIds) return true;
    for (int i = 0; info.machineIds[i]; ++i) {
        if (machineId == info.machineIds[i]) return true;
    }
    return false;
}

void PluginPaneManager::onMachineSwitch(const std::string& machineId, wxWindow* parent, wxAuiNotebook* notebook) {
    m_currentMachineId = machineId;
    m_notebook = notebook;

    // Destroy panes that are no longer relevant
    auto it = m_livePanes.begin();
    while (it != m_livePanes.end()) {
        if (!isRelevant(it->second.info, machineId)) {
            if (it->second.window) {
                if (it->second.info.destroyPane) {
                    it->second.info.destroyPane(it->second.window, it->second.info.ctx);
                }
                // Find and delete the page in notebook if it exists
                if (m_notebook) {
                    for (size_t i = 0; i < m_notebook->GetPageCount(); ++i) {
                        if (m_notebook->GetPage(i) == it->second.window) {
                            m_notebook->DeletePage(i);
                            break;
                        }
                    }
                } else if (it->second.window && parent) {
                    it->second.window->Destroy();
                }
            }
            it = m_livePanes.erase(it);
        } else {
            ++it;
        }
    }

    // Create panes that are now relevant but don't exist yet
    for (const auto& info : m_registeredPanes) {
        if (isRelevant(info, machineId) && m_livePanes.find(info.paneId) == m_livePanes.end()) {
            if (info.createPane) {
                void* handle = info.createPane(parent, info.ctx);
                if (handle) {
                    wxWindow* win = static_cast<wxWindow*>(handle);
                    m_livePanes[info.paneId] = {info, win};
                    if (m_notebook) {
                        m_notebook->AddPage(win, info.displayName ? info.displayName : info.paneId);
                    }
                }
            }
        }
    }
}

void PluginPaneManager::tickAll(uint64_t cycles) {
    for (auto& pair : m_livePanes) {
        if (pair.second.info.refreshPane && pair.second.window) {
            pair.second.info.refreshPane(pair.second.window, cycles, pair.second.info.ctx);
        }
    }
}

void PluginPaneManager::populateMenu(wxMenuBar* menuBar) {
    // This could be complex. For now, let's just log.
    // In a real app, we'd find/create menu categories.
}
