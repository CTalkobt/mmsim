#include "vic_display_pane.h"
#include <wx/dcbuffer.h>
#include <wx/settings.h>
#include <algorithm>

VicDisplayPane::VicDisplayPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
              wxFULL_REPAINT_ON_RESIZE)
{
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    m_ratioBtn   = new wxToggleButton(this, wxID_ANY, "Lock Ratio");
    m_captureBtn = new wxToggleButton(this, wxID_ANY, "Capture Keyboard");
    m_ratioBtn->SetValue(true);

    auto* toolbar = new wxBoxSizer(wxHORIZONTAL);
    toolbar->Add(m_ratioBtn,   0, wxALL, 4);
    toolbar->AddStretchSpacer();
    toolbar->Add(m_captureBtn, 0, wxALL, 4);

    auto* sizer = new wxBoxSizer(wxVERTICAL);
    sizer->Add(toolbar, 0, wxEXPAND);
    SetSizer(sizer);

    Bind(wxEVT_PAINT, &VicDisplayPane::OnPaint,         this);
    Bind(wxEVT_SIZE,  &VicDisplayPane::OnSize,           this);
    m_ratioBtn->Bind(wxEVT_TOGGLEBUTTON,   [this](wxCommandEvent&) { Refresh(); });
    m_captureBtn->Bind(wxEVT_TOGGLEBUTTON, &VicDisplayPane::OnCaptureToggle, this);
}

void VicDisplayPane::OnCaptureToggle(wxCommandEvent&) {
    bool active = m_captureBtn->GetValue();
    m_captureBtn->SetLabel(active ? "Keyboard Captured \u25cf" : "Capture Keyboard");
    if (m_captureCallback) m_captureCallback(active);
}

void VicDisplayPane::SetCaptureActive(bool active) {
    m_captureBtn->SetValue(active);
    m_captureBtn->SetLabel(active ? "Keyboard Captured \u25cf" : "Capture Keyboard");
}

void VicDisplayPane::SetVideoOutput(IVideoOutput* vid) {
    m_video = vid;
    if (m_video) {
        auto dim = m_video->getDimensions();
        m_pixelBuf.assign((size_t)dim.width * dim.height, 0u);
    } else {
        m_pixelBuf.clear();
    }
    Refresh();
}

void VicDisplayPane::RefreshFrame() {
    if (m_video) Refresh();
}

void VicDisplayPane::OnSize(wxSizeEvent& event) {
    Refresh();
    event.Skip();
}

void VicDisplayPane::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);

    wxColour bgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
    wxColour fgCol = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);
    dc.SetBackground(wxBrush(bgCol));
    dc.Clear();

    if (!m_video || m_pixelBuf.empty()) {
        dc.SetTextForeground(fgCol);
        dc.DrawText("No video output", 10, 40);
        return;
    }

    m_video->renderFrame(m_pixelBuf.data());

    auto dim = m_video->getDimensions();
    int srcW = dim.width;
    int srcH = dim.height;

    auto* rgb = new unsigned char[srcW * srcH * 3];
    for (int i = 0; i < srcW * srcH; ++i) {
        uint32_t px  = m_pixelBuf[i];
        rgb[i*3]     = (px >> 16) & 0xFF;  // R
        rgb[i*3 + 1] = (px >>  8) & 0xFF;  // G
        rgb[i*3 + 2] =  px        & 0xFF;  // B
    }
    wxImage img(srcW, srcH, rgb);

    wxRect canvas = GetClientRect();
    int btnH = m_ratioBtn->GetSize().y + 8;
    canvas.y      += btnH;
    canvas.height -= btnH;
    if (canvas.width <= 0 || canvas.height <= 0) return;

    int dstX = canvas.x;
    int dstY = canvas.y;
    int dstW = canvas.width;
    int dstH = canvas.height;

    if (m_ratioBtn->GetValue()) {
        float scale = std::min((float)dstW / srcW, (float)dstH / srcH);
        int fitW = (int)(srcW * scale);
        int fitH = (int)(srcH * scale);
        dstX += (dstW - fitW) / 2;
        dstY += (dstH - fitH) / 2;
        dstW = fitW;
        dstH = fitH;
    }

    wxBitmap bmp(img.Scale(dstW, dstH, wxIMAGE_QUALITY_NEAREST));
    dc.DrawBitmap(bmp, dstX, dstY, false);
}
