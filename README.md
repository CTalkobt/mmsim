# mmemu — Multi-Machine Emulator / Simulator

**mmsim** (Multi-Machine Simulator) is a universal 8/16-bit emulation platform designed with a modular, plugin-based architecture. It provides a robust host infrastructure that allows different CPU cores, memory models, and I/O devices to be composed into complete machine simulations at runtime.

Help with this development by contributing and buy me a coffee at: https://kodecoffee.com/i/ctalkobt

## 1. Core Architecture

The system is built on a strictly decoupled library system to ensure flexibility and maintainability:

- **libmem**: Abstract memory and address bus models (`IBus`).
- **libcore**: Generic CPU core interfaces (`ICore`) and machine configuration logic.
- **libdevices**: Infrastructure for memory-mapped I/O handlers.
- **libtoolchain**: Multi-ISA assembly and disassembly services.
- **libdebug**: Advanced debugging features including breakpoints, trace history, and system snapshots.

### Plugin System
All processor-specific logic and specialized devices are decoupled from the host and loaded dynamically as `.so` (or `.dll`) modules via the `PluginLoader`. At startup, the system scans the `./lib` directory for modules and registers their capabilities with the core registries.

---

## 2. Plugin Architecture

The **mmsim** ecosystem relies on a modular system where specialized logic is provided by dynamically loadable plugins.

### 2.1 Startup Discovery
At startup, all binaries (`mmemu-cli`, `mmemu-gui`, `mmemu-mcp`) use the `PluginLoader` to scan for resources:
- **Search Path**: `./lib/` relative to the current working directory.
- **Resource Types**: The loader looks for `.so` files (on Linux) that export the required ABI.

### 2.2 The Plugin ABI
Every plugin must export a C-compatible entry point that returns a manifest of its capabilities:
```cpp
extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host);
```

### 2.3 Supported Resource Types
A single plugin module can provide multiple resources, which are automatically registered upon loading:

- **CPU Cores (`ICore`)**: Implementations of processor logic and register sets (e.g., 6502).
- **Toolchains**: Integrated **Assemblers** and **Disassemblers** for specific ISAs.
- **Machine Presets**: Descriptor factories that define how to compose a CPU, memory bus, and devices into a specific system (e.g., C64, VIC-20).
- **I/O Devices (`IOHandler`)**: Individual memory-mapped hardware components.

---

## 3. Supported Machines

**mmsim** currently supports several 8-bit machine presets from the Commodore family:

### Commodore VIC-20
- **Profiles**: `vic20`, `vic20+3k`, `vic20+8k`, `vic20+16k`, `vic20+32k`.
- **Hardware**: MOS 6502 CPU, 6560/6561 VIC-I Video/Sound, dual 6522 VIAs, keyboard matrix, joystick.

### Commodore 64
- **Profile**: `c64`.
- **Hardware**: MOS 6510 CPU, 6567/6569 VIC-II Video, 6581 SID Sound, dual 6526 CIAs, PLA banking, keyboard matrix.

### Commodore PET Series
- **Profiles**: `pet2001`, `pet4032`, `pet8032`.
- **Hardware**: MOS 6502 CPU, 6520 PIA, 6522 VIA, 6545 CRTC (for 4032/8032), discrete video logic (for 2001), IEEE-488 bus.

---

## 4. CLI Target (Implemented)

The `mmemu-cli` binary provides an interactive REPL for low-level machine control and debugging.

### Key Features:
- **Interactive REPL**: Command-driven interface with session management.
- **Machine Lifecycle**: List available machine types and create instances at runtime via the `create <id>` command.
- **CPU Control**: 
    - `step [n]`: Execute specific number of cycles.
    - `regs`: Detailed register inspection (automatically adapts to the current CPU).
    - `asm <addr>`: **Interactive Assembly** - Enter a continuous assembly session at the specified address (exit with `.`).
    - `.<instr>`: **Direct Execution** - Use the native mini-assembler to instantly execute a single instruction (e.g., `.lda #$42`).
- **Memory Tools**:
    - `m <addr> [len]`: Hex and ASCII memory dump.
    - `f <addr> <val> [len]`: Direct memory modification (supports ranges).
    - `copy <src> <dst> <len>`: Copy memory blocks.
    - `load <path> <addr>`: Binary file injection into the simulation space.
- **Disassembly**: Multi-ISA disassembly with integrated symbol resolution.

---

## 5. MCP Target (Implemented)

The `mmemu-mcp` binary implements the **Model Context Protocol**, allowing AI agents (like Claude) to interact directly with the simulator.

### Available Tools:
- `step_cpu(machine_id, count)`: Execute instructions.
- `read_memory(machine_id, addr, size)`: Inspect memory states.
- `write_memory(machine_id, addr, bytes)`: Inject code or data.
- `read_registers(machine_id)`: Inspect CPU state.

---

## 6. GUI Target (Implemented)

The `mmemu-gui` binary provides a professional, multi-pane graphical debugging environment.

### Integrated Panes:
- **Register Pane**: Live view of all CPU registers with automatic change highlighting.
- **Memory Pane**: High-performance scrollable hex/ASCII dump with side-effect-free reads.
- **Disassembly Pane**: Real-time disassembly centered on the current program counter.
- **Console Pane**: Integrated CLI environment with 100% parity with the standalone CLI target.

### Advanced Features:
- **Asynchronous Refresh**: UI updates at 30-60 Hz for a smooth experience.
- **Interactive Dialogs**: Easy machine selection, memory filling, and instruction assembly.
- **Shared Engine**: All core logic is shared between CLI and GUI targets for consistent behavior.

---

## 7. Implementation Roadmap

- **Phase 10 (Complete)**: VIC-20 machine integration — VIA 6522, VIC-I 6560, keyboard matrix.
- **Phase 11 (Complete)**: C64 machine implementation — MOS 6510, C64 PLA, CIA 6526, VIC-II, SID 6581.
- **Phase 12 (Complete)**: PET/CBM machine implementation — MOS 6520 PIA, 6545 CRTC, IEEE-488.
- **Phase 13 (In Progress)**: Runtime Image and Cartridge Loading (`.prg`, `.crt`, `.bin`).
- **Phase 26 (In Progress)**: Atari 8-bit Family (400/800/XL/XE) — ANTIC, GTIA, POKEY.

---

## 8. Plugin Ecosystem

**mmsim** utilizes a modular plugin architecture. For a complete list of available processors, devices, and machine presets, see the **[Plugin Index (doc/README-PLUGINS.md)](doc/README-PLUGINS.md)**.

### 8.1 Machine Types
- [Machine Descriptor JSON Format (machines/README-machines.md)](machines/README-machines.md)
- [VIC-20 Machine Implementation](doc/README-VIC20.md)
- [C64 Machine Implementation](doc/README-C64.md)
- [PET Machine Implementation](doc/README-PET.md)

### 8.2 Video
- [6560 VIC-I (Video/Sound)](doc/README-6560.md)
- [6567/6569 VIC-II](doc/README-VIC2.md)
- [6545 CRTC](doc/README-6545.md)
- [PET Video Subsystem](doc/README-PET-VIDEO.md)

### 8.3 Sound
- [6581 SID Implementation](doc/README-SID.md)

### 8.4 Processors
- [6502/6510 Implementation](doc/README-6502.md)
- [6510 I/O Port & Banking](doc/README-6510.md)

### 8.5 I/O & Peripherals
- [6520 PIA Implementation](doc/README-6520.md)
- [6522 VIA Implementation](doc/README-6522.md)
- [6526 CIA Implementation](doc/README-6526.md)
- [C64 PLA Banking Controller](doc/README-C64PLA.md)
- [PET Keyboard Matrix](doc/README-KBD-PET.md)
- [VIC-20 Keyboard Matrix](doc/README-KBD-VIC20.md)

---

## 9. Getting Started

### Prerequisites

To build **mmsim**, you will need:

- **Compiler**: A C++17 compatible compiler (e.g., GCC 9+, Clang 10+).
- **Build System**: `make`.
- **Libraries**:
    - `spdlog`: Fast C++ logging library.
    - `fmt`: Modern formatting library.
    - `wxWidgets` (3.0+): Required for the GUI target and most machine/device plugins.
    - `ALSA` (`libasound`): Required for audio output in the GUI and test binaries.
    - `nlohmann/json` (3.x): JSON parsing library used by the machine loader.
- **Tools**: `pkg-config` (often used by `wx-config`).

On Debian/Ubuntu-based systems, you can install the dependencies with:
```bash
sudo apt-get install build-essential libwxgtk3.0-gtk3-dev libspdlog-dev libfmt-dev libasound2-dev nlohmann-json3-dev
```

### Building

The project uses a standard `Makefile`.

```bash
make all      # Build CLI, GUI, and MCP binaries and all plugins
make cli      # Build only the CLI binary (mmemu-cli)
make gui      # Build only the GUI binary (mmemu-gui)
make mcp      # Build only the MCP binary (mmemu-mcp)
make plugins  # Build all dynamic plugins in ./lib
make test     # Build and run the unified test suite
make clean    # Remove all build artifacts
```

### Running

After building, the binaries are located in the `bin/` directory, and plugins in the `lib/` directory.

```bash
./bin/mmemu-cli
./bin/mmemu-gui
```

---

## 10. Development Standards
- Adhere to the conventions in [STYLEGUIDE.md](STYLEGUIDE.md).
- Track all significant updates in [CHANGELOG.md](CHANGELOG.md).
