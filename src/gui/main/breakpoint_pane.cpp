#include "breakpoint_pane.h"
#include "libdebug/main/breakpoint_list.h"
#include "libdebug/main/expression_evaluator.h"
#include <sstream>
#include <iomanip>

// ---------------------------------------------------------------------------
// Local dialog for adding a breakpoint/watchpoint
// ---------------------------------------------------------------------------
class AddBreakpointDialog : public wxDialog {
public:
    AddBreakpointDialog(wxWindow* parent, DebugContext* dbg)
        : wxDialog(parent, wxID_ANY, "Add Breakpoint",
                   wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE), m_dbg(dbg)
    {
        auto* sizer = new wxBoxSizer(wxVERTICAL);
        auto* form  = new wxFlexGridSizer(2, 2, 6, 10);
        form->AddGrowableCol(1);

        form->Add(new wxStaticText(this, wxID_ANY, "Address:"),
                  0, wxALIGN_CENTER_VERTICAL);
        m_addr = new wxTextCtrl(this, wxID_ANY, "");
        form->Add(m_addr, 1, wxEXPAND);

        form->Add(new wxStaticText(this, wxID_ANY, "Type:"),
                  0, wxALIGN_CENTER_VERTICAL);
        wxArrayString types;
        types.Add("Exec");
        types.Add("Read Watch");
        types.Add("Write Watch");
        m_type = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, types);
        m_type->SetSelection(0);
        form->Add(m_type, 1, wxEXPAND);

        sizer->Add(form, 0, wxEXPAND | wxALL, 10);
        sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 8);
        SetSizerAndFit(sizer);
        if (auto* btn = dynamic_cast<wxButton*>(FindWindowById(wxID_OK, this))) btn->SetDefault();
        m_addr->SetFocus();
        Bind(wxEVT_BUTTON, &AddBreakpointDialog::OnOK, this, wxID_OK);
    }

    uint32_t GetAddress() const { return m_addressVal; }

    BreakpointType GetBreakpointType() const {
        switch (m_type->GetSelection()) {
            case 1:  return BreakpointType::READ_WATCH;
            case 2:  return BreakpointType::WRITE_WATCH;
            default: return BreakpointType::EXEC;
        }
    }

private:
    void OnOK(wxCommandEvent& event) {
        if (ExpressionEvaluator::evaluate(m_addr->GetValue().ToStdString(), m_dbg, m_addressVal)) {
            EndModal(wxID_OK);
        } else {
            wxMessageBox("Invalid address expression!", "Error", wxOK | wxICON_ERROR);
        }
    }

    wxTextCtrl* m_addr;
    wxChoice*   m_type;
    DebugContext* m_dbg;
    uint32_t    m_addressVal = 0;
};

// ---------------------------------------------------------------------------
// BreakpointPane
// ---------------------------------------------------------------------------
BreakpointPane::BreakpointPane(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
{
    auto* sizer = new wxBoxSizer(wxVERTICAL);

    m_list = new wxListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                            wxLC_REPORT | wxLC_SINGLE_SEL | wxBORDER_SUNKEN);
    m_list->InsertColumn(0, "#",       wxLIST_FORMAT_RIGHT,  32);
    m_list->InsertColumn(1, "Type",    wxLIST_FORMAT_LEFT,   72);
    m_list->InsertColumn(2, "Address", wxLIST_FORMAT_LEFT,   72);
    m_list->InsertColumn(3, "En",      wxLIST_FORMAT_CENTER, 36);
    m_list->InsertColumn(4, "Hits",    wxLIST_FORMAT_RIGHT,  48);
    sizer->Add(m_list, 1, wxEXPAND | wxALL, 4);

    auto* btnSizer  = new wxBoxSizer(wxHORIZONTAL);
    auto* btnAdd    = new wxButton(this, wxID_ANY, "Add...");
    m_btnDelete     = new wxButton(this, wxID_ANY, "Delete");
    m_btnEnable     = new wxButton(this, wxID_ANY, "Enable");
    m_btnDisable    = new wxButton(this, wxID_ANY, "Disable");
    btnSizer->Add(btnAdd,        0, wxRIGHT, 4);
    btnSizer->Add(m_btnDelete,   0, wxRIGHT, 4);
    btnSizer->Add(m_btnEnable,   0, wxRIGHT, 4);
    btnSizer->Add(m_btnDisable,  0);
    sizer->Add(btnSizer, 0, wxLEFT | wxBOTTOM, 4);

    SetSizer(sizer);
    UpdateButtonStates();

    btnAdd->Bind(wxEVT_BUTTON, &BreakpointPane::OnAdd,    this);
    m_btnDelete->Bind(wxEVT_BUTTON,  &BreakpointPane::OnDelete,  this);
    m_btnEnable->Bind(wxEVT_BUTTON,  &BreakpointPane::OnEnable,  this);
    m_btnDisable->Bind(wxEVT_BUTTON, &BreakpointPane::OnDisable, this);
    m_list->Bind(wxEVT_LIST_ITEM_SELECTED,   &BreakpointPane::OnItemSelected,   this);
    m_list->Bind(wxEVT_LIST_ITEM_DESELECTED, &BreakpointPane::OnItemDeselected, this);
    m_list->Bind(wxEVT_LIST_ITEM_ACTIVATED,  &BreakpointPane::OnItemActivated,  this);
}

void BreakpointPane::SetDebugContext(DebugContext* dbg) {
    m_dbg = dbg;
    RefreshValues();
}

void BreakpointPane::RefreshValues() {
    // Preserve selection by id
    int selId = SelectedId();

    m_list->DeleteAllItems();
    if (!m_dbg) return;

    const auto& bps = m_dbg->breakpoints().breakpoints();
    for (const auto& bp : bps) {
        long row = m_list->GetItemCount();
        m_list->InsertItem(row, std::to_string(bp.id));

        const char* typeName = "exec";
        if (bp.type == BreakpointType::READ_WATCH)  typeName = "read";
        if (bp.type == BreakpointType::WRITE_WATCH) typeName = "write";
        m_list->SetItem(row, 1, typeName);

        std::ostringstream ss;
        ss << "$" << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << bp.addr;
        m_list->SetItem(row, 2, ss.str());
        m_list->SetItem(row, 3, bp.enabled ? "y" : "n");
        m_list->SetItem(row, 4, std::to_string(bp.hitCount));
        m_list->SetItemData(row, bp.id);

        if (bp.id == selId)
            m_list->SetItemState(row, wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED);
    }
    UpdateButtonStates();
}

void BreakpointPane::OnAdd(wxCommandEvent&) {
    if (!m_dbg) {
        wxMessageBox("No machine loaded.", "Add Breakpoint", wxOK | wxICON_WARNING, this);
        return;
    }
    AddBreakpointDialog dlg(this, m_dbg);
    if (dlg.ShowModal() == wxID_OK)
        m_dbg->breakpoints().add(dlg.GetAddress(), dlg.GetBreakpointType());
    RefreshValues();
}

void BreakpointPane::OnDelete(wxCommandEvent&) {
    int id = SelectedId();
    if (id >= 0 && m_dbg) {
        m_dbg->breakpoints().remove(id);
        RefreshValues();
    }
}

void BreakpointPane::OnEnable(wxCommandEvent&) {
    int id = SelectedId();
    if (id >= 0 && m_dbg) {
        m_dbg->breakpoints().setEnabled(id, true);
        RefreshValues();
    }
}

void BreakpointPane::OnDisable(wxCommandEvent&) {
    int id = SelectedId();
    if (id >= 0 && m_dbg) {
        m_dbg->breakpoints().setEnabled(id, false);
        RefreshValues();
    }
}

void BreakpointPane::OnItemActivated(wxListEvent&) {
    // Double-click toggles enabled
    int id = SelectedId();
    if (id < 0 || !m_dbg) return;
    for (const auto& bp : m_dbg->breakpoints().breakpoints()) {
        if (bp.id == id) {
            m_dbg->breakpoints().setEnabled(id, !bp.enabled);
            RefreshValues();
            return;
        }
    }
}

void BreakpointPane::OnItemSelected(wxListEvent&)   { UpdateButtonStates(); }
void BreakpointPane::OnItemDeselected(wxListEvent&) { UpdateButtonStates(); }

void BreakpointPane::UpdateButtonStates() {
    bool hasSel = m_dbg && m_list->GetSelectedItemCount() > 0;
    m_btnDelete->Enable(hasSel);
    m_btnEnable->Enable(hasSel);
    m_btnDisable->Enable(hasSel);
}

int BreakpointPane::SelectedId() const {
    long item = m_list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
    return item == -1 ? -1 : (int)m_list->GetItemData(item);
}
