#pragma once
#include <wx/wx.h>
#include "libdevices/main/io_handler.h"

class TapePane : public wxPanel {
public:
    TapePane(wxWindow* parent);
    void SetTapeDevice(IOHandler* tape);
    void RefreshStatus();

private:
    void OnMount(wxCommandEvent&);
    void OnPlay(wxCommandEvent&);
    void OnStop(wxCommandEvent&);
    void OnRewind(wxCommandEvent&);
    void OnRecord(wxCommandEvent&);
    void OnSaveRecording(wxCommandEvent&);

    IOHandler* m_tape = nullptr;
    wxStaticText* m_statusLabel;
    wxButton* m_mountBtn;
    wxButton* m_playBtn;
    wxButton* m_stopBtn;
    wxButton* m_rewindBtn;
    wxButton* m_recordBtn;
    wxButton* m_saveBtn;
};
