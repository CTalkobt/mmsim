# mmemu — Multi-Machine Emulator / Simulator

**mmsim** (Multi-Machine Simulator) is a universal 8/16-bit emulation platform designed with a modular, plugin-based architecture. It provides a robust host infrastructure that allows different CPU cores, memory models, and I/O devices to be composed into complete machine simulations at runtime.

## 1. Core Architecture

The system is built on a strictly decoupled library system to ensure flexibility and maintainability:

- **libmem**: Abstract memory and address bus models (`IBus`).
- **libcore**: Generic CPU core interfaces (`ICore`) and machine configuration logic.
- **libdevices**: Infrastructure for memory-mapped I/O handlers.
- **libtoolchain**: Multi-ISA assembly and disassembly services.
- **libdebug**: Advanced debugging features including breakpoints, trace history, and system snapshots.

### Plugin System
All processor-specific logic and specialized devices are decoupled from the host and loaded dynamically as `.so` (or `.dll`) modules via the `PluginLoader`. This allows for a "drop-in" ecosystem where new architectures can be added without recompiling the main application.

---

## 2. CLI Target (Implemented)

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

## 3. MCP Target (Implemented)

The `mmemu-mcp` binary implements the **Model Context Protocol**, allowing AI agents (like Claude) to interact directly with the simulator.

### Available Tools:
- `step_cpu(machine_id, count)`: Execute instructions.
- `read_memory(machine_id, addr, size)`: Inspect memory states.
- `write_memory(machine_id, addr, bytes)`: Inject code or data.
- `read_registers(machine_id)`: Inspect CPU state.

---

## 4. GUI Target (Implemented)

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

## 5. Implementation Roadmap

- **Phase 9+: libdevices & Machines**: Implementation of classic hardware components (VIC-II, SID, CIA) and machine presets (C64, VIC-20).
- **Phase 12: Performance Profiling**: Advanced cycle-accurate profiling and execution heatmaps.

---

## 6. Plugin Ecosystem

### Processors
- [6502 (Implemented)](README-6502.md)
- *Z80, 68000, ARM, etc. (Planned)*

### Devices & Machines
- *Documentation for specialized devices and machine presets will be added as they are implemented.*

---

## 5. Getting Started

### Prerequisites
- C++17 compatible compiler (e.g., GCC 9+)
- `make`
- `wxWidgets` (optional, required for GUI target)

### Building
```bash
make all      # Build CLI, GUI, and MCP
make plugins  # Build the core plugins (e.g., 6502)
make test     # Build and run the unified test suite
```

### Running the CLI
```bash
./bin/mmemu-cli
```

---

## 6. Development Standards
- Adhere to the conventions in [STYLEGUIDE.md](STYLEGUIDE.md).
- Track all significant updates in [CHANGELOG.md](CHANGELOG.md).
