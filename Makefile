# mmemu — Multi Machine Emulator
# Top-level Makefile
.PHONY: all cli gui mcp libs test plugins clean

all: cli gui mcp plugins

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -Wno-unused-parameter -O2 -fPIC -fvisibility=default -MMD -MP
INCLUDES  = -Isrc -Isrc/include -Isrc/cli/main -Isrc/gui/main -Isrc/libcore/main \
            -Isrc/libdebug/main -Isrc/libdevices/main -Isrc/libmem/main \
            -Isrc/libtoolchain/main -Isrc/mcp/main -Isrc/plugin_loader/main \
            -Isrc/plugins/6502/main -Isrc/plugins/devices/via6522/main \
            -Isrc/plugins/devices/vic6560/main -Isrc/plugins/machines/vic20/main \
            -Isrc/plugins/devices/kbd_vic20/main -Isrc/plugins/viceImporter/main \
            -Isrc/plugins/devices/c64_pla/main -Isrc/plugins/devices/cia6526/main \
            -Isrc/plugins/devices/vic2/main -Isrc/plugins/devices/sid6581/main \
            -Isrc/plugins/machines/c64/main -Isrc/plugins/devices/pia6520/main \
            -Isrc/plugins/devices/crtc6545/main -Isrc/plugins/devices/pet_video/main \
            -Isrc/plugins/devices/pokey/main \
            -Isrc/plugins/machines/pet/main -Itests

BINDIR   = bin
LIBDIR   = lib
ILIBDIR  = lib/internal

# wxWidgets configuration
WXCXXFLAGS = $(shell wx-config --cxxflags)
WXLIBS     = $(shell wx-config --libs aui,xrc,html,qa,core,xml,net,base)

# Plugins must NOT link spdlog/fmt themselves; they use host API function pointers.
# The host binary (BASE_LIBS) exports spdlog symbols; dlopen resolves them at load time.
PLUGIN_LIBS =

# Library Sources
LIBMEM_SRCS       = src/libmem/main/ibus.cpp src/libmem/main/memory_bus.cpp src/libmem/main/libmem.cpp
LIBCORE_SRCS      = src/libcore/main/icore.cpp src/libcore/main/rom_loader.cpp src/libcore/main/core_registry.cpp \
                    src/libcore/main/machines/machine_registry.cpp src/libcore/main/libcore.cpp \
                    src/libcore/main/image_loader.cpp
LIBDEVICES_SRCS   = src/libdevices/main/libdevices.cpp src/libdevices/main/io_registry.cpp \
                    src/libdevices/main/device_registry.cpp src/libdevices/main/joystick.cpp \
                    src/libdevices/main/ieee488.cpp
LIBTOOLCHAIN_SRCS = src/libtoolchain/main/symbol_table.cpp src/libtoolchain/main/source_map.cpp \
                    src/libtoolchain/main/toolchain_registry.cpp src/libtoolchain/main/libtoolchain.cpp
LIBDEBUG_SRCS     = src/libdebug/main/breakpoint_list.cpp src/libdebug/main/debug_context.cpp \
                    src/libdebug/main/trace_buffer.cpp src/libdebug/main/libdebug.cpp
LIBPLUGINS_SRCS   = src/plugin_loader/main/plugin_loader.cpp src/plugin_loader/main/logging.cpp

# Plugin Sources
PLUGIN_6502_SRCS = src/plugins/6502/main/cpu6502.cpp \
                   src/plugins/6502/main/cpu6510.cpp \
                   src/plugins/6502/main/disassembler_6502.cpp \
                   src/plugins/6502/main/assembler_6502.cpp \
                   src/plugins/6502/main/kickassembler.cpp \
                   src/plugins/6502/main/plugin_init.cpp

PLUGIN_VIA6522_SRCS = src/plugins/devices/via6522/main/via6522.cpp \
                      src/plugins/devices/via6522/main/plugin_init.cpp

PLUGIN_VIC6560_SRCS = src/plugins/devices/vic6560/main/vic6560.cpp \
                      src/plugins/devices/vic6560/main/plugin_init.cpp

PLUGIN_KBDVIC20_SRCS = src/plugins/devices/kbd_vic20/main/kbd_vic20.cpp \
                       src/plugins/devices/kbd_vic20/main/plugin_init.cpp

# Machine plugins: Split into Core vs GUI parts
PLUGIN_VIC20_CORE_SRCS = src/plugins/machines/vic20/main/machine_vic20.cpp
PLUGIN_VIC20_GUI_SRCS  = src/plugins/machines/vic20/main/plugin_init.cpp

PLUGIN_C64_CORE_SRCS = src/plugins/machines/c64/main/machine_c64.cpp
PLUGIN_C64_GUI_SRCS  = src/plugins/machines/c64/main/plugin_init.cpp

PLUGIN_VICEIMPORTER_SRCS = src/plugins/viceImporter/main/plugin_main.cpp \
                            src/plugins/viceImporter/main/rom_discovery.cpp \
                            src/plugins/viceImporter/main/rom_importer.cpp \
                            src/plugins/viceImporter/main/rom_import_pane.cpp

PLUGIN_C64PLA_SRCS = src/plugins/devices/c64_pla/main/c64_pla.cpp \
                     src/plugins/devices/c64_pla/main/plugin_init.cpp

PLUGIN_CIA6526_SRCS = src/plugins/devices/cia6526/main/cia6526.cpp \
                      src/plugins/devices/cia6526/main/plugin_init.cpp

PLUGIN_VIC2_SRCS = src/plugins/devices/vic2/main/vic2.cpp \
                   src/plugins/devices/vic2/main/plugin_init.cpp

PLUGIN_SID6581_SRCS = src/plugins/devices/sid6581/main/sid6581.cpp \
                      src/plugins/devices/sid6581/main/plugin_init.cpp

PLUGIN_PIA6520_SRCS = src/plugins/devices/pia6520/main/pia6520.cpp \
                      src/plugins/devices/pia6520/main/plugin_init.cpp

PLUGIN_CBMLOADER_SRCS = src/plugins/cbm-loader/main/prg_loader.cpp \
                        src/plugins/cbm-loader/main/crt_parser.cpp \
                        src/plugins/cbm-loader/main/cbm_cart_handler.cpp \
                        src/plugins/cbm-loader/main/plugin_init.cpp

PLUGIN_CRTC6545_SRCS = src/plugins/devices/crtc6545/main/crtc6545.cpp \
                       src/plugins/devices/crtc6545/main/plugin_init.cpp

PLUGIN_PETVIDEO_SRCS = src/plugins/devices/pet_video/main/pet_video.cpp \
                       src/plugins/devices/pet_video/main/plugin_init.cpp

PLUGIN_PET_SRCS = src/plugins/machines/pet/main/machine_pet.cpp \
                  src/plugins/machines/pet/main/plugin_init.cpp \
                  src/plugins/devices/keyboard/main/keyboard_matrix_pet.cpp

GUI_SRCS = src/gui/main/main.cpp \
           src/gui/main/machine_selector.cpp \
           src/gui/main/register_pane.cpp \
           src/gui/main/memory_pane.cpp \
           src/gui/main/disasm_pane.cpp \
           src/gui/main/console_pane.cpp \
           src/gui/main/cartridge_pane.cpp \
           src/gui/main/screen_pane.cpp \
           src/gui/main/dialogs/memory_dialogs.cpp \
           src/gui/main/dialogs/assemble_dialog.cpp \
           src/gui/main/dialogs/image_dialogs.cpp \
           src/cli/main/cli_interpreter.cpp \
           src/gui/main/plugin_pane_manager.cpp \
           src/cli/main/plugin_command_registry.cpp \
           src/gui/main/audio_output.cpp

CLI_SRCS = src/cli/main/main.cpp \
           src/cli/main/cli_interpreter.cpp \
           src/cli/main/plugin_command_registry.cpp

MCP_SRCS = src/mcp/main/main.cpp \
           src/mcp/main/plugin_tool_registry.cpp

# Test Sources
TEST_SRCS = tests/test_main.cpp \
            tests/test_plugin_extension.cpp \
            tests/test_vice_importer.cpp \
            src/libmem/test/test_flatmembus.cpp \
            src/libcore/test/test_libcore.cpp \
            src/libcore/test/test_registry.cpp \
            src/libdevices/test/test_devices.cpp \
            src/libdevices/test/test_joystick.cpp \
            src/libdebug/test/test_debug.cpp \
            src/libtoolchain/test/test_toolchain.cpp \
            src/plugins/6502/test/test_cpu6502.cpp \
            src/plugins/6502/test/test_disasm6502.cpp \
            src/plugins/6502/test/test_assembler6502.cpp \
            src/plugins/machines/vic20/test/test_vic20_integration.cpp \
            src/plugins/machines/c64/test/test_c64_integration.cpp \
            src/plugins/machines/pet/test/test_pet_integration.cpp \
            src/plugins/cbm-loader/test/test_cbm_loader.cpp \
            src/plugins/devices/pet_video/test/test_pet_video.cpp \
            src/plugins/devices/pia6520/test/test_pia6520.cpp \
            src/plugins/devices/via6522/test/test_via6522.cpp \
            src/plugins/devices/antic/test/test_antic.cpp \
            src/plugins/devices/gtia/test/test_gtia.cpp \
            src/plugins/devices/pokey/test/test_pokey.cpp

            # Test-related objects (excluding plugin entry points to avoid multiple mmemuPluginInit definitions)
            ALL_PLUGIN_OBJS = src/plugins/6502/main/cpu6502.o \
                  src/plugins/6502/main/cpu6510.o \
                  src/plugins/6502/main/disassembler_6502.o \
                  src/plugins/6502/main/assembler_6502.o \
                  src/plugins/6502/main/kickassembler.o \
                  src/plugins/devices/via6522/main/via6522.o \
                  src/plugins/devices/vic6560/main/vic6560.o \
                  src/plugins/devices/kbd_vic20/main/kbd_vic20.o \
                  src/plugins/machines/vic20/main/machine_vic20.o \
                  src/plugins/viceImporter/main/rom_discovery.o \
                  src/plugins/viceImporter/main/rom_importer.o \
                  src/plugins/devices/c64_pla/main/c64_pla.o \
                  src/plugins/devices/cia6526/main/cia6526.o \
                  src/plugins/devices/vic2/main/vic2.o \
                  src/plugins/devices/sid6581/main/sid6581.o \
                  src/plugins/machines/c64/main/machine_c64.o \
                  src/plugins/devices/pia6520/main/pia6520.o \
                  src/plugins/cbm-loader/main/prg_loader.o \
                  src/plugins/cbm-loader/main/crt_parser.o \
                  src/plugins/cbm-loader/main/cbm_cart_handler.o \
                  src/plugins/devices/crtc6545/main/crtc6545.o \
                  src/plugins/devices/pet_video/main/pet_video.o \
                  src/plugins/machines/pet/main/machine_pet.o \
                  src/plugins/devices/keyboard/main/keyboard_matrix_pet.o \
                  src/plugins/devices/antic/main/antic.o \
                  src/plugins/devices/gtia/main/gtia.o \
                  src/plugins/devices/pokey/main/pokey.o
REGISTRY_OBJS = src/cli/main/plugin_command_registry.o \
                 src/mcp/main/plugin_tool_registry.o \
                 src/gui/main/plugin_pane_manager.o

TEST_OBJS = $(TEST_SRCS:.cpp=.o) $(sort $(ALL_PLUGIN_OBJS)) $(REGISTRY_OBJS)
TEST_BIN  = $(BINDIR)/mmemu-test

# Objects
LIBMEM_OBJS       = $(LIBMEM_SRCS:.cpp=.o)
LIBCORE_OBJS      = $(LIBCORE_SRCS:.cpp=.o)
LIBDEVICES_OBJS   = $(LIBDEVICES_SRCS:.cpp=.o)
LIBTOOLCHAIN_OBJS = $(LIBTOOLCHAIN_SRCS:.cpp=.o)
LIBDEBUG_OBJS     = $(LIBDEBUG_SRCS:.cpp=.o)
LIBPLUGINS_OBJS   = $(LIBPLUGINS_SRCS:.cpp=.o)

PLUGIN_6502_OBJS  = $(PLUGIN_6502_SRCS:.cpp=.o)
PLUGIN_VIA6522_OBJS = $(PLUGIN_VIA6522_SRCS:.cpp=.o)
PLUGIN_VIC6560_OBJS = $(PLUGIN_VIC6560_SRCS:.cpp=.o)
PLUGIN_KBDVIC20_OBJS = $(PLUGIN_KBDVIC20_SRCS:.cpp=.o)
PLUGIN_VIC20_CORE_OBJS = $(PLUGIN_VIC20_CORE_SRCS:.cpp=.o) \
                         src/plugins/devices/via6522/main/via6522.o \
                         src/plugins/devices/vic6560/main/vic6560.o
PLUGIN_VIC20_GUI_OBJS  = src/plugins/machines/vic20/main/plugin_init.o
PLUGIN_VICEIMPORTER_OBJS = $(PLUGIN_VICEIMPORTER_SRCS:.cpp=.o)
PLUGIN_C64PLA_OBJS   = $(PLUGIN_C64PLA_SRCS:.cpp=.o)
PLUGIN_CIA6526_OBJS  = $(PLUGIN_CIA6526_SRCS:.cpp=.o)
PLUGIN_VIC2_OBJS     = $(PLUGIN_VIC2_SRCS:.cpp=.o)
PLUGIN_SID6581_OBJS  = $(PLUGIN_SID6581_SRCS:.cpp=.o)
PLUGIN_C64_CORE_OBJS = $(PLUGIN_C64_CORE_SRCS:.cpp=.o) \
                       src/plugins/devices/cia6526/main/cia6526.o \
                       src/plugins/devices/vic2/main/vic2.o \
                       src/plugins/devices/sid6581/main/sid6581.o \
                       src/plugins/devices/c64_pla/main/c64_pla.o
PLUGIN_C64_GUI_OBJS  = src/plugins/machines/c64/main/plugin_init.o
PLUGIN_PIA6520_OBJS  = $(PLUGIN_PIA6520_SRCS:.cpp=.o)
PLUGIN_CBMLOADER_OBJS = $(PLUGIN_CBMLOADER_SRCS:.cpp=.o)
PLUGIN_CRTC6545_OBJS = $(PLUGIN_CRTC6545_SRCS:.cpp=.o)
PLUGIN_PETVIDEO_OBJS = $(PLUGIN_PETVIDEO_SRCS:.cpp=.o)
PLUGIN_PET_OBJS      = $(PLUGIN_PET_SRCS:.cpp=.o) \
                       src/plugins/devices/pia6520/main/pia6520.o \
                       src/plugins/devices/via6522/main/via6522.o \
                       src/plugins/devices/crtc6545/main/crtc6545.o \
                       src/plugins/devices/pet_video/main/pet_video.o

PLUGIN_ANTIC_SRCS = src/plugins/devices/antic/main/antic.cpp \
                    src/plugins/devices/antic/main/plugin_init.cpp
PLUGIN_ANTIC_OBJS = $(PLUGIN_ANTIC_SRCS:.cpp=.o)

PLUGIN_GTIA_SRCS = src/plugins/devices/gtia/main/gtia.cpp \
                   src/plugins/devices/gtia/main/plugin_init.cpp
PLUGIN_GTIA_OBJS = $(PLUGIN_GTIA_SRCS:.cpp=.o)

PLUGIN_POKEY_SRCS = src/plugins/devices/pokey/main/pokey.cpp \
                    src/plugins/devices/pokey/main/plugin_init.cpp
PLUGIN_POKEY_OBJS = $(PLUGIN_POKEY_SRCS:.cpp=.o)

GUI_OBJS = $(GUI_SRCS:.cpp=.o)

CLI_OBJS = $(CLI_SRCS:.cpp=.o)
MCP_OBJS = $(MCP_SRCS:.cpp=.o)

# Binaries
CLI_BIN = $(BINDIR)/mmemu-cli
GUI_BIN = $(BINDIR)/mmemu-gui
MCP_BIN = $(BINDIR)/mmemu-mcp

PLUGINS = $(LIBDIR)/mmemu-plugin-6502.so \
          $(LIBDIR)/mmemu-plugin-via6522.so \
          $(LIBDIR)/mmemu-plugin-vic6560.so \
          $(LIBDIR)/mmemu-plugin-vic20.so \
          $(LIBDIR)/mmemu-plugin-kbd-vic20.so \
          $(LIBDIR)/mmemu-plugin-vice-importer.so \
          $(LIBDIR)/mmemu-plugin-c64-pla.so \
          $(LIBDIR)/mmemu-plugin-cia6526.so \
          $(LIBDIR)/mmemu-plugin-vic2.so \
          $(LIBDIR)/mmemu-plugin-sid6581.so \
          $(LIBDIR)/mmemu-plugin-c64.so \
          $(LIBDIR)/mmemu-plugin-pia6520.so \
          $(LIBDIR)/mmemu-plugin-cbm-loader.so \
          $(LIBDIR)/mmemu-plugin-crtc6545.so \
          $(LIBDIR)/mmemu-plugin-pet-video.so \
          $(LIBDIR)/mmemu-plugin-pet.so \
          $(LIBDIR)/mmemu-plugin-antic.so \
          $(LIBDIR)/mmemu-plugin-gtia.so \
          $(LIBDIR)/mmemu-plugin-pokey.so

LIBS = $(ILIBDIR)/libmem.a $(ILIBDIR)/libcore.a $(ILIBDIR)/libdevices.a \
       $(ILIBDIR)/libtoolchain.a $(ILIBDIR)/libdebug.a $(ILIBDIR)/libplugins.a

BASE_LIBS = -Wl,--whole-archive -L$(ILIBDIR) -lplugins -ldebug -ltoolchain -ldevices -lcore -lmem -Wl,--no-whole-archive -ldl -lspdlog -lfmt

# ---------------------------------------------------------------------------
# Build rules
# ---------------------------------------------------------------------------

cli: $(CLI_BIN) plugins
gui: $(GUI_BIN) plugins
mcp: $(MCP_BIN) plugins
libs: $(LIBS)
plugins: $(PLUGINS)

$(BINDIR) $(LIBDIR) $(ILIBDIR):
	mkdir -p $@

# Internal Libraries
$(ILIBDIR)/libmem.a: $(LIBMEM_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

$(ILIBDIR)/libcore.a: $(LIBCORE_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

$(ILIBDIR)/libdevices.a: $(LIBDEVICES_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

$(ILIBDIR)/libtoolchain.a: $(LIBTOOLCHAIN_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

$(ILIBDIR)/libdebug.a: $(LIBDEBUG_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

$(ILIBDIR)/libplugins.a: $(LIBPLUGINS_OBJS) | $(ILIBDIR)
	$(AR) rcs $@ $^

# Plugin rules
$(LIBDIR)/mmemu-plugin-6502.so: $(PLUGIN_6502_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-via6522.so: $(PLUGIN_VIA6522_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-vic6560.so: $(PLUGIN_VIC6560_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-vic20.so: $(PLUGIN_VIC20_CORE_OBJS) $(PLUGIN_VIC20_GUI_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-kbd-vic20.so: $(PLUGIN_KBDVIC20_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-vice-importer.so: $(PLUGIN_VICEIMPORTER_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-c64-pla.so: $(PLUGIN_C64PLA_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-cia6526.so: $(PLUGIN_CIA6526_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-vic2.so: $(PLUGIN_VIC2_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-sid6581.so: $(PLUGIN_SID6581_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-c64.so: $(PLUGIN_C64_CORE_OBJS) $(PLUGIN_C64_GUI_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-pia6520.so: $(PLUGIN_PIA6520_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-cbm-loader.so: $(PLUGIN_CBMLOADER_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-crtc6545.so: $(PLUGIN_CRTC6545_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-pet-video.so: $(PLUGIN_PETVIDEO_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-pet.so: $(PLUGIN_PET_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-antic.so: $(PLUGIN_ANTIC_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-gtia.so: $(PLUGIN_GTIA_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

$(LIBDIR)/mmemu-plugin-pokey.so: $(PLUGIN_POKEY_OBJS) | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $^ $(WXLIBS) $(PLUGIN_LIBS)

# Binary rules
$(CLI_BIN): $(CLI_OBJS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -rdynamic -o $@ $(CLI_OBJS) $(BASE_LIBS)

$(GUI_BIN): $(GUI_OBJS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(WXCXXFLAGS) -rdynamic -o $@ $(GUI_OBJS) $(BASE_LIBS) $(WXLIBS) -lasound

$(MCP_BIN): $(MCP_OBJS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -rdynamic -o $@ $(MCP_OBJS) $(BASE_LIBS)

$(TEST_BIN): $(TEST_OBJS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests -rdynamic -o $@ $(TEST_OBJS) $(BASE_LIBS) $(WXLIBS) -lasound

# Generic rules
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(WXCXXFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

test: $(TEST_BIN) plugins
	./$(TEST_BIN)

clean:
	rm -rf $(BINDIR) $(LIBDIR) $(ILIBDIR)
	find src -name "*.o" -delete
	find src -name "*.d" -delete

# Auto-generated header dependency files (produced by -MMD -MP).
# The leading dash suppresses errors when .d files don't exist yet.
-include $(shell find src -name '*.d' 2>/dev/null)
