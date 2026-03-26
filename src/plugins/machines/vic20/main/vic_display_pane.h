#pragma once
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include "libdevices/main/ivideo_output.h"
#include <vector>

/**
 * Panel that renders an IVideoOutput device (e.g. VIC-I) and scales it to fit.
 * A "Lock Ratio" toggle preserves the native pixel aspect ratio; when off the
 * image is stretched freely to fill the pane.
 */
class VicDisplayPane : public wxPanel {
public:
    explicit VicDisplayPane(wxWindow* parent);

    /** Set (or clear) the video device to render. */
    void SetVideoOutput(IVideoOutput* vid);

    /** Call once per timer tick to pull a fresh frame and queue a repaint. */
    void RefreshFrame();

private:
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);

    IVideoOutput*         m_video = nullptr;
    std::vector<uint32_t> m_pixelBuf;
    wxToggleButton*       m_ratioBtn;
};
