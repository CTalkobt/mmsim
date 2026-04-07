#include "machine_inspector_pane.h"

MachineInspectorPane::MachineInspectorPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    m_tree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE | wxTR_HIDE_ROOT);
    sizer->Add(m_tree, 1, wxEXPAND | wxALL, 5);
    SetSizer(sizer);
}

void MachineInspectorPane::setMachine(MachineDescriptor* desc) {
    m_desc = desc;
    refreshValues();
}

void MachineInspectorPane::refreshValues() {
    m_tree->DeleteAllItems();
    if (!m_desc) return;

    wxTreeItemId root = m_tree->AddRoot("Machine");
    
    // Display name and basics
    wxTreeItemId info = m_tree->AppendItem(root, "Identity");
    m_tree->AppendItem(info, "ID: " + m_desc->machineId);
    m_tree->AppendItem(info, "Name: " + m_desc->displayName);
    m_tree->AppendItem(info, "License: " + m_desc->licenseClass);

    // Hierarchy from JSON
    if (!m_desc->sourceSpec.is_null()) {
        wxTreeItemId config = m_tree->AppendItem(root, "Configuration");
        populateNode(config, m_desc->sourceSpec);
        m_tree->Expand(config);
    }

    m_tree->Expand(info);
}

void MachineInspectorPane::populateNode(const wxTreeItemId& parentNode, const nlohmann::json& spec) {
    if (spec.is_object()) {
        for (auto it = spec.begin(); it != spec.end(); ++it) {
            std::string key = it.key();
            const auto& val = it.value();
            
            if (val.is_primitive()) {
                std::stringstream ss;
                ss << key << ": " << val;
                m_tree->AppendItem(parentNode, ss.str());
            } else {
                wxTreeItemId childNode = m_tree->AppendItem(parentNode, key);
                populateNode(childNode, val);
            }
        }
    } else if (spec.is_array()) {
        int i = 0;
        for (const auto& item : spec) {
            std::stringstream ss;
            ss << "[" << i << "]";
            wxTreeItemId childNode = m_tree->AppendItem(parentNode, ss.str());
            populateNode(childNode, item);
            i++;
        }
    }
}
