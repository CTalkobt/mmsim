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
- **Symbols**: Standard KERNAL jump table symbols are auto-loaded from `roms/vic20/kernal.sym`.

### Commodore 64
- **Profile**: `c64`.
- **Hardware**: MOS 6510 CPU, 6567/6569 VIC-II Video, 6581 SID Sound, dual 6526 CIAs, PLA banking, keyboard matrix.
- **Symbols**: Standard KERNAL jump table symbols are auto-loaded from `roms/c64/kernal.sym`.

### Commodore PET Series
- **Profiles**: `pet2001`, `pet4032`, `pet8032`.
- **Hardware**: MOS 6502 CPU, 6520 PIA, 6522 VIA, 6545 CRTC (for 4032/8032), discrete video logic (for 2001), IEEE-488 bus.
- **Symbols**: Standard KERNAL jump table symbols are auto-loaded from `roms/pet4032/kernal.sym`.

---

## 4. Symbol Management & Expressions

**mmsim** features a robust expression evaluator and symbol management system integrated into all targets.

### 4.1 Expression Evaluator
Anywhere an address or value is required (CLI, GUI dialogs, MCP), you can use complex expressions:
- **Formats**:
    - **Hex**: `$1000` or `0x1000`.
    - **Binary**: `%10101010`.
    - **Decimal**: `4096`.
- **Symbols**: Use any defined label (e.g., `CHROUT`, `start_vector`).
- **Registers**: Use current CPU register names (e.g., `PC + 2`, `SP`).
- **Arithmetic**: Basic addition (`+`) and subtraction (`-`) are supported.

### 4.2 Symbol Management
Manage symbols via the CLI `sym` command or the GUI **Symbols** pane:
- `sym add <label> <addr>`: Manually add a label.
- `sym del <label>`: Remove a label.
- `sym list`: List all symbols.
- `sym search <query>`: Search for symbols by name.
- `sym load <path>`: Load symbols from a `.sym` file (supports `label = value` and `label value` formats).
- `sym clear`: Clear the current symbol table.

### 4.3 Kernal and BASIC Routine Monitoring
You can monitor the entry and exit points of Kernal and BASIC routines by setting the "kernal" or "basic" logger to DEBUG level:
```bash
log level kernal debug
log level basic debug
```
When enabled, the system will log the routine name and all register states upon entry, and log them again upon exit (detected via the matching `RTS`).

---

## 5. CLI Target (Implemented)

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
    - `swap <addr1> <addr2> <len>`: Swap two equal-length memory blocks.
    - `search <hex...>`: Search for a hex byte pattern.
    - `findnext` / `findprior`: Navigate search results.
    - `load <path> <addr>`: Binary file injection.
- **Symbol Table**: Full management via the `sym` command.
- **Disassembly**: Multi-ISA disassembly with integrated symbol resolution.

---

## 6. MCP Target (Implemented)

The `mmemu-mcp` binary implements the **Model Context Protocol**, allowing AI agents to interact with the simulator.

### Available Tools:
- `step_cpu(machine_id, count)`: Execute instructions.
- `run_cpu(machine_id, max_steps)`: Run until breakpoint or limit.
- `read_memory(machine_id, addr, size)`: Inspect memory.
- `write_memory(machine_id, addr, bytes)`: Inject data.
- `read_registers(machine_id)`: Inspect CPU state.
- `disassemble(machine_id, addr, count)`: Disassemble instructions.
- `list_symbols`, `add_symbol`, `remove_symbol`, `load_symbols`, `clear_symbols`: Manage symbols.
- `set_breakpoint` / `set_watchpoint` / `delete_breakpoint`: Breakpoint management.
- `get_stack(machine_id, count)`: Show the call stack trace.
- `load_image`, `attach_cartridge`, `reset_machine`: Lifecycle and media.
- `list_machines`, `create_machine`: Machine management.
- `list_loggers`, `set_log_level`: Logging control.

---

## 7. GUI Target (mmemu-gui)

Multi-pane graphical debugging environment built on wxWidgets.

### 7.1 Toolbar & Menus
- **Ctrl+Y**: Show the Symbol Table pane.
- **Ctrl+B / Ctrl+T / Ctrl+M**: Quick access to Breakpoints, Stack, and Machine tabs.
- **Debug Menu**: Enhanced with memory fill/copy/swap and symbol management.

### 7.2 Panes
- **Disassembly Pane**: Real-time ISA decoding with symbol resolution.
- **Symbols Pane**: Integrated management of labels with search and "Go To" navigation.
- **Console Pane**: Full parity with `mmemu-cli`.
- **Register & Memory Panes**: Real-time inspection and editing.
- **Stack Pane**: JSR/RTS tracking with navigation support.

---

## 8. Plugin Ecosystem

For a complete list of supported hardware, see **[doc/README-PLUGINS.md](doc/README-PLUGINS.md)**.

---

## 9. Getting Started

### Prerequisites
- **C++17 Compiler** (GCC 9+, Clang 10+).
- **Libraries**: `spdlog`, `fmt`, `wxWidgets` (3.0+), `ALSA` (`libasound`), `nlohmann/json`.

### Building
```bash
make all      # Build everything
make test     # Build and run the test suite (153+ tests)
```

---

## 10. Development Standards
- Adhere to [STYLEGUIDE.md](STYLEGUIDE.md).
- Track updates in [CHANGELOG.md](CHANGELOG.md).
