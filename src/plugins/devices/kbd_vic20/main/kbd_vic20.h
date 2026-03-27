#pragma once

#include "libdevices/main/iport_device.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include <vector>
#include <map>

/**
 * Standard Commodore 8x8 Keyboard Matrix.
 * Wired to two I/O ports.
 */
class KbdVic20 : public IKeyboardMatrix {
public:
    KbdVic20();
    virtual ~KbdVic20() = default;

    // IKeyboardDevice interface
    void keyDown(int row, int col) override;
    void keyUp(int row, int col) override;
    void clearKeys() override;
    bool pressKeyByName(const std::string& keyName, bool down) override;
    IPortDevice* getPort(int index) override {
        if (index == 0) return (IPortDevice*)&m_colPort;
        if (index == 1) return (IPortDevice*)&m_rowPort;
        return nullptr;
    }

    /**
     * Port implementation for the "Output" port (Columns).
     * Wired to VIA2 Port B ($9120) — CPU writes column select here.
     */
    class ColumnPort : public IPortDevice {
    public:
        ColumnPort(KbdVic20* parent) : m_parent(parent) {}
        uint8_t readPort() override { return m_val; }
        void writePort(uint8_t val) override { m_val = val; m_parent->updateMatrix(); }
        void setDdr(uint8_t ddr) override { (void)ddr; }
        uint8_t m_val = 0xFF;
    private:
        KbdVic20* m_parent;
    };

    /**
     * Port implementation for the "Input" port (Rows).
     * Wired to VIA2 Port A ($9121) — CPU reads row sense here.
     */
    class RowPort : public IPortDevice {
    public:
        RowPort(KbdVic20* parent) : m_parent(parent) {}
        uint8_t readPort() override { return m_parent->m_rowVal; }
        void writePort(uint8_t val) override { (void)val; }
        void setDdr(uint8_t ddr) override { (void)ddr; }
    private:
        KbdVic20* m_parent;
    };

    ColumnPort* getColumnPort() { return &m_colPort; }
    RowPort* getRowPort() { return &m_rowPort; }

private:
    void updateMatrix();

    uint8_t m_matrix[8]; // 8 rows, each byte is 8 columns
    uint8_t m_rowVal = 0xFF;
    
    ColumnPort m_colPort;
    RowPort    m_rowPort;

    static std::map<std::string, std::pair<int, int>> s_keyMap;
};
