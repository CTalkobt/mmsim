#include "cartridge_pane.h"
#include "libcore/main/image_loader.h"
#include "libcore/main/machine_desc.h"
#include "gui_ids.h"
#include <iomanip>
#include <sstream>

CartridgePane::CartridgePane(wxWindow* parent)
    : wxPanel(parent)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(this, wxID_ANY, "Cartridge Information"), 0, wxALL | wxALIGN_CENTER, 10);
    
    m_infoText = new wxStaticText(this, wxID_ANY, "No cartridge attached.", wxDefaultPosition, wxDefaultSize, wxST_NO_AUTORESIZE | wxALIGN_LEFT);
    sizer->Add(m_infoText, 1, wxEXPAND | wxALL, 10);
    
    m_ejectBtn = new wxButton(this, wxID_ANY, "Eject Cartridge");
    m_ejectBtn->Enable(false);
    sizer->Add(m_ejectBtn, 0, wxALL | wxALIGN_CENTER, 10);
    
    SetSizer(sizer);
    
    m_ejectBtn->Bind(wxEVT_BUTTON, &CartridgePane::OnEject, this);
}

void CartridgePane::SetBus(IBus* bus) {
    m_bus = bus;
    RefreshMetadata();
}

void CartridgePane::RefreshMetadata() {
    if (!m_bus) {
        m_infoText->SetLabel("No machine active.");
        m_ejectBtn->Enable(false);
        return;
    }
    
    auto* cart = ImageLoaderRegistry::instance().getActiveCartridge(m_bus);
    if (cart) {
        auto md = cart->metadata();
        std::stringstream ss;
        ss << "Name: " << md.displayName << "\n"
           << "Type: " << md.type << "\n"
           << "Path: " << md.imagePath << "\n"
           << "Banks: " << md.bankCount << "\n"
           << "Range: $" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << md.startAddr
           << " - $" << std::setw(4) << md.endAddr << "\n";
        m_infoText->SetLabel(ss.str());
        m_ejectBtn->Enable(true);
    } else {
        m_infoText->SetLabel("No cartridge attached.");
        m_ejectBtn->Enable(false);
    }
    Layout();
}

void CartridgePane::OnEject(wxCommandEvent&) {
    if (!m_bus) return;
    auto* cart = ImageLoaderRegistry::instance().getActiveCartridge(m_bus);
    if (cart) {
        cart->eject(m_bus);
        ImageLoaderRegistry::instance().setActiveCartridge(m_bus, nullptr);
        RefreshMetadata();
        
        // Notify frame to reset machine (using standard Reset command)
        wxCommandEvent evt(wxEVT_MENU, ID_RESET);
        wxWindow* top = wxGetTopLevelParent(this);
        if (top) top->GetEventHandler()->AddPendingEvent(evt);
    }
}
