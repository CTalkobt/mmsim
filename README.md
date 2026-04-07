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

## 6. GUI Target (mmemu-gui)

The `mmemu-gui` binary provides a professional, multi-pane graphical debugging environment built on wxWidgets. The layout consists of a left column (Disassembly + Memory, and a bottom Console), a center notebook, and a right-side Register pane.

---

### 6.1 Toolbar

| Button | Shortcut | Action |
|---|---|---|
| Machine | Ctrl+L | Open the machine selector dialog |
| Step | F11 | Execute one CPU instruction |
| Run | F5 | Start continuous execution |
| Pause | F6 | Halt continuous execution |
| Go to | Ctrl+G | Jump the memory/disasm view to an address |
| Keyboard Focus | Ctrl+Shift+K | Toggle keyboard capture for the emulated machine |

---

### 6.2 Menus

#### File
| Item | Shortcut | Description |
|---|---|---|
| Load Machine… | Ctrl+L | Select and instantiate a machine preset |
| Exit | | Quit the application |

#### Control
| Item | Shortcut | Description |
|---|---|---|
| Step | F11 | Execute one CPU instruction |
| Run | F5 | Start continuous execution at ~1 MHz |
| Pause | F6 | Stop continuous execution |
| Reset | Ctrl+R | Invoke the machine's reset handler |
| Load Image… | Ctrl+I | Load a `.prg` or `.bin` file into memory |
| Attach Cartridge… | | Attach a `.crt` or `.car` cartridge image |
| Eject Cartridge | | Detach the currently-inserted cartridge and reset |

#### Debug
| Item | Shortcut | Description |
|---|---|---|
| Assemble… | Ctrl+A | Assemble a single instruction at a given address |
| Go to Address… | Ctrl+G | Navigate memory/disasm views to an address; optionally set PC |
| Search Memory… | Ctrl+F | Search memory for a hex or ASCII pattern |
| Find Next | F3 | Jump to the next occurrence of the last search pattern |
| Find Prior | Shift+F3 | Jump to the previous occurrence |
| Fill Memory… | | Fill a memory range with a single byte value |
| Copy Memory… | | Copy a block of memory from one address to another |
| Swap Memory… | | Swap two equal-length memory blocks |
| Breakpoints | Ctrl+B | Show the Breakpoints notebook tab |
| Stack Trace | Ctrl+T | Show the Stack Trace notebook tab |
| Machine Explorer | Ctrl+M | Show the Machine Explorer notebook tab |
| New Memory View | Ctrl+Shift+M | Open a new memory view tab |
| Rename Memory View… | | Rename the currently-selected memory view tab |

---

### 6.3 Panes

#### Disassembly Pane
Displays real-time disassembly of the memory visible around the current program counter. The current PC is highlighted. Automatically follows the PC during single-step or on every refresh tick while running. Uses the machine's registered disassembler for accurate ISA decoding.

#### Memory Pane
A tabbed set of scrollable hex and ASCII dump views of the machine's address bus. Multiple independent memory views can be open simultaneously, each positioned at a different address. Views are displayed as tabs below the Disassembly pane.

- **Navigation**: Use the vertical scroll bar or **Go to Address** (Ctrl+G / Debug menu) to jump to any address in the active view.
- **Multiple views**: Use **Debug → New Memory View** (Ctrl+Shift+M) to open an additional memory view tab. Each tab tracks its own scroll position and address independently. Close any tab with its × button; the last tab cannot be closed.
- **Renaming**: Use **Debug → Rename Memory View…** to give the active tab a descriptive name (e.g. "Stack", "ROM", "Zero Page").
- **In-place editing**: Click any hex byte to open an editor cell (red highlight). Type up to two hex digits and press **Enter** to commit and advance to the next byte, or **Escape** to cancel without writing. Clicking another cell while an editor is open commits nothing and opens a new editor at the clicked position.
- **Context menu** (right-click):
  - **Go to Address…** — navigate the pane to a specific address.
  - **Fill Memory…** — fill a range with a constant byte.
  - **Copy Memory…** — copy a block to another address.
  - **Swap Memory…** — swap two equal-length blocks.
  - **Search Memory…** — search for a hex or ASCII pattern.

#### Console Pane
An embedded interactive REPL with full parity with the `mmemu-cli` standalone binary. Accepts all CLI commands (`step`, `regs`, `m`, `f`, `copy`, `load`, `asm`, etc.) directly in the GUI. Output is displayed in a scrollable text area above the input field.

#### Register Pane
Displays all CPU registers for the currently-loaded machine. Registers that changed since the last refresh are highlighted. Updates automatically at ~30 Hz while running and after each manual step.

#### Screen Pane (plugin-provided)
Visible when a machine with a video output device is loaded (e.g., VIC-20, C64). Renders the video frame produced by the VIC-I or VIC-II chip at ~30 Hz. Contains a **Capture Keyboard** button to toggle keyboard routing to the emulated machine (equivalent to Ctrl+Shift+K).

---

### 6.4 Notebook Tabs

#### Machine
Shows the full machine descriptor: CPU slots, bus slots, and IO device registry. Useful for verifying the hardware composition of the loaded machine preset.

#### Cartridge
Displays metadata for the currently-attached cartridge image (type, bank layout, ROM regions). Empty when no cartridge is attached.

#### Breakpoints
Lists all active breakpoints and watchpoints. Breakpoints can be added, removed, enabled, or disabled. Supports address breakpoints and memory watchpoints. When a breakpoint is hit during Run, execution pauses and the pane is automatically brought into focus.

#### Stack
Displays the call stack trace based on JSR/RTS tracking. Each frame shows the return address. Double-clicking a frame navigates the disassembly pane to that address.

---

### 6.5 Drag and Drop
Program images (`.prg`, `.bin`) and cartridge files (`.crt`, `.car`) can be dragged and dropped onto the main window. Program images open the Load Image dialog pre-filled with the dropped path. Cartridge files open the Attach Cartridge flow directly.

---

## 7. Plugin Ecosystem

**mmsim** utilizes a modular plugin architecture. For a complete list of available processors, devices, and machine presets, see the **[Plugin Index (doc/README-PLUGINS.md)](doc/README-PLUGINS.md)**.

### 7.1 Machine Types
- [Machine Descriptor JSON Format (machines/README-machines.md)](machines/README-machines.md)
- [VIC-20 Machine Implementation](doc/README-VIC20.md)
- [C64 Machine Implementation](doc/README-C64.md)
- [PET Machine Implementation](doc/README-PET.md)

### 7.2 Video
- [6560 VIC-I (Video/Sound)](doc/README-6560.md)
- [6567/6569 VIC-II](doc/README-VIC2.md)
- [6545 CRTC](doc/README-6545.md)
- [PET Video Subsystem](doc/README-PET-VIDEO.md)
- [ANTIC Video Subsystem](doc/README-ANTIC.md)
- [GTIA Color/PMG Subsystem](doc/README-GTIA.md)

### 7.3 Sound
- [6581 SID Implementation](doc/README-SID.md)
- [POKEY Audio/IO Implementation](doc/README-POKEY.md)

### 7.4 Processors
- [6502/6510 Implementation](doc/README-6502.md)
- [6510 I/O Port & Banking](doc/README-6510.md)

### 7.5 I/O & Peripherals
- [6520 PIA Implementation](doc/README-6520.md)
- [6522 VIA Implementation](doc/README-6522.md)
- [6526 CIA Implementation](doc/README-6526.md)
- [C64 PLA Banking Controller](doc/README-C64PLA.md)
- [PET Keyboard Matrix](doc/README-KBD-PET.md)
- [VIC-20 Keyboard Matrix](doc/README-KBD-VIC20.md)

---

## 8. Getting Started

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

## 9. Development Standards
- Adhere to the conventions in [STYLEGUIDE.md](STYLEGUIDE.md).
- Track all significant updates in [CHANGELOG.md](CHANGELOG.md).
