#include <wx/wx.h>

// mmemu - Multi Machine Emulator
// GUI entry point — placeholder until implementation begins

class MmemuApp : public wxApp {
public:
    bool OnInit() override;
};

class MmemuFrame : public wxFrame {
public:
    MmemuFrame();
};

wxIMPLEMENT_APP(MmemuApp);

bool MmemuApp::OnInit() {
    auto *frame = new MmemuFrame();
    frame->Show(true);
    return true;
}

MmemuFrame::MmemuFrame()
    : wxFrame(nullptr, wxID_ANY, "mmemu - Multi Machine Emulator",
              wxDefaultPosition, wxSize(800, 600))
{
    auto *panel = new wxPanel(this);
    auto *label = new wxStaticText(panel, wxID_ANY,
        "mmemu - Multi Machine Emulator\nVersion 0.1.0-dev",
        wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);

    auto *sizer = new wxBoxSizer(wxVERTICAL);
    sizer->AddStretchSpacer();
    sizer->Add(label, 0, wxALIGN_CENTER);
    sizer->AddStretchSpacer();
    panel->SetSizer(sizer);
}
