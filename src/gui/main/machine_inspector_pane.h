#pragma once

#include <wx/wx.h>
#include <wx/treectrl.h>
#include "libcore/main/machine_desc.h"

/**
 * A pane that displays the hierarchical layout of the machine,
 * initially based on its JSON specification.
 */
class MachineInspectorPane : public wxPanel {
public:
    MachineInspectorPane(wxWindow* parent);

    void setMachine(MachineDescriptor* desc);
    void refreshValues();

private:
    void populateNode(const wxTreeItemId& parentNode, const nlohmann::json& spec);

    wxTreeCtrl* m_tree;
    MachineDescriptor* m_desc = nullptr;
};
