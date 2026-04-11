#pragma once
#include <wx/wx.h>
#include <vector>
#include "libdevices/main/io_handler.h"

struct MachineDescriptor;

class DriveStatusPane : public wxPanel {
public:
    DriveStatusPane(wxWindow* parent);
    void SetMachine(MachineDescriptor* md);
    void RefreshStatus();

private:
    struct DriveInfo {
        int unit;
        IOHandler* handler;
        wxStaticText* label;
        wxStaticText* fileLabel;
        wxPanel* led;
    };

    void OnMount(wxCommandEvent& event);
    void OnEject(wxCommandEvent& event);

    MachineDescriptor* m_md = nullptr;
    std::vector<DriveInfo> m_drives;
    wxScrolledWindow* m_scroll;
    wxBoxSizer* m_mainSizer;
};
