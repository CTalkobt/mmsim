#pragma once
#include <wx/wx.h>
#include <wx/tglbtn.h>
#include "libdevices/main/ivideo_output.h"
#include "gui/main/ikeyboard_capture_pane.h"
#include <functional>
#include <vector>

/**
 * Panel that renders an IVideoOutput device (e.g. VIC-I) and scales it to fit.
 * Also implements IKeyboardCapturePane so the host can wire the "Capture
 * Keyboard" toggle button to the frame's key-routing logic.
 */
class ScreenPane : public wxPanel, public IKeyboardCapturePane {
public:
    explicit ScreenPane(wxWindow* parent);

    /** Set (or clear) the video device to render. */
    void SetVideoOutput(IVideoOutput* vid);

    /** Call once per timer tick to pull a fresh frame and queue a repaint. */
    void RefreshFrame();

    // IKeyboardCapturePane
    void SetCaptureCallback(std::function<void(bool)> fn) override { m_captureCallback = std::move(fn); }
    void SetCaptureActive(bool active) override;

private:
    void OnPaint(wxPaintEvent&);
    void OnSize(wxSizeEvent&);
    void OnCaptureToggle(wxCommandEvent&);

    IVideoOutput*              m_video = nullptr;
    std::vector<uint32_t>      m_pixelBuf;
    wxToggleButton*            m_ratioBtn;
    wxToggleButton*            m_captureBtn;
    std::function<void(bool)>  m_captureCallback;
};
