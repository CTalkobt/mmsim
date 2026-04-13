#include "test_harness.h"
#include "virtual_iec.h"
#include <vector>
#include <string>
#include <iostream>

class IECHost {
public:
    IECHost(VirtualIECBus& bus) : m_bus(bus) {}

    void setATN(bool level) { m_atn = level; updateBus(); }
    void setCLK(bool level) { m_clk = level; updateBus(); }
    void setDATA(bool level) { m_data = level; updateBus(); }

    void updateBus() {
        uint8_t val = 0;
        if (m_atn) val |= (1 << 3);
        if (m_clk) val |= (1 << 4);
        if (m_data) val |= (1 << 5);
        m_bus.writePort(val);
    }

    bool getCLK() { return (m_bus.readPort() & 0x40) == 0; } // active low
    bool getDATA() { return (m_bus.readPort() & 0x80) == 0; }

    void waitState(uint64_t cycles) {
        uint64_t step = 100;
        for (uint64_t i = 0; i < cycles; i += step) {
            m_bus.tick(step);
        }
    }

    bool sendByte(uint8_t byte, bool atn) {
        if (atn) {
            if (!m_atn) {
                setATN(true);
                setCLK(false);
                setDATA(false);
                int timeout = 5000;
                while (!getDATA() && timeout--) waitState(100);
                if (timeout <= 0) { std::cout << "sendByte: ATN ACK timeout\n"; return false; }
            }
            setCLK(true);
            waitState(2000);
            if (getDATA()) { std::cout << "sendByte: Device did not release DATA on ATN CLK low\n"; return false; }
        } else {
            // Data mode: wait for listener to release DATA (ready for data)
            setDATA(false);
            int timeout = 10000;
            while (getDATA() && timeout--) waitState(100);
            if (timeout <= 0) { std::cout << "sendByte: LISTENING device not ready\n"; return false; }
        }

        for (int i = 0; i < 8; ++i) {
            bool bit = (byte >> i) & 1;
            setDATA(bit);
            waitState(200);
            setCLK(true);
            waitState(400);
            setCLK(false);
            waitState(200);
        }

        int timeout = 5000;
        while (!getDATA() && timeout--) waitState(100);
        if (timeout <= 0) { std::cout << "sendByte: Byte ACK timeout for " << (int)byte << "\n"; return false; }

        setCLK(true);
        waitState(1000);
        setCLK(false);
        waitState(1000);

        return true;
    }

    bool receiveByte(uint8_t& byte) {
        bool eoi;
        return receiveByte(byte, eoi);
    }

    bool receiveByte(uint8_t& byte, bool& eoi) {
        eoi = false;

        // Step 0: pull DATA (listener present / not ready)
        setDATA(true);
        waitState(100);

        // Wait for talker CLK released (Step 1: ready to send)
        int timeout = 10000;
        while (getCLK() && timeout--) waitState(100);
        if (timeout <= 0) { std::cout << "receiveByte: CLK release timeout\n"; return false; }

        // Step 2: release DATA (ready for data)
        setDATA(false);

        // Wait for talker to pull CLK (start of bit transfer).
        // If CLK not pulled within ~300 cycles, it's EOI.
        int eoiTimer = 0;
        while (!getCLK()) {
            waitState(100);
            eoiTimer += 100;
            if (eoiTimer >= 300 && !eoi) {
                eoi = true;
                // EOI ack: pull DATA for >=60us, then release
                setDATA(true);
                waitState(100);
                setDATA(false);
            }
            if (eoiTimer >= 10000) { std::cout << "receiveByte: CLK pull timeout\n"; return false; }
        }

        // Bit transfer: 8 bits, LSB first. CLK is true on entry.
        byte = 0;
        for (int i = 0; i < 8; ++i) {
            if (i > 0) {
                timeout = 10000;
                while (!getCLK() && timeout--) waitState(100);
                if (timeout <= 0) { std::cout << "receiveByte: Bit " << i << " CLK pull timeout\n"; return false; }
            }

            if (getDATA()) byte |= (1 << i);

            timeout = 10000;
            while (getCLK() && timeout--) waitState(100);
            if (timeout <= 0) { std::cout << "receiveByte: Bit " << i << " CLK release timeout\n"; return false; }
        }

        // Frame handshake: pull DATA (byte accepted)
        setDATA(true);
        waitState(1000);

        return true;
    }

private:
    VirtualIECBus& m_bus;
    bool m_atn = false;
    bool m_clk = false;
    bool m_data = false;
};

TEST_CASE(iec_d64_directory) {
    VirtualIECBus bus(8);
    IECHost host(bus);

    ASSERT(bus.mountDisk(8, "tests/resources/1541-demo.d64"));

    std::cout << "--- Sending LISTEN 8 ---\n";
    ASSERT(host.sendByte(0x28, true));
    std::cout << "--- Sending SECOND 0 ---\n";
    ASSERT(host.sendByte(0x60, true));
    
    std::cout << "--- Releasing ATN ---\n";
    host.setATN(false);
    host.waitState(5000);
    std::cout << "--- Sending filename '$' ---\n";
    ASSERT(host.sendByte('$', false));
    std::cout << "--- Sending UNLISTEN ---\n";
    ASSERT(host.sendByte(0x3F, true)); // UNLISTEN
    host.setATN(false);
    host.waitState(5000);

    std::cout << "--- Sending TALK 8 ---\n";
    ASSERT(host.sendByte(0x48, true));
    std::cout << "--- Sending SECOND 0 ---\n";
    ASSERT(host.sendByte(0x60, true));
    host.setATN(false);
    host.setDATA(true);  // turnaround: host becomes listener, pulls DATA
    host.waitState(1000);

    // 4. Read directory data
    std::vector<uint8_t> data;
    for (int i = 0; i < 200; ++i) {
        uint8_t b;
        bool eoi;
        if (host.receiveByte(b, eoi)) {
            data.push_back(b);
            if (eoi) break;
        } else {
            break;
        }
    }

    ASSERT(data.size() > 0);
    
    bool foundHeader = false;
    for (size_t i = 0; i + 5 < data.size(); ++i) {
        if (data[i] == 'M' && data[i+1] == 'M' && data[i+2] == 'E' && data[i+3] == 'M' && data[i+4] == 'U') {
            foundHeader = true;
            break;
        }
    }
    ASSERT(foundHeader);
}

TEST_CASE(iec_d64_load_file) {
    VirtualIECBus bus(8);
    IECHost host(bus);

    ASSERT(bus.mountDisk(8, "tests/resources/1541-demo.d64"));

    // 1. LISTEN 8, SECOND 0, Send "HOW TO USE", UNLISTEN
    ASSERT(host.sendByte(0x28, true));
    ASSERT(host.sendByte(0x60, true));
    host.setATN(false);
    host.waitState(5000);
    std::string filename = "HOW TO USE";
    for (char c : filename) ASSERT(host.sendByte(c, false));
    ASSERT(host.sendByte(0x3F, true));
    host.setATN(false);
    host.waitState(5000);

    // 2. TALK 8, SECOND 0
    ASSERT(host.sendByte(0x48, true));
    ASSERT(host.sendByte(0x60, true));
    host.setATN(false);
    host.setDATA(true);  // turnaround: host becomes listener
    host.waitState(1000);

    // 3. Read first few bytes of file
    uint8_t b1, b2;
    ASSERT(host.receiveByte(b1));
    ASSERT(host.receiveByte(b2));
    
    ASSERT(b1 == 0x01);
    ASSERT(b2 == 0x04); // $0401 load address (VIC-20 BASIC)
}
