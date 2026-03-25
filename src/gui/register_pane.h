#pragma once
#include <wx/wx.h>
#include <wx/grid.h>
#include "libcore/icore.h"
#include <vector>
#include <map>

class RegisterPane : public wxPanel {
public:
    RegisterPane(wxWindow* parent);
    void SetCPU(ICore* cpu);
    void RefreshValues();

private:
    ICore* m_cpu = nullptr;
    wxGrid* m_grid;
    std::vector<uint32_t> m_prevValues;
    wxFont m_fixedFont;
};
