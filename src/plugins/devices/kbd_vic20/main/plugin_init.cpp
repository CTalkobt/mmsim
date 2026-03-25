#include "mmemu_plugin_api.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/ikeyboard_device.h"
#include "kbd_vic20.h"

/**
 * Wrapper that allows a keyboard device to be registered as an IOHandler
 * while also exposing the IKeyboardDevice interface.
 */
class KeyboardDeviceWrapper : public IOHandler, public IKeyboardDevice {
public:
    KeyboardDeviceWrapper() { m_kbd = new KbdVic20(); }
    ~KeyboardDeviceWrapper() { delete m_kbd; }

    // IOHandler interface
    std::string name() const override { return "kbd_vic20"; }
    uint32_t baseAddr() const override { return 0; }
    uint32_t addrMask() const override { return 0; }
    bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t) override { return false; }
    void reset() override { m_kbd->clearKeys(); }
    void tick(uint64_t) override {}

    // IKeyboardDevice interface
    void keyDown(int row, int col) override { m_kbd->keyDown(row, col); }
    void keyUp(int row, int col) override { m_kbd->keyUp(row, col); }
    void clearKeys() override { m_kbd->clearKeys(); }
    bool pressKeyByName(const std::string& keyName, bool down) override { 
        return m_kbd->pressKeyByName(keyName, down); 
    }
    IPortDevice* getPort(int index) override { return m_kbd->getPort(index); }

private:
    KbdVic20* m_kbd;
};

static IOHandler* createKeyboard() {
    return new KeyboardDeviceWrapper();
}

static DevicePluginInfo s_devices[] = {
    {"kbd_vic20", createKeyboard}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "mmemu-plugin-kbd-vic20",
    "1.0.0",
    0, nullptr,
    0, nullptr,
    1, s_devices,
    0, nullptr
};

#include "libdevices/main/device_registry.h"

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    if (host->deviceRegistry) DeviceRegistry::setInstance(host->deviceRegistry);
    return &s_manifest;
}
