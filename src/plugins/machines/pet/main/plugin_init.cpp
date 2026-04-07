#include "mmemu_plugin_api.h"
#include "libcore/main/json_machine_loader.h"
#include "libdevices/main/io_handler.h"
#include "libdevices/main/ikeyboard_matrix.h"
#include "libdevices/main/iport_device.h"
#include "keyboard_matrix_pet.h"

class KbdPetWrapper : public IOHandler, public IKeyboardMatrix {
public:
    explicit KbdPetWrapper(PetKeyboardMatrix::Layout layout)
        : m_layout(layout), m_kbd(layout), m_colPort(&m_kbd), m_rowPort(&m_kbd) {}

    // IOHandler
    const char* name()     const override { return "kbd_pet"; }
    uint32_t    baseAddr() const override { return 0; }
    uint32_t    addrMask() const override { return 0; }
    bool ioRead(IBus*, uint32_t, uint8_t*) override { return false; }
    bool ioWrite(IBus*, uint32_t, uint8_t)  override { return false; }
    void reset() override { m_kbd = PetKeyboardMatrix(m_layout); }
    void tick(uint64_t) override {}

    // IKeyboardMatrix
    void keyDown(int /*row*/, int /*col*/) override {}
    void keyUp(int /*row*/, int /*col*/)   override {}
    void clearKeys() override { m_kbd = PetKeyboardMatrix(m_layout); }
    bool pressKeyByName(const std::string& name, bool down) override {
        if (down) m_kbd.keyDown(name);
        else      m_kbd.keyUp(name);
        return true;
    }
    IPortDevice* getPort(int index) override {
        if (index == 0) return &m_colPort; // column: CPU reads
        if (index == 1) return &m_rowPort; // row:    CPU writes
        return nullptr;
    }

private:
    // Column port: CPU reads column bits from PIA1 Port B
    class ColumnPort : public IPortDevice {
    public:
        explicit ColumnPort(PetKeyboardMatrix* kbd) : m_kbd(kbd) {}
        uint8_t readPort() override       { return m_kbd->readPort(); }
        void    writePort(uint8_t) override {}
        void    setDdr(uint8_t)    override {}
    private:
        PetKeyboardMatrix* m_kbd;
    };

    // Row port: CPU writes row select to PIA1 Port A
    class RowPort : public IPortDevice {
    public:
        explicit RowPort(PetKeyboardMatrix* kbd) : m_kbd(kbd) {}
        uint8_t readPort() override            { return 0xFF; }
        void    writePort(uint8_t val) override { m_kbd->writePort(val); }
        void    setDdr(uint8_t)        override {}
    private:
        PetKeyboardMatrix* m_kbd;
    };

    PetKeyboardMatrix::Layout m_layout;
    PetKeyboardMatrix m_kbd;
    ColumnPort        m_colPort;
    RowPort           m_rowPort;
};

static IOHandler* createKbdPetGraphics() {
    return new KbdPetWrapper(PetKeyboardMatrix::Layout::GRAPHICS);
}
static IOHandler* createKbdPetBusiness() {
    return new KbdPetWrapper(PetKeyboardMatrix::Layout::BUSINESS);
}

static const SimPluginHostAPI* g_host = nullptr;

static const char* s_deps[] = { "6502", "pia6520", "via6522", "crtc6545", "pet-video", "cbm-loader", nullptr };

static DevicePluginInfo s_devices[] = {
    {"kbd_pet_graphics", createKbdPetGraphics},
    {"kbd_pet_business", createKbdPetBusiness}
};

static SimPluginManifest s_manifest = {
    MMEMU_PLUGIN_API_VERSION,
    "pet",
    "Commodore PET Series",
    "1.0.0",
    s_deps,
    nullptr,
    0, nullptr,
    0, nullptr,
    2, s_devices,
    0, nullptr,
    0, nullptr,
    0, nullptr
};

extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host) {
    g_host = host;
    JsonMachineLoader loader;
    loader.loadFile("machines/pet.json");
    return &s_manifest;
}
