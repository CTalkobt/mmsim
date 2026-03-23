# mmemu — Multi Machine Emulator
# Top-level Makefile
#
# Targets:
#   all   — build cli, gui, mcp  (default)
#   cli   — build bin/mmemu-cli
#   gui   — build bin/mmemu-gui  (requires wxWidgets)
#   mcp   — build bin/mmemu-mcp
#   libs  — build static libraries (lib/*.a)
#   test  — build and run tests
#   clean — remove build artefacts

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -O2
INCLUDES  = -Isrc -Isrc/include
AR        = ar
ARFLAGS   = rcs

WXCONFIG  ?= wx-config
WXCXXFLAGS := $(shell $(WXCONFIG) --cxxflags 2>/dev/null)
WXLIBS     := $(shell $(WXCONFIG) --libs    2>/dev/null)

BINDIR = bin
LIBDIR = lib

# Libraries
LIBS = $(LIBDIR)/libmem.a $(LIBDIR)/libcore.a $(LIBDIR)/libdevices.a \
       $(LIBDIR)/libtoolchain.a $(LIBDIR)/libdebug.a

# Binaries
CLI_BIN = $(BINDIR)/mmemu-cli
GUI_BIN = $(BINDIR)/mmemu-gui
MCP_BIN = $(BINDIR)/mmemu-mcp
TEST_BIN = $(BINDIR)/mmemu-tests

# Sources
CLI_SRCS = src/cli/main.cpp
GUI_SRCS = src/gui/main.cpp
MCP_SRCS = src/mcp/main.cpp
TEST_SRCS = tests/test_main.cpp tests/test_smoke.cpp tests/test_flatmembus.cpp tests/test_cpu6502.cpp tests/test_disasm6502.cpp tests/test_debug.cpp

# Library Sources (wildcards for future files)
LIBMEM_SRCS       = $(wildcard src/libmem/*.cpp)
LIBCORE_SRCS      = $(wildcard src/libcore/*.cpp) $(wildcard src/libcore/machines/*.cpp)
LIBDEVICES_SRCS   = $(wildcard src/libdevices/*.cpp) $(wildcard src/libdevices/devices/*.cpp)
LIBTOOLCHAIN_SRCS = $(wildcard src/libtoolchain/*.cpp)
LIBDEBUG_SRCS     = $(wildcard src/libdebug/*.cpp)

# Objects
LIBMEM_OBJS       = $(LIBMEM_SRCS:.cpp=.o)
LIBCORE_OBJS      = $(LIBCORE_SRCS:.cpp=.o)
LIBDEVICES_OBJS   = $(LIBDEVICES_SRCS:.cpp=.o)
LIBTOOLCHAIN_OBJS = $(LIBTOOLCHAIN_SRCS:.cpp=.o)
LIBDEBUG_OBJS     = $(LIBDEBUG_SRCS:.cpp=.o)

ALL_OBJS = $(LIBMEM_OBJS) $(LIBCORE_OBJS) $(LIBDEVICES_OBJS) \
           $(LIBTOOLCHAIN_OBJS) $(LIBDEBUG_OBJS)

# ---------------------------------------------------------------------------
# Phony targets
# ---------------------------------------------------------------------------

.PHONY: all cli gui mcp libs test clean

all: cli gui mcp

cli: $(CLI_BIN)

gui: $(GUI_BIN)

mcp: $(MCP_BIN)

libs: $(LIBS)

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

# ---------------------------------------------------------------------------
# Binary rules
# ---------------------------------------------------------------------------

$(CLI_BIN): $(CLI_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(CLI_SRCS) $(LIBS)

$(GUI_BIN): $(GUI_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(WXCXXFLAGS) -o $@ $(GUI_SRCS) $(LIBS) $(WXLIBS)

$(MCP_BIN): $(MCP_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(MCP_SRCS) $(LIBS)

$(TEST_BIN): $(TEST_SRCS) $(LIBS) | $(BINDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -Itests -o $@ $(TEST_SRCS) $(LIBS)

# ---------------------------------------------------------------------------
# Generic object rule
# ---------------------------------------------------------------------------

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $<

# ---------------------------------------------------------------------------
# Test target
# ---------------------------------------------------------------------------

test: $(TEST_BIN)
	./$(TEST_BIN)

# ---------------------------------------------------------------------------

clean:
	rm -rf $(BINDIR) $(LIBDIR) $(ALL_OBJS)
