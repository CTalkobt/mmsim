#include "stack_pane.h"
#include "libdebug/main/stack_trace.h"
#include <sstream>
#include <iomanip>

StackPane::StackPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    m_list->InsertColumn(0, "#",          wxLIST_FORMAT_RIGHT, 32);
    m_list->InsertColumn(1, "Type",       wxLIST_FORMAT_LEFT,  56);
    m_list->InsertColumn(2, "Value",      wxLIST_FORMAT_LEFT,  72);
    m_list->InsertColumn(3, "Pushed By",  wxLIST_FORMAT_LEFT,  80);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, 4);

    auto* bottom = new wxBoxSizer(wxHORIZONTAL);
    m_depthLabel = new wxStaticText(this, wxID_ANY, "Depth: 0");
    auto* btnClear = new wxButton(this, wxID_ANY, "Clear");
    bottom->Add(m_depthLabel, 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    bottom->Add(btnClear, 0, wxRIGHT | wxBOTTOM, 4);
    sizer->Add(bottom, 0, wxEXPAND);

    SetSizer(sizer);

    btnClear->Bind(wxEVT_BUTTON, &StackPane::OnClear, this);
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED, &StackPane::OnItemActivated, this);
}

void StackPane::SetGotoCallback(std::function<void(uint32_t)> cb) {
    m_gotoCallback = std::move(cb);
}

void StackPane::SetDebugContext(DebugContext* dbg) {
    m_dbg = dbg;
    RefreshValues();
}

void StackPane::RefreshValues() {
    m_list->DeleteAllItems();
    if (!m_dbg) return;

    auto& st = m_dbg->stackTrace();
    auto entries = st.recent(0);  // all, most-recent first

    for (int i = 0; i < (int)entries.size(); ++i) {
        const auto& e = entries[i];
        m_list->InsertItem(i, std::to_string(i));
        m_list->SetItem(i, 1, stackPushTypeName(e.type));

        std::ostringstream val;
        val << "$" << std::uppercase << std::hex << std::setfill('0');
        if (e.type == StackPushType::CALL || e.type == StackPushType::BRK)
            val << std::setw(4) << e.value;
        else
            val << std::setw(2) << e.value;
        m_list->SetItem(i, 2, val.str());

        std::ostringstream by;
        by << "$" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << e.pushedByPc;
        m_list->SetItem(i, 3, by.str());
    }

    m_depthLabel->SetLabel(wxString::Format("Depth: %d", st.depth()));
}

void StackPane::OnClear(wxCommandEvent&) {
    if (m_dbg) m_dbg->stackTrace().clear();
    RefreshValues();
}

void StackPane::OnItemActivated(wxListEvent& event) {
    if (!m_gotoCallback || !m_dbg) return;
    int idx = event.GetIndex();
    auto entries = m_dbg->stackTrace().recent(0);
    if (idx < 0 || idx >= (int)entries.size()) return;
    const auto& e = entries[idx];
    // For calls/BRK navigate to the target; for register pushes go to the push site
    if (e.type == StackPushType::CALL || e.type == StackPushType::BRK)
        m_gotoCallback(e.value);
    else
        m_gotoCallback(e.pushedByPc);
}
