#pragma once

#include <wx/wx.h>
#include <wx/treectrl.h>
#include <wx/combobox.h>
#include "libcore/main/machine_desc.h"
#include "libdevices/main/io_handler.h"

/**
 * A pane that displays detailed information about individual devices.
 */
class DeviceInfoPane : public wxPanel {
public:
    DeviceInfoPane(wxWindow* parent);

    void setMachine(MachineDescriptor* desc);
    void refreshValues();

private:
    void onDeviceSelected(wxCommandEvent& evt);
    void onPaintBitmaps(wxPaintEvent& evt);

    wxComboBox* m_deviceSelector;
    wxTreeCtrl* m_tree;
    wxScrolledWindow* m_bitmapWindow;
    MachineDescriptor* m_desc = nullptr;
    DeviceInfo m_currentInfo;
};
