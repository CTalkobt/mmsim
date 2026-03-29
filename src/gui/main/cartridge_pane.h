#pragma once
#include <wx/wx.h>
#include "libmem/main/ibus.h"

class CartridgePane : public wxPanel {
public:
    CartridgePane(wxWindow* parent);
    
    void SetBus(IBus* bus);
    void RefreshMetadata();

private:
    void OnEject(wxCommandEvent& event);
    
    IBus* m_bus = nullptr;
    wxStaticText* m_infoText;
    wxButton* m_ejectBtn;
};
