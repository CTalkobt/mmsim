# mmemu — Multi Machine Emulator
# Top-level Makefile

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2 -fPIC
INCLUDES  = -Isrc -Isrc/include \
            -Isrc/cli/main -Isrc/gui/main -Isrc/libcore/main -Isrc/libdebug/main \
            -Isrc/libdevices/main -Isrc/libmem/main -Isrc/libtoolchain/main \
            -Isrc/mcp/main -Isrc/plugin_loader/main -Isrc/plugins/6502/main
AR        = ar
ARFLAGS   = rcs

WXCONFIG  ?= wx-config
WXCXXFLAGS := $(shell $(WXCONFIG) --cxxflags 2>/dev/null)
WXLIBS     := $(shell $(WXCONFIG) --libs    2>/dev/null)

BINDIR = bin
LIBDIR = lib

# Libraries
LIBS = $(LIBDIR)/libmem.a $(LIBDIR)/libcore.a $(LIBDIR)/libdevices.a \
       $(LIBDIR)/libtoolchain.a $(LIBDIR)/libdebug.a $(LIBDIR)/libplugins.a

PLUGINS = $(LIBDIR)/mmemu-plugin-6502.so

# Binaries
CLI_BIN = $(BINDIR)/mmemu-cli
GUI_BIN = $(BINDIR)/mmemu-gui
MCP_BIN = $(BINDIR)/mmemu-mcp
TEST_BIN = $(BINDIR)/mmemu-tests

# Sources
CLI_SRCS = src/cli/main/main.cpp src/cli/main/cli_interpreter.cpp
GUI_SRCS = src/gui/main/main.cpp src/gui/main/machine_selector.cpp src/gui/main/register_pane.cpp \
           src/gui/main/memory_pane.cpp src/gui/main/disasm_pane.cpp src/gui/main/console_pane.cpp \
           src/gui/main/dialogs/memory_dialogs.cpp src/gui/main/dialogs/assemble_dialog.cpp \
           src/cli/main/cli_interpreter.cpp
MCP_SRCS = src/mcp/main/main.cpp
TEST_SRCS = tests/test_main.cpp src/libcore/test/test_libcore.cpp \
            src/libmem/test/test_flatmembus.cpp src/libdebug/test/test_debug.cpp \
            src/plugins/6502/test/test_cpu6502.cpp src/plugins/6502/test/test_disasm6502.cpp \
            src/plugins/6502/test/test_assembler6502.cpp src/libtoolchain/test/test_toolchain.cpp \
            src/libcore/test/test_registry.cpp src/libdevices/test/test_devices.cpp

# Library Sources
LIBMEM_SRCS       = src/libmem/main/ibus.cpp src/libmem/main/memory_bus.cpp src/libmem/main/libmem.cpp
LIBCORE_SRCS      = src/libcore/main/icore.cpp src/libcore/main/rom_loader.cpp src/libcore/main/core_registry.cpp \
                    src/libcore/main/machines/machine_registry.cpp src/libcore/main/libcore.cpp
LIBDEVICES_SRCS   = src/libdevices/main/libdevices.cpp src/libdevices/main/io_registry.cpp
LIBTOOLCHAIN_SRCS = src/libtoolchain/main/symbol_table.cpp src/libtoolchain/main/source_map.cpp \
                    src/libtoolchain/main/toolchain_registry.cpp src/libtoolchain/main/libtoolchain.cpp
LIBDEBUG_SRCS     = src/libdebug/main/breakpoint_list.cpp src/libdebug/main/debug_context.cpp \
                    src/libdebug/main/trace_buffer.cpp src/libdebug/main/libdebug.cpp
LIBPLUGINS_SRCS   = src/plugin_loader/main/plugin_loader.cpp

# Plugin 6502 Sources
PLUGIN_6502_SRCS  = src/plugins/6502/main/cpu6502.cpp src/plugins/6502/main/disassembler_6502.cpp \
                    src/plugins/6502/main/assembler_6502.cpp src/plugins/6502/main/kickassembler.cpp \
                    src/plugins/6502/main/plugin_init.cpp

# Objects
LIBMEM_OBJS       = $(LIBMEM_SRCS:.cpp=.o)
LIBCORE_OBJS      = $(LIBCORE_SRCS:.cpp=.o)
LIBDEVICES_OBJS   = $(LIBDEVICES_SRCS:.cpp=.o)
LIBTOOLCHAIN_OBJS = $(LIBTOOLCHAIN_SRCS:.cpp=.o)
LIBDEBUG_OBJS     = $(LIBDEBUG_SRCS:.cpp=.o)
LIBPLUGINS_OBJS   = $(LIBPLUGINS_SRCS:.cpp=.o)
PLUGIN_6502_OBJS  = $(PLUGIN_6502_SRCS:.cpp=.o)

ALL_LIB_OBJS = $(LIBMEM_OBJS) $(LIBCORE_OBJS) $(LIBDEVICES_OBJS) \
               $(LIBTOOLCHAIN_OBJS) $(LIBDEBUG_OBJS) $(LIBPLUGINS_OBJS)

# ---------------------------------------------------------------------------
# Ensure clean and build targets run sequentially even with -j
# ---------------------------------------------------------------------------
ifneq ($(filter clean,$(MAKECMDGOALS)),)
  all cli gui mcp libs plugins test: clean
endif

# ---------------------------------------------------------------------------
# Phony targets
# ---------------------------------------------------------------------------

.PHONY: all cli gui mcp libs test plugins clean

all: cli gui mcp plugins

cli: $(CLI_BIN)

gui: $(GUI_BIN)

mcp: $(MCP_BIN)

libs: $(LIBS)

plugins: $(PLUGINS)

# ---------------------------------------------------------------------------
# Directory rules
# ---------------------------------------------------------------------------

$(BINDIR) $(LIBDIR):
	mkdir -p $@

# ---------------------------------------------------------------------------
# Library rules
# ---------------------------------------------------------------------------

$(LIBDIR)/libmem.a: $(LIBMEM_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/libcore.a: $(LIBCORE_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/libdevices.a: $(LIBDEVICES_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/libtoolchain.a: $(LIBTOOLCHAIN_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/libdebug.a: $(LIBDEBUG_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

$(LIBDIR)/libplugins.a: $(LIBPLUGINS_OBJS) | $(LIBDIR)
	$(AR) $(ARFLAGS) $@ $^

# ---------------------------------------------------------------------------
# Plugin rules
# ---------------------------------------------------------------------------

$(LIBDIR)/mmemu-plugin-6502.so: $(PLUGIN_6502_OBJS) $(LIBDIR)/libmem.a $(LIBDIR)/libcore.a $(LIBDIR)/libtoolchain.a | $(LIBDIR)
	$(CXX) $(CXXFLAGS) -shared -o $@ $(PLUGIN_6502_OBJS) -L$(LIBDIR) -lmem -lcore -ltoolchain

# ---------------------------------------------------------------------------
# Binary rules
# ---------------------------------------------------------------------------

# Helper to link against all libs
BASE_LIBS = -L$(LIBDIR) -lplugins -ldebug -ltoolchain -ldevices -lcore -lmem -ldl

$(CLI_BIN): $(CLI_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CLI_SRCS) $(BASE_LIBS)

$(GUI_BIN): $(GUI_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(WXCXXFLAGS) -o $@ $(GUI_SRCS) $(BASE_LIBS) $(WXLIBS)

$(MCP_BIN): $(MCP_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MCP_SRCS) $(BASE_LIBS)

$(TEST_BIN): $(TEST_SRCS) $(LIBS) $(PLUGIN_6502_OBJS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests -o $@ $(TEST_SRCS) $(PLUGIN_6502_OBJS) $(BASE_LIBS)

# ---------------------------------------------------------------------------
# Generic object rule
# ---------------------------------------------------------------------------

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# ---------------------------------------------------------------------------
# Test target
# ---------------------------------------------------------------------------

test: $(TEST_BIN) plugins
	./$(TEST_BIN)

# ---------------------------------------------------------------------------

clean:
	rm -rf $(BINDIR) $(LIBDIR) $(ALL_LIB_OBJS) $(PLUGIN_6502_OBJS)
