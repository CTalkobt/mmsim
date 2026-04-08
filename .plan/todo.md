# Implementation Roadmap: Multi-Machine Emulator

This roadmap covers the greenfield build of mmemu. All source files are currently
stubs. Phases build incrementally; each one leaves the project in a compilable,
testable state. VIC-20 is the first complete machine target; C64 follows as the
second, reusing most of the same device and bus infrastructure.

Architecture specs are in `.plan/arch.md`. Section references below point there.

---

## Phase 0: Project Foundation

*Goal: Establish the directory layout, build system, and test harness so every
subsequent phase has a clean home.*

- [x] **Directory skeleton**
   - [x] Create `src/libmem/`, `src/libcore/`, `src/libdevices/`, `src/libtoolchain/`, `src/libdebug/` as library roots (currently only stub headers exist).
   - [x] Create `src/libcore/machines/` for per-machine descriptor factories.
   - [x] Create `src/libdevices/devices/` for individual chip implementations.
   - [x] Create `roms/` for BASIC, KERNAL, and character ROM images (`.gitignore`'d; loaded at runtime).
   - [x] Create `tests/` for unit tests.
- [x] **Build system** (`Makefile`)
   - [x] Compile each `src/lib*/` tree into a static archive (`libmem.a`, `libcore.a`, `libdevices.a`, `libtoolchain.a`, `libdebug.a`).
   - [x] Link `mmemu-cli` and `mmemu-mcp` against all five archives.
   - [x] Link `mmemu-gui` against all five archives plus wxWidgets.
   - [x] Add `make test` target that builds and runs `tests/`.
   - [x] Add `make libs` target that builds archives only (no binaries).
   - [x] Propagate `INCLUDES = -Isrc -Isrc/include` to all translation units.
- [x] **Unit test harness** (`tests/`)
   - [x] Choose and document a single-header test framework (e.g., `doctest.h` or a hand-rolled `test_main.cpp`).
   - [x] Add a trivial passing smoke test so `make test` succeeds from the start.
- [x] **ROM loader utility** (`src/libcore/rom_loader.h/cpp`)
   - [x] `bool romLoad(const char *path, uint8_t *buf, size_t expectedSize)` — loads a binary file, returns false with an error message if the size doesn't match.
   - [x] Used by all machine factories to load BASIC/KERNAL/char ROMs at startup.

---

## Phase 0.5: Architectural Refactor (Critique Response)

*Goal: Address structural deficiencies identified in the Phase 0/1 review.*

- [x] **Interface Segregation**
   - [x] Refactor `ICore` to separate execution, register access, and disassembly.
   - [x] Refactor `IBus` to be leaner.
- [x] **Encapsulation**
   - [x] Move `ExecutionObserver* observer` to private/protected with proper accessors in `ICore` and `IBus`.
- [x] **Side-effect-free Peeking**
   - [x] Ensure `IBus::peek8` is strictly side-effect-free; do not delegate to `read8` by default if `read8` can trigger I/O side effects.
- [x] **Plugin ABI Stability**
   - [x] Refactor `SimPluginHostAPI` to use a stable C-compatible interface (opaque pointers/function pointers) instead of C++ classes.
- [x] **Resource Management**
   - [x] Fix memory leaks in `MachineRegistry::createMachine` and `MachineDescriptor` lifecycle.
- [x] **Undefined Behavior**
   - [x] Fix potential UB in bus masking (e.g., `1u << 32`).

---

## Phase 1: libmem — Address Bus Abstraction

*Goal: A working `IBus` interface and a `FlatMemoryBus` concrete implementation.
See arch.md §3.1.*

- [x] **`IBus` interface** (`src/libmem/ibus.h`)
   - [x] `BusConfig` struct: `addrBits`, `dataBits`, `role` (`BusRole` enum), `littleEndian`, `addrMask`.
   - [x] `IBus` abstract class: `read8`, `write8`, `read16/32`, `write16/32` (default: assemble from `read8/write8`).
   - [x] `peek8(addr)` — side-effect-free read for debugger/disassembler (default: delegates to `read8`).
   - [x] `reset()` virtual hook.
   - [x] Snapshot: `stateSize()`, `saveState()`, `loadState()`.
   - [x] Write log: `writeCount()`, `getWrites()`, `clearWriteLog()` — used by `libdebug` write history pane.
- [x] **`FlatMemoryBus`** (`src/libmem/memory_bus.h/cpp`)
   - [x] Heap-allocated byte array, configurable address width (default 16-bit / 64 KB).
   - [x] ROM overlay support: `addOverlay(base, size, data, writable)` — overlays redirect reads (and optionally writes) to a separate buffer, enabling switchable ROM banks without copying.
   - [x] Write log implemented in `FlatMemoryBus` (ring buffer of `{addr, before, after}` triples, appended on every `write8`).
   - [x] `stateSize/save/load` snapshots the full RAM array.
- [x] **Unit tests** (`tests/test_flatmembus.cpp`)
   - [x] Read/write round-trip for all address widths.
   - [x] ROM overlay: write is silently dropped; read returns ROM data.
   - [x] Write log captures correct before/after values.
   - [x] Snapshot/restore produces identical bus state.

---

## Phase 2: libcore — ICore Interface and MOS 6502 CPU

*Goal: A fully functional MOS 6502 core implementing `ICore`, with a register
descriptor table and disassembler stub. See arch.md §3.2.*

- [x] **`ICore` interface** (`src/libcore/icore.h`)
   - [x] `RegDescriptor` struct: `name`, `width` (`RegWidth` enum), `flags` (`REGFLAG_*`), `alias`.
   - [x] `ICore` abstract class — all methods as specified in arch.md §3.2.
   - [x] `ISA_CAP_*` bitmask constants.
   - [x] `ExecutionObserver* observer` member (linked to `libdebug`; initially `nullptr`).
- [x] **`MOS6502` CPU** (`src/libcore/cpu6502.h/cpp`)
   - [x] Internal state: `A`, `X`, `Y`, `SP`, `PC`, `P` (NV-BDIZC), `cycles`.
   - [x] Full NMOS 6502 opcode table (all official opcodes; common illegal opcodes mostly implemented).
   - [x] Register descriptor table: 8 entries — A, X, Y, SP, PC, P, cycles (REGFLAG_INTERNAL), plus a 9th placeholder for future B/Z extension.
   - [x] `step()` — fetch-decode-execute one instruction, return cycle count; fires `observer->onStep()` if set.
   - [x] `isCallAt()` — checks for `$20` (JSR abs); returns 3 (instruction size) or 0.
   - [x] `isReturnAt()` — checks for `$60` (RTS) or `$40` (RTI).
   - [x] `isProgramEnd()` — true if CPU is in a halt/infinite-loop state (detect `JMP $addr` where addr == PC).
   - [x] `writeCallHarness()` — writes `JSR lo hi; RTS` into scratch memory.
   - [x] `disassembleOne()` — inline disassembler for a single instruction (mnemonic + operand string); used by `libtoolchain`.
   - [x] `disassembleEntry()` — fills a `DisasmEntry` with all fields including `isCall`, `isReturn`, `isBranch`, `targetAddr`.
   - [x] Snapshot: `saveState/loadState` uses `sizeof(CPU6502State)` POD blob.
- [x] **`MachineDescriptor` and `CpuSlot`/`BusSlot`** (`src/libcore/machine_desc.h`)
   - [x] Structs as specified in arch.md §3.3: `CpuSlot`, `BusSlot`, `MachineDescriptor`.
   - [x] `onInit`, `onReset`, `schedulerStep` lifecycle hooks.
   - [x] `MemOverlay` list on descriptor.
- [x] **`MachineRegistry`** (`src/libcore/machines/machine_registry.h/cpp`)
   - [x] Global registry: `registerMachine(id, factoryFn)`, `createMachine(id) → MachineDescriptor*`.
   - [x] `enumerate(vector<string> &ids)` for GUI machine selector.
- [x] **Unit tests** (`tests/test_cpu6502.cpp`)
   - [x] Known-output tests for representative opcodes: LDA imm, STA zpg, ADC, SBC, LSR, ROR, BNE (taken/not taken), JSR/RTS, BRK/RTI.
   - [x] Cycle count accuracy for at least one instruction per addressing mode.
   - [x] Snapshot/restore: run 100 steps, save, run 100 more, restore, verify registers match post-save state.

---

## Phase 3: libtoolchain — Disassembler and Assembler Interfaces

*Goal: A standalone disassembler for 6502 code and an assembler wrapper for
KickAssembler. See arch.md §6.*

- [x] **`IDisassembler`** (`src/libtoolchain/idisasm.h`)
   - [x] `DisasmEntry` struct as in arch.md §6.1.
   - [x] `IDisassembler` abstract class: `isaName()`, `disasmOne()`, `disasmEntry()`.
   - [x] **Base Implementation**: Provide functional delegation to `ICore::disassembleOne()` if a specialized disassembler is not provided.
- [x] **`Disassembler6502`** (`src/libtoolchain/disassembler_6502.cpp`)
   - [x] Wraps `MOS6502::disassembleEntry()`.
   - [x] Resolves operand addresses against a `SymbolTable*` if provided (replaces `$C000` with `main` etc.).
- [x] **`IAssembler`** (`src/libtoolchain/iassembler.h`)
   - [x] `AssemblerResult` struct: `success`, `errorMessage`, `outputPath`, `symPath`, `listPath`.
   - [x] `IAssembler` abstract class: `name()`, `isaSupported()`, `assemble()`.
   - [x] **Backend Metadata**: Implement a mechanism to store tool-specific metadata (binary path, directory location, additional command-line options).
- [x] **`KickAssemblerBackend`** (`src/libtoolchain/kickassembler.h/cpp`)
   - [x] Invokes `java -jar tools/KickAss65CE02.jar` as a child process.
   - [x] **Metadata Configuration**: Support configurable directory location for the JAR and additional user-provided command-line options.
   - [x] Parses stdout for errors; populates `AssemblerResult`.
   - [x] Validates that `tools/KickAss65CE02.jar` exists; returns a clear error if not.
- [x] **`SymbolTable`** (`src/libtoolchain/symbol_table.h/cpp`)
   - [x] Load from KickAssembler `.sym` file (address → label map).
   - [x] `nearest(addr)` — returns the closest label at or before `addr`, plus the byte offset.
- [x] **`SourceMap`** (`src/libtoolchain/source_map.h/cpp`)
   - [x] Load from KickAssembler `.lst` file.
   - [x] `addrToSource(addr) → {file, line}`.
   - [x] `sourceToAddr(file, line) → addr`.
- [x] **Unit tests** (`tests/test_disasm6502.cpp`)
   - [x] Disassemble a hand-crafted byte sequence; compare output strings.
   - [x] `DisasmEntry` fields: verify `isCall`, `isBranch`, `targetAddr` for JSR, BNE, JMP.

---

## Phase 4: libdebug — Debug Infrastructure

*Goal: Breakpoints, trace history, write log, execution observer, and state
snapshots. Drives all debug panes described in arch.md §13.*

- [x] **`ExecutionObserver`** (`src/libdebug/execution_observer.h`)
   - [x] `onStep(ICore*, IBus*, DisasmEntry*)` — called by `ICore::step()` after each instruction.
   - [x] `onMemoryWrite(IBus*, uint32_t addr, uint8_t before, uint8_t after)` — called by `IBus::write8`.
   - [x] Default no-op implementation; `DebugContext` is the concrete subclass.
- [x] **`ExpressionEvaluator`** (`src/libdebug/expression_evaluator.h/cpp`)
   - [x] `evaluate(condition, ICore*, IBus*)` — stub; empty condition returns true (full expression parsing deferred).
- [x] **`BreakpointList`** (`src/libdebug/breakpoint_list.h/cpp`)
   - [x] `Breakpoint` struct: `addr`, `type` (EXEC / READ_WATCH / WRITE_WATCH), `condition` string, `hitCount`, `enabled`.
   - [x] `checkExec(addr, cpu, bus)` — returns the matching breakpoint or nullptr; evaluates condition via `ExpressionEvaluator`.
   - [x] `checkWrite(addr, cpu, bus)` — scans write-watch entries.
   - [x] Add, remove, enable/disable by index.
- [x] **`TraceBuffer`** (`src/libdebug/trace_buffer.h/cpp`)
   - [x] Fixed-size ring buffer of `TraceEntry` (addr, mnemonic string, register snapshot, cycle count).
   - [x] `push(entry)` — overwrites oldest on overflow.
   - [x] Configurable buffer size (default 1 000).
- [x] **`DebugContext`** (`src/libdebug/debug_context.h/cpp`)
   - [x] Implements `ExecutionObserver`.
   - [x] Holds a `BreakpointList`, `TraceBuffer`, and snapshot list.
   - [x] `onStep()` — appends to `TraceBuffer`; evaluates exec breakpoints.
   - [x] `saveSnapshot(label)` / `restoreSnapshot(idx)` — stores/restores `ICore` + `IBus` state blobs.
   - [x] `diffSnapshots(i, j)` — returns lists of changed registers and changed memory addresses.
- [x] **Unit tests** (`tests/test_debug.cpp`)
   - [x] Breakpoint fires at correct address; `hitCount` increments.
   - [x] Write-watch fires on matching write; not on reads.
   - [x] Trace buffer wraps correctly at capacity.
   - [x] Snapshot round-trip: save → mutate → restore → verify state identical to save point.

---

## Phase 5: CLI Target Implementation

*Goal: A functional command-line interface for manual machine control.*

- [x] **Command Loop** (`src/cli/main.cpp`)
   - [x] Interactive REPL with input parsing.
   - [x] `step`, `run`, `pause`, `stop` commands mapped to `ICore`.
   - [x] `regs` command: displays all registers from `ICore::regDescriptor()`.
   - [x] `m` (memory) and `f` (fill) commands using `IBus::read8/write8`.
   - [x] `disasm` command using `IDisassembler` interface.
   - [x] `load` command to load binary files into memory.
   - [x] **Direct Instruction Execution**: Support `.<instr>` prefix for instant assembly and execution.
- [x] **Machine Integration**
   - [x] Support selecting machines from `MachineRegistry`.
   - [x] Handle multiple CPU slots if present in the descriptor.
- [x] **Debug Commands**
   - [x] `break <addr>` / `watch <read|write> <addr>` — add exec/watchpoints via `DebugContext`.
   - [x] `delete <id>`, `enable <id>`, `disable <id>` — manage breakpoints.
   - [x] `info breaks` — list active breakpoints.
   - [x] `run [addr]` loops until `DebugContext::isPaused()` or `isProgramEnd()`.
   - [x] Integration test: `tests/test_breakpoints.cpp`.

---

## Phase 6: Plugin Architecture & Refactor

*Goal: Decouple 6502-specific code into a loadable plugin and implement the Plugin Loader.*

- [x] **Finalize Plugin ABI** (`src/include/mmemu_plugin_api.h`)
   - [x] Define C-compatible structs for `SimPluginManifest`, `CorePluginInfo`, etc.
   - [x] Add factory function pointers for `ICore`, `IDisassembler`, and `IAssembler`.
- [x] **Plugin Loader Implementation** (`src/plugin_loader/`)
   - [x] Implement `PluginLoader::load(path)` using `dlopen`/`dlsym`.
   - [x] Implement automatic plugin discovery in standard paths.
   - [x] Registry integration: automatically register plugin-provided cores/toolchains.
- [x] **Create 6502 Plugin** (`src/plugins/6502/`)
   - [x] Move `MOS6502`, `Disassembler6502`, and `Assembler6502` to the plugin directory.
   - [x] Implement `mmemuPluginInit` entry point for the 6502 plugin.
   - [x] Update `Makefile` to build `mmemu-plugin-6502.so`.
- [x] **Decouple Host**
   - [x] Remove hardcoded 6502 references from `libcore` and `libtoolchain`.
   - [x] Update CLI to use `PluginLoader` and request "6502" tools from the registry.

---

## Phase 7: MCP Target Implementation

*Goal: Enable AI agents to interact with the simulator via the Model Context Protocol.*

- [x] **MCP Server** (`src/mcp/main.cpp`)
   - [x] Implementation of the MCP specification over JSON-RPC 2.0.
   - [x] Communication over stdin/stdout.
- [x] **Tools Registration**
   - [x] `step_cpu(machine_id, count)`: Execute N instructions.
   - [x] `read_memory(machine_id, addr, size)`: Hex dump or raw data.
   - [x] `write_memory(machine_id, addr, bytes)`: Inject data.
   - [x] `read_registers(machine_id)`: Get full CPU state.
- [x] **Resource Registration**
   - [x] `machine_state`: Snapshot of the current session state.
- [x] **Validation Harness**
   - [x] Use MCP tools to automate the execution and validation of 6502 test programs.

---

## Phase 8: GUI Target Implementation

*Goal: A rich graphical interface for machine simulation and debugging.*
See Cross-Cutting GUI Standards for pane requirements. 

- [x] **Application Skeleton** (`src/gui/main.cpp`)
   - [x] wxWidgets main frame with menu, toolbar, and status bar.
   - [x] Tabbed or multi-pane layout for different debug views.
- [x] **Register Pane**
   - [x] Dynamically creates rows from `RegDescriptor` table.
   - [x] Highlights changed registers since the last step.
   - [x] Supports multi-CPU slot selection.
- [x] **Memory Pane**
   - [x] Hex/ASCII dump with scroll support.
   - [x] Bus selector dropdown for Harvard architectures.
   - [x] Highlights recently written bytes using `IBus::getWrites()`.
- [x] **Disassembly Pane**
   - [x] Scrollable view centered on the current PC.
   - [x] Integration with `IDisassembler` and `SymbolTable`.
- [x] **Machine Selector**
   - [x] Dialog to choose machine presets from `MachineRegistry`.
   - [x] Display machine ID, display name, and license class.

---

## Phase 8.5: GUI Advanced Debugger Features

*Goal: Port advanced CLI features to the GUI to provide parity for power users.*

- [x] **CLI Console Pane**
   - [x] Integrated text console for direct command input (matching Phase 5 CLI features).
   - [x] Support for all CLI commands within the GUI environment.
- [x] **Memory Manipulation Tools**
   - [x] "Fill Memory" dialog: Specify start address, length, and byte value.
   - [x] "Copy Memory" dialog: Specify source range and destination address.
- [x] **Inline Assembler**
   - [x] "Assemble" dialog: Input single assembly instruction and target address.
   - [x] Integration with `IAssembler::assembleLine` from `libtoolchain`.
- [x] **Navigation & Search**
   - [x] "Go to Address" shortcut/dialog in Memory and Disassembly panes. Allow
          setting of PC in CLI if not already available. 
   - [x] Search for hex patterns or ASCII strings in memory. Add capability to CLI and 
          MCP if not already available.

---

## Phase 9: libdevices — IOHandler Base Layer

*Goal: The abstract `IOHandler`/`IORegistry` infrastructure shared by all
devices. No concrete chips yet — those come in Phases 10 and 11.*

- [x] **`IOHandler`** (`src/libdevices/io_handler.h`)
   - [x] Abstract base: `name()`, `baseAddr()`, `addrMask()`.
   - [x] `ioRead(IBus*, uint32_t addr, uint8_t* val) → bool` — return true if handled.
   - [x] `ioWrite(IBus*, uint32_t addr, uint8_t val) → bool`.
   - [x] `reset()`, `tick(uint64_t cycles)` — called each step for timers/counters.
- [x] **`IORegistry`** (`src/libdevices/io_registry.h/cpp`)
   - [x] `registerHandler(IOHandler*)` — stores in priority order by base address.
   - [x] `dispatchRead(addr, val*)` / `dispatchWrite(addr, val)` — walks handler list, returns on first match.
   - [x] `tickAll(uint64_t cycles)` — calls `tick()` on every handler.
   - [x] `resetAll()`.
   - [x] `enumerate(vector<IOHandler*>&)` — for device inspector pane.
- [x] **`IPortDevice`** (`src/libdevices/iport_device.h`)
   - [x] `readPort() → uint8_t`, `writePort(uint8_t val)`, `setDdr(uint8_t ddr)`.
   - [x] Used to inject keyboard matrix, joystick, cassette, etc. into VIA ports without coupling VIA logic to specific peripherals.
- [x] **`ISignalLine`** (`src/libdevices/isignal_line.h`)
   - [x] `get() → bool`, `set(bool)`, `pulse()`.
    - [X] Used for single-bit control lines: ATN, SRQ, cassette motor, IRQ, NMI.
- [X] **Unit tests** (`tests/test_io_registry.cpp`)
    - [X] Two handlers at non-overlapping ranges; verify each is dispatched correctly.
    - [X] Read returns false when no handler covers the address.
   - [x] `tickAll` calls `tick()` on every registered handler.

---

## Phase 10: VIC-20 Machine [COMPLETED]

*Goal: A fully bootable VIC-20 simulation. This is the first machine where all
five libraries are exercised together.*

### Phase 10.1 MOS 6522 VIA (`src/plugins/devices/via6522/`)

The VIC-20 has two VIA chips. One implementation services both.

- [x] Register file: ORA, DDRA, ORB, DDRB, T1CL/CH (latch+counter), T2CL/CH, SR, ACR, PCR, IFR, IER, IORA.
- [x] Timer 1: continuous / one-shot mode; underflow sets IFR bit 6; triggers IRQ via `ISignalLine`.
- [x] Timer 2: one-shot pulse count; underflow sets IFR bit 5.
- [x] Port A / Port B: reads consult an injected `IPortDevice*`; DDR masks applied correctly. Unconnected pins read as 1.
- [x] CB2 / CA2 handshake lines via `ISignalLine`.
- [x] `tick(cycles)` updates timer counters and fires interrupts.
- [x] `reset()` clears all registers to power-on state.
- [x] `IOHandler` interface: responds to 16-byte window, mirrored across the VIA's aliased address range.

### Phase 10.2 MOS 6560/6561 VIC (`src/plugins/devices/vic6560/`)

*The VIC-I chip used in the VIC-20 (6560 NTSC, 6561 PAL).*

- [x] Register file (16 registers, $9000–$900F): interlace, screen origin X/Y, columns, rows, double-size, raster compare, memory pointers, light pen X/Y, paddle X/Y, sound, volume/colour, auxiliary colour, border colour, background colour.
- [x] Video geometry: screen columns and rows read from registers; default 22×23 visible characters.
- [x] Character fetch: reads character codes from screen RAM, then glyph data from character ROM, using memory pointers in VIC registers. Uses `IBus::peek8()` to avoid side effects.
- [x] Colour RAM: separate 4-bit colour array at $9600–$97FF (400 bytes for 22×23). Stored as a plain array owned by the VIC-20 machine descriptor; VIC receives a pointer.
- [x] Raster counter: increments each line; comparison register triggers IRQ.
- [x] Sound: three square-wave tone generators (high/low nibble frequency + on/off bits) and one noise channel. Volume register. Amplitude model only — no waveform synthesis at this stage (can be upgraded later).
- [x] `IVideoOutput` interface: `renderFrame()` fills an RGBA pixel buffer with the current frame using current register state and bus data.
- [x] `tick(cycles)` drives the raster counter; fires raster IRQ via `ISignalLine` when the counter matches.

### Phase 10.3 VIC-20 Memory Map (`src/libcore/machines/machine_vic20.cpp`)

```
Address       Size    Description
$0000–$03FF    1 KB   Zero page, stack, system vectors
$0400–$0FFF    3 KB   Unexpanded: unmapped (open bus); 3K expansion fills this
$1000–$1DFF    3.5KB  Main user RAM
$1E00–$1FFF    0.5KB  Default screen RAM (22×23 char codes = 506 bytes)
$2000–$7FFF   24 KB   BLK1–BLK3 cartridge/expansion slots (unmapped by default)
$8000–$8FFF    4 KB   Character ROM (VIC-20 char set)
$9000–$900F   16 B    VIC-I registers (mirrored to $93FF)
$9110–$911F   16 B    VIA #1 (mirrored to $93FF)
$9120–$912F   16 B    VIA #2 (mirrored to $93FF)
$9400–$95FF  512 B    Colour RAM nybbles (low 4 bits used; mirrored)
$9600–$97FF  512 B    Colour RAM alias (actual accessible window)
$A000–$BFFF    8 KB   BLK5 cartridge slot (unmapped by default)
$C000–$DFFF    8 KB   BASIC ROM
$E000–$FFFF    8 KB   KERNAL ROM
```

- [x] `FlatMemoryBus(16)` for the 64 KB address space.
- [x] Load character ROM at $8000 via `romLoad()`.
- [x] Load BASIC ROM at $C000 via `romLoad()` with `writable=false` overlay.
- [x] Load KERNAL ROM at $E000 via `romLoad()` with `writable=false` overlay.
- [x] IORegistry entries: VIC ($9000), VIA #1 ($9110), VIA #2 ($9120).
- [x] Colour RAM as a plain 512-byte array at $9400–$97FF; `FlatMemoryBus` overlay for read/write (4-bit mask on write).
- [x] `MachineDescriptor` id `"vic20"`, single CPU slot (`MOS6502`), single bus slot (`"system"`).
- [x] `onReset`: loads ROMs (if not already loaded), resets CPU and all devices.
- [x] Register factory with `MachineRegistry::registerMachine("vic20", ...)` — all five variants registered via `s_machines[]` in `plugin_init.cpp`; `PluginLoader::registerPluginItems` handles this automatically at plugin load time.

### Phase 10.4 VIC-20 Peripheral Wiring

- [x] **Keyboard matrix** (`src/plugins/devices/keyboard/main/keyboard_matrix.h/cpp`)
   - [x] 8×8 matrix (64 keys), wired to VIA #1 port A (column output) and VIA #2 port B (row input).
   - [x] `keyDown(row, col)` / `keyUp(row, col)` API for the GUI/CLI to inject keystrokes.
   - [x] Implements `IPortDevice` on both ports; VIA reads return appropriate row bits.

- [x] **Joystick** (`src/libdevices/joystick.h/cpp`)
   - [x] 5-bit active-low directional + fire, injected into VIA #1 port B.
   - [x] `setState(uint8_t bits)` API.

### Phase 10.5 Plugin Extension Host API

*Goal: Define the interfaces and host-side registration infrastructure that let
dynamically loaded plugins contribute GUI panes, CLI commands, and MCP tools
without any static linkage to the host binaries. All three frontends consume the
same registration layer; individual plugins opt in to whichever surfaces they
need.*

**Extend `SimPluginHostAPI`** (`src/include/mmemu_plugin_api.h`)

- [x] Add function pointers to `SimPluginHostAPI` for each extension point:
    - `registerPane(const PluginPaneInfo*)` — GUI pane registration (no-op in CLI/MCP hosts).
    - `registerCommand(const PluginCommandInfo*)` — CLI command registration (no-op in GUI/MCP hosts).
    - `registerMcpTool(const PluginMcpToolInfo*)` — MCP tool registration (no-op in CLI/GUI hosts).
- [x] Each function pointer is set to a real implementation in the host that
      supports it, and to a safe no-op stub in hosts that do not, so plugins call
      unconditionally without host-capability checks.

**GUI pane extension** (`src/include/mmemu_plugin_api.h`, `src/gui/plugin_pane_manager.h/cpp`)

- [x] `struct PluginPaneInfo` — all window handles are `void*`; no wxWidgets
      types appear anywhere in this struct or in `mmemu_plugin_api.h`:
    - `const char* paneId` — unique identifier (e.g. `"vice-importer.main"`).
    - `const char* displayName` — tab/title label shown in notebook and menu.
    - `const char* menuSection` — top-level menu name under which a show/hide
      item is placed (e.g. `"Tools"` or `"View"`); `nullptr` to suppress menu entry.
    - `const char* const* machineIds` — null-terminated list of machine IDs for
      which the pane is relevant; `nullptr` means show for all machines.
    - `void* (*createPane)(void* parentHandle, void* ctx)` — factory called on
      the GUI thread; `parentHandle` is the opaque parent window; returns an
      opaque pane handle. `ctx` is the plugin-supplied context pointer.
    - `void  (*destroyPane)(void* paneHandle, void* ctx)` — called before the
      pane is removed (machine switch or shutdown); plugin frees its resources.
    - `void  (*refreshPane)(void* paneHandle, uint64_t cycles, void* ctx)` —
      called by the host's cycle timer (or on any registered event) so the plugin
      can update displayed data; must return quickly (no blocking I/O).
    - `void* ctx` — plugin-supplied opaque context passed to all callbacks.
- [x] `PluginPaneManager` (`src/gui/plugin_pane_manager.h/cpp`) — the only site
      that casts `void*` handles to `wxWindow*`:
    - Maintains the registered `PluginPaneInfo` list and a parallel list of live
      pane handles (one per registered pane, null until first shown).
    - **Menu integration**: for each registered pane with a non-null `menuSection`,
      appends a checkable menu item under that section; toggling it
      shows/hides the pane and keeps the check state in sync.
    - **Machine switch**: hides and destroys panes whose `machineIds` do not
      include the new machine ID; lazily creates and shows panes that match.
      The corresponding menu items are enabled/disabled to match visibility.
    - **Notebook integration**: panes appear as additional tabs in the main
      frame's `wxAuiNotebook`; users can drag, float, or close them like any
      built-in pane.
    - **Periodic refresh**: the host's existing cycle/render timer calls
      `PluginPaneManager::tickAll(uint64_t cycles)`, which iterates live panes
      and invokes `refreshPane` on each. No wxWidgets types cross the boundary —
      the opaque `void* paneHandle` is passed straight through.
    - `destroyPane` is called on tab close or machine unload before the
      `wxWindow*` is deleted.

**CLI command extension** (`src/include/mmemu_plugin_api.h`, `src/cli/plugin_command_registry.h/cpp`)

- [x] `struct PluginCommandInfo`:
    - `const char* name` — command token (e.g. `"importroms"`).
    - `const char* usage` — one-line usage string printed by `help`.
    - `int (*execute)(int argc, const char* const* argv, void* ctx)` — called on
      the CLI thread; returns 0 on success, non-zero on error.
    - `void* ctx` — plugin-supplied opaque context.
- [x] `PluginCommandRegistry` (`src/cli/plugin_command_registry.h/cpp`):
    - `registerCommand(const PluginCommandInfo*)` — stores entry; rejects
      duplicates (logs a warning, does not overwrite built-in commands).
    - `dispatch(const std::vector<std::string>& tokens)` — called by the CLI REPL
      after built-in command lookup fails; returns false if no plugin handles it.
    - `listCommands(std::vector<std::string>&)` — appended to `help` output.

**MCP tool extension** (`src/include/mmemu_plugin_api.h`, `src/mcp/plugin_tool_registry.h/cpp`)

- [x] `struct PluginMcpToolInfo`:
    - `const char* toolName` — JSON-RPC method name (e.g. `"importroms"`).
    - `const char* schemaJson` — JSON Schema string for the `params` object.
    - `void (*handle)(const char* paramsJson, char** resultJson, void* ctx)` —
      called on the MCP thread; writes a heap-allocated JSON string to
      `*resultJson`; the host frees it with the companion `freeString` pointer.
    - `void (*freeString)(char*)` — plugin-provided free function (avoids
      cross-module `free` mismatch).
    - `void* ctx` — plugin-supplied opaque context.
- [x] `PluginToolRegistry` (`src/mcp/plugin_tool_registry.h/cpp`):
    - `registerTool(const PluginMcpToolInfo*)` — stores entry; rejects name
      collisions with built-in tools.
    - `dispatch(const std::string& method, const std::string& paramsJson, std::string& resultJson)` — returns false if no plugin handles the method.
    - `listTools(std::vector<std::string>&)` — appended to `tools/list` response.

**Dynamic loading contract**

- [x] `PluginLoader::load()` (Phase 6) is extended to call `registerPane`,
      `registerCommand`, and `registerMcpTool` via the already-populated
      `SimPluginHostAPI` pointer passed into `mmemuPluginInit`. No additional
      loader changes are required beyond ensuring the function pointers are set
      before the plugin entry point is called.
- [x] Plugins that do not need a given surface simply do not call the
      corresponding registration function; no stub implementation is required in
      the plugin.

**Unit tests** (`tests/test_plugin_extension.cpp`)

- [x] `PluginCommandRegistry`: register two commands; verify dispatch routes to
      the correct handler; verify unknown token returns false.
- [x] `PluginCommandRegistry`: registration of a name that collides with a
      built-in command is rejected and returns false.
- [x] `PluginToolRegistry`: register a tool; `dispatch` calls its handler and
      returns the JSON result; unknown method returns false.
- [x] `PluginPaneInfo` struct is fully expressible in plain C (no wxWidgets
      headers required); verified by compiling a test TU with no wx include paths.
- [x] `PluginPaneManager` (tested via a wx-free mock that substitutes `void*`
      stubs for real windows): registration stores entry; `machineIds` filter
      suppresses `createPane` for non-matching machine IDs and invokes it for
      matching ones; `tickAll` calls `refreshPane` on all live panes.

### Phase 10.6 VIC-20 Integration Test

- [x] Assemble a minimal test program (LDA #$41 / STA $1E00 / JMP *) directly as machine code bytes.
- [x] Load into bus, reset, run until `isProgramEnd()`.
- [x] Verify the correct byte appeared in screen RAM at $1E00.
- [x] Run for enough cycles to advance at least two VIC-I raster lines; verify via `IORegistry::dispatchRead` at $9004.
- [x] **VIA timer validation**: program VIA1 T1, tick past underflow, verify IFR bit 6 via `IORegistry::dispatchRead` at $911D.
- [x] **Extension Host Verification**:
    - [x] Verify the VIC-20 machine correctly identifies itself (`machineId == "vic20"`) for pane filtering.
    - [x] Verify that the `"importroms"` CLI command is registered when the vice-importer `.so` is loaded via `PluginLoader`.

### Phase 10.7 VICE ROM Importer Plugin (`src/plugins/viceImporter/`)

*Goal: Provide an end-user-triggered utility that discovers an existing VICE
installation on the host system and copies the VIC-20 ROM images into the
emulator's `roms/` directory. The utility is never invoked automatically — the
user must explicitly request it via a CLI command or GUI menu item.*

**Plugin identity and wiring**

- [x] Plugin entry point `mmemuPluginInit` in `src/plugins/viceImporter/main/plugin_main.cpp`; plugin manifest declares:
    - `pluginId = "vice-importer"`, `displayName = "VICE ROM Importer"`.
    - Dependency on `"vic20"` machine plugin (listed in `SimPluginManifest::deps[]`).
    - `supportedMachineIds[]` = `{ "vic20" }` — the importer only activates for
      machine IDs it knows about; adding future machines (C64, etc.) requires
      explicit additions here.
- [x] Receives the `SimPluginHostAPI` pointer and stores it for subsequent registration calls.
- [x] `Makefile` target builds `lib/mmemu-plugin-vice-importer.so`; link order
      ensures the vic20 machine plugin is loaded first.

**Discovery engine** (`src/plugins/viceImporter/main/rom_discovery.h/cpp`)

- [x] `struct RomSource { std::string label; std::string basePath; }` — describes
      a candidate VICE installation.
- [x] `std::vector<RomSource> discoverSources(const std::string& machineId)`
    - Searches a hardcoded list of well-known VICE install paths (platform-specific:
      `~/.local/share/vice`, `/usr/share/vice`, `/usr/share/games/vice`,
      `/opt/vice`, `~/Applications/VICE.app`, `C:\Program Files\VICE`).
    - For each candidate path, verifies that the expected VIC-20 ROM files are
      present before including it in the result (no partial matches).
    - Returns an empty vector if nothing is found; never throws.
    - Internal `discoverSourcesInPaths(machineId, paths)` helper exposed for testing.
- [x] `struct RomFileSpec { std::string srcRelPath; std::string destName; size_t expectedSize; }` — maps one source ROM file to its canonical destination name and expected byte size.
- [x] `std::vector<RomFileSpec> romFilesFor(const std::string& machineId)` — returns the spec list for `"vic20"`: `basic.bin` (8 KB), `kernal.bin` (8 KB), `char.bin` (4 KB) sourced from VICE's `VIC20/` subdirectory.

**Import operation** (`src/plugins/viceImporter/main/rom_importer.h/cpp`)

- [x] `ImportResult importRoms(const RomSource& src, const std::string& machineId, const std::string& destDir)`
    - Copies each file listed by `romFilesFor()` from `src.basePath` to `destDir/`.
    - Validates byte size of each file after copy; rolls back (deletes copied files)
      on any mismatch or IO error, and returns a descriptive error string.
    - Never overwrites an existing file unless the caller sets `overwrite = true`
      (prevents silent ROM replacement).
    - Returns `{ success, std::vector<std::string> copiedFiles, std::string errorMessage }`.

**CLI integration** (via Phase 10.5 `PluginCommandRegistry`)

- [x] Registers a `PluginCommandInfo` for `"importroms"` during `mmemuPluginInit`.
- [x] Command syntax: `importroms <machineId> [--list] [--source <n>] [--dest <path>] [--overwrite]`:
    - `--list`: prints discovered sources and exits (dry-run, no files copied).
    - `--source <n>`: selects source by index from the discovered list; if omitted
      and exactly one source exists it is used automatically; if multiple exist the
      command prints the list and asks the user to re-run with `--source`.
    - `--dest`: overrides the default destination (`roms/`).
    - `--overwrite`: permits replacing existing ROM files.
    - Prints a per-file copy summary and a final success/failure line.

**GUI integration** (via Phase 10.5 `PluginPaneManager`)

- [x] Registers a `PluginPaneInfo` with `machineIds = { "vic20", nullptr }` during `mmemuPluginInit`.
- [x] `createPane` returns a `RomImportPane` (`wxPanel` subclass):
    - Dropdown of discovered sources populated by `discoverSources()`.
    - Read-only display of files that will be copied and their destination.
    - "Import" button disabled when no source is selected; enabled otherwise.
    - Status feedback inline within the pane; error shown on failure.
    - On success, displays a prompt suggesting the user reset/reload the machine.
- [x] Pane is automatically hidden by `PluginPaneManager` when a non-VIC-20 machine is active.

**MCP integration** (via Phase 10.5 `PluginToolRegistry`)

- [x] Registers a `PluginMcpToolInfo` for `"import_roms"` during `mmemuPluginInit`.
- [x] Schema defines `machineId` (string, required), `sourceIndex` (integer, optional), and `overwrite` (boolean, optional).
- [x] Handler invokes `importRoms()` and returns a JSON response with success status and list of copied files.

**Unit tests** (`tests/test_vice_importer.cpp`)

- [x] `discoverSourcesInPaths("vic20", paths)` with a temp-dir mock filesystem returns expected sources; missing/wrong-size files are excluded.
- [x] `importRoms()` copies files to a temp directory; validates sizes match specs.
- [x] `importRoms()` with a deliberate size mismatch rolls back all copied files and returns a non-empty error string.
- [x] `importRoms()` does not overwrite an existing file unless `overwrite=true`.

---

## Phase 11: C64 Machine [COMPLETED]

*Goal: A bootable C64 simulation. Reuses `FlatMemoryBus`, `MOS6502` (via
`MOS6510` subclass), VIA→CIA upgrade, and adds VIC-II and SID.*

### Phase 11.1 MOS 6510 CPU (`src/plugins/6502/main/cpu6510.h/cpp`) [COMPLETED]

The 6510 is a 6502 with a built-in 6-bit I/O port at addresses $00 (DDR) and $01 (DATA). The port controls the PLA banking lines.

- [x] Subclass `MOS6502`; installs a proxy bus (PortBus) that intercepts $00/$01 before passing all other accesses through to the real bus (necessary because MOS6502::read/write are private).
- [x] Port bits: bit 0 = LORAM, bit 1 = HIRAM, bit 2 = CHAREN, bits 3–5 = cassette/serial (not used for banking).
- [x] Exposes `ISignalLine` for each port output (`signalLoram()`, `signalHiram()`, `signalCharen()`) so the PLA handler can observe changes.
- [x] Registered as `"6510"` core in the 6502 plugin manifest; `"raw6510"` machine factory added.

### Phase 11.2 C64 PLA Banking (`src/plugins/devices/c64_pla/main/c64_pla.h/cpp`) [COMPLETED]

The PLA maps the 6510 port bits to ROM/IO visibility in the upper address space.

- [x] `C64PLA` registered as `IOHandler` at baseAddr=$A000 so it sorts before $D000 devices in IORegistry.
- [x] `ioRead` implements the full banking matrix:
    - $E000–$FFFF: KERNAL ROM when HIRAM=1; flat RAM otherwise.
    - $A000–$BFFF: BASIC ROM when HIRAM=1 && LORAM=1; flat RAM otherwise.
    - $D000–$DFFF: Char ROM when HIRAM=1 && CHAREN=0; returns false (I/O devices respond) when CHAREN=1; flat RAM when HIRAM=0.
- [x] All writes return false: ROM areas accept writes to underlying flat RAM; I/O writes reach device handlers in the dispatch chain.
- [x] ROM data pointers set via `setBasicRom/setKernalRom/setCharRom`; signal lines via `setSignals(loram, hiram, charen)`.

### Phase 11.3 MOS 6526 CIA (`src/plugins/devices/cia6526/main/cia6526.h/cpp`) [COMPLETED]

- [x] Register file: PRA, PRB, DDRA, DDRB, TA lo/hi (latch + counter), TB lo/hi, TOD 10ths/sec/min/hr, SDR, ICR, CRA, CRB.
- [x] Timer A / Timer B: continuous and one-shot modes; underflow sets ICR bits 0/1; TB can count TA underflows (CRB bits 5–6).
- [x] TOD (Time of Day): BCD clock; tenths-to-hours cascade with AM/PM; alarm IRQ; latch frozen on TODHR read, released on TODTEN read; writing TODHR halts clock until TODTEN written.
- [x] ICR: write bit 7=1 sets mask bits, bit 7=0 clears; read returns pending|IRQ_ANY then clears all.
- [x] `keyDown` / `keyUp` injected into PRA/PRB via `IPortDevice*` (same interface as VIA6522).
- [x] `reset()` clears all registers; IRQ line de-asserted; port devices notified of DDR=0.

### Phase 11.4 MOS 6567/6569 VIC-II (`src/plugins/devices/vic2/main/vic2.h/cpp`) [COMPLETED]

- [x] Register file ($D000–$D02E, 47 registers): Sprites X/Y (×8), MSB X, control 1/2, raster compare, light pen X/Y, sprite enable/expand/priority/multicolor/collision, IRQ status/enable, border/background/sprite colours. Reads beyond $D02E return $FF; colour registers return $F0 in high nibble.
- [x] Sprite engine: 8 sprites, 24×21 pixels; sprite data pointer read from screen_base+1016+sp; X/Y expansion (doubles pixels/rows); monochrome and multicolor ($D025/$D026 shared colors); priority (behind/in-front of background); sprite-sprite bounding-box collision detection ($D01E); sprite-background pixel collision detection ($D01F). Collision IRQs fire via $D019/$D01A.
- [x] Video modes: Standard text, Multicolor text, Standard bitmap, Multicolor bitmap, Extended background color (ECB). Mode selected by $D011 ECM/BMM and $D016 MCM bits.
- [x] VIC-II banking: `setDmaBus(IBus*)` + `setBankBase(uint32_t)` for 16 KB bank selected by CIA #2 port A. Character ROM accessible at bank offsets $1000–$1FFF and $9000–$9FFF via `setCharRom()`.
- [x] `renderFrame()`: frame-based renderer (384×272 total, 320×200 display area, 32px left/right border, 36px top/bottom). Renders background then composites sprites with priority and collision.
- [x] `tick(cycles)`: cycle-accumulating raster counter (65 cycles/line × 263 lines NTSC); fires raster IRQ via `ISignalLine` when raster matches $D011 bit-8 + $D012 compare value.

### Phase 11.5 MOS 6581 SID (`src/plugins/devices/sid6581/main/sid6581.h/cpp`) [COMPLETED]

- [x] Three voices (offsets +0 to +6 per voice × 3): FREQ lo/hi, PW lo/hi (12-bit), Control (Gate/Sync/Ring/Test/Tri/Saw/Pulse/Noise), Attack/Decay, Sustain/Release.
- [x] Waveform generators: Triangle (folded from 24-bit phase accumulator), Sawtooth (upper 12 bits), Pulse (threshold on upper 12 bits vs. pulse-width register), Noise (23-bit Galois LFSR, taps 22 and 17, clocked on phase bit-19 rising edge; output mapped from 8 LFSR bit positions to 12-bit value). Multiple active waveforms are ANDed (real SID behaviour for combined waveforms).
- [x] Hard sync: voice phase reset when sync-source oscillator wraps. Ring modulation: triangle MSB XOR'd with sync-source phase MSB.
- [x] Test bit: freezes phase accumulator at 0, loads LFSR with 0x7FFFFF.
- [x] ADSR envelope: 8-bit level (0–255); attack linear 0→255 using standard SID rate table (9–31250 cycles/step at 1 MHz); decay/release at 3× the attack rate, linear 255→sustain→0; gate 0→1 triggers attack, 1→0 triggers release.
- [x] Filter: Chamberlin state-variable filter (LP/BP/HP simultaneously available). 11-bit cutoff mapped logarithmically 30–12 kHz; resonance from $D417 upper nibble (Q range ~0.2–2.0). Per-voice filter routing via $D417 bits 0–2; output mode (LP/BP/HP) from $D418 bits 4–6.
- [x] Volume: 4-bit master volume ($D418 bits 0–3); voice-3 disconnect from audio output ($D418 bit 7, for LFO/OSC3 read use).
- [x] Read-only: OSC3 (phase accumulator bits 23–16), ENV3 (envelope level). Paddle X/Y return 0xFF.
- [x] IAudioOutput: ring buffer (8192 samples); `tick()` synthesises samples at configured rate; `pullSamples()` drains the buffer. Clock rate configurable via `setClockHz()` (default 985248 PAL; 1022727 NTSC).
- [x] Plugin: `mmemu-plugin-sid6581.so`, device registered as `"6581"`.

### Phase 11.6 C64 Memory Map and Wiring [COMPLETED]

- [x] `MachineDescriptor` id `"c64"` — `src/plugins/machines/c64/main/machine_c64.cpp`.
- [x] `onInit`: creates and registers C64PLA ($A000), VIC2 ($D000), SID ($D400), ColorRAM ($D800), CIA1 ($DC00), CIA2 ($DD00) in IORegistry; sets bus IO hooks.
- [x] `onReset`: resets all devices via `ioRegistry->resetAll()`, then resets CPU.
- [x] 6510 banking signals wired from `MOS6510::signalLoram/Hiram/Charen()` into `C64PLA::setSignals()`.
- [x] VIC-II DMA bus = system bus; CIA2 Port A write callback updates `VIC2::setBankBase()` on bits 0–1 change.
- [x] IRQ: VIC-II raster + CIA1 → `CpuIrqLine`; NMI: CIA2 → `CpuNmiLine`.
- [x] ROM images loaded from `roms/c64/{basic,kernal,char}.bin`; PLA and VIC-II receive ROM pointers.
- [x] CIA6526 gained `setPortAWriteCallback()` for VIC bank switching.
- [x] Plugin: `mmemu-plugin-c64.so`; machine registered as `"c64"`.
- [x] GUI screen pane registered (reuses `VicDisplayPane` from VIC-20 plugin).
- [x] Integration test: 9 test cases (setup, CPU variant, RAM write, 6510 I/O port, CIA timer, PLA banking, VIC-II raster, boot screen content, and color updates). 100% tests pass.

---


## Phase 12: Commodore PET/CBM Machine [COMPLETED]

*Goal: Implement the Commodore PET series (2001, 3000, 4000, 8000), including 
the 6520 PIA, 6545 CRTC, and the unique PET memory maps.*

### Phase 12.1: MOS 6520 PIA (`src/plugins/devices/pia6520/`) [COMPLETED]

- [x] Implement `PIA6520 : public IOHandler`.
- [x] Dual 8-bit ports (Port A and Port B) with data direction registers.
- [x] Control registers (CRA, CRB) for interrupt control and DDR/Port selection.
- [x] CA1/CA2 and CB1/CB2 line handling for handshaking (used by IEEE-488).
- [x] `reset()` clears all registers and de-asserts interrupt lines.
- [x] Snapshot support for registers and line states.

### Phase 12.2: MOS 6545/6845 CRTC (`src/plugins/devices/crtc6545/`) [COMPLETED]

- [x] Implement `CRTC6545 : public IOHandler`.
- [x] Register-based interface: Address Register ($E880) and Data Register ($E881).
- [x] 18 internal registers for screen timing (H-sync, V-sync, Interlace, etc.).
- [x] Address generation for character memory and row/column counting.
- [x] Support for both 40-column and 80-column PET models via register configuration.
- [x] `tick(cycles)`: internal counters for horizontal and vertical sync pulses.

### Phase 12.3: PET Video Subsystem (`src/plugins/devices/pet_video/`) [COMPLETED]

- [x] Implement `PetVideo` inheriting from `IVideoOutput`.
- [x] **Discrete logic model (2001)**: Simulate basic timing logic used before the CRTC was introduced.
- [x] **CRTC model (4000/8000)**: Interface with `CRTC6545` for address generation.
- [x] Character ROM mapping: Support for Graphics (lower/upper) and Business 
      (lower/upper) character sets.
- [x] **Video RAM Overlay**: Map $8000–$87FF (2 KB) using `FlatMemoryBus::addOverlay`.
- [x] `renderFrame()`: produces RGBA buffer from PET character memory and 
      attributes. Supports "Green" or "Amber" phosphor simulation.

### Phase 12.4: IEEE-488 Bus Implementation (`src/libdevices/ieee488.h/cpp`) [COMPLETED]

- [x] Implement `IEEE488Bus` interface for communication with disk drives 
      and printers.
- [x] Wire PIA signals (ATN, DAV, NRFD, NDAC, EOI, SRQ, IFC, REN) to the bus.
- [x] **HLE Disk Drive (Unit 8)**: Trap IEEE-488 sequences to provide fast host-file access.
- [x] Support for PET "disk commands" (e.g., `LOAD "$",8`).

### Phase 12.5: PET Machine Factory and Memory Map [COMPLETED]

- [x] `MachineDescriptor` for `"pet2001"`, `"pet4032"`, and `"pet8032"`.
- [x] **Banking Logic**: Support for BASIC 1.0/2.0/4.0, Editor, KERNAL, and Character ROM.
- [x] **I/O Window**: PET I/O window at $E800–$EFFF containing PIAs, VIAs, and CRTC.
- [x] **Keyboard Matrix**: Implement both Graphics (2001/4000) and Business (8000) 
      keyboard matrices wired to PIA #1.
- [x] `onReset`: Load appropriate ROMs for the selected model and reset all chips.

### Phase 12.6: PET Integration Tests [COMPLETED]

- [x] **PIA Loopback**: Verify Port A to Port B communication via external 
      wiring model.
- [x] **Video Buffer**: Write to $8000 (video RAM) and verify `renderFrame` 
      detects the change.
- [x] **CRTC Configuration**: Change registers in 6545 and verify screen 
      geometry (columns/rows) updates.
- [x] **IEEE-488 Smoke Test**: Perform a mock `LISTEN/DATA/UNLISTEN` sequence 
      and verify PIA flags.

---

## Phase 13: Runtime Image and Cartridge Loading [PARTIALLY COMPLETED]

*Goal: Enable dynamic loading of .prg (RAM injection), .bin (raw memory), and
cartridge images (.crt or raw) into a running machine via CLI, GUI, and MCP.
Format-specific parsers live in machine-family plugins; `libcore` provides only
the registration mechanism and the format-agnostic BIN loader.*

### Phase 13.1: Core Loader Infrastructure (`src/libcore/main/image_loader.h/cpp`) [COMPLETED]

- [x] Define `IImageLoader` interface: `canLoad(path)`, `load(path, IBus*, machine)`.
- [x] Define `ICartridgeHandler` interface: `attach(IBus*)`, `eject(IBus*)`,
      `metadata()` (returns type string, bank count, address range).
- [x] Implement `ImageLoaderRegistry` — singleton, same pattern as `DeviceRegistry`.
- [x] **BIN Loader**: Implement `BinLoader : public IImageLoader` for raw binary
      data loading at a user-specified physical address. Register in the host binary.
- [x] CLI/GUI/MCP delegate `load`, `cart`, and `eject` commands through the
      registry without knowing any specific file format.

### Phase 13.2: CBM Loader Plugin (`src/plugins/cbm-loader/`) [COMPLETED]

*Handles all Commodore-family program and cartridge image formats. Shared by
VIC-20, C64, PET, and C128.*

- [x] Implement `PrgLoader : public IImageLoader` for Commodore `.prg` files
      (2-byte little-endian load address header, data follows); inject into the
      active machine's `IBus`.
- [x] Implement `CrtParser` for the OpenC64Cart `.crt` format: file/CHIP packet
      headers, cartridge type, hardware ID, bank count.
- [x] Implement `CbmCartridgeHandler : public ICartridgeHandler` for basic 8 KB /
      16 KB ROM mapping via `IBus` overlay; drive EXROM/GAME signal lines for C64.
- [x] Register `PrgLoader` and `CbmCartridgeHandler` via `mmemuPluginInit` into
      `ImageLoaderRegistry`.
- [x] **Snapshot Integration**: Include active cartridge image path and bank state
      in machine snapshots.

### Phase 13.3: Atari Loader Plugin (`src/plugins/atari-loader/`)

*Handles Atari 8-bit program and cartridge image formats. Shared across all
Atari machine targets.*

- [ ] Implement `XexLoader : public IImageLoader` for Atari `.xex` files
      (optional $FFFF leader, segment start/end address pairs); inject each
      segment and patch RUNAD ($02E0) / INITAD ($02E2) vectors.
- [ ] Implement `CarParser` for the Atari `.car` format (16-byte header, cartridge
      type ID, banking metadata).
- [ ] Implement `AtariCartridgeHandler : public ICartridgeHandler` for `.car`
      ROM mapping at the correct Atari address window.
- [ ] Register via `mmemuPluginInit` into `ImageLoaderRegistry`.

### Phase 13.4: CLI Interface Extensions [COMPLETED]

- [x] `load <filename> [address]`: Delegates to `ImageLoaderRegistry`; uses header
      for `.prg` / `.xex`, requires address for `.bin`.
- [x] `cart <filename>`: Attaches a cartridge image via `ICartridgeHandler` and
      triggers a machine reset if necessary.
- [x] `run`: Sets the PC to the start address of the last loaded image and
      resumes execution.
- [x] `eject`: Calls `ICartridgeHandler::eject()` and restores the machine's
      default memory mapping.

### Phase 13.5: GUI Image Management [COMPLETED]

- [x] **Load Image Dialog**: File picker with optional load address override and
      a "Run after load" checkbox.
- [x] **Cartridge Pane**: Displays currently attached cartridge metadata (type,
      bank count, address range) sourced from `ICartridgeHandler::metadata()`.
- [x] **Drag-and-Drop**: Dragging `.prg`/`.xex` or `.crt`/`.car` into the
      machine display window triggers immediate loading or attachment.
- [x] **File History**: Recently loaded images list for quick reattachment.

### Phase 13.6: MCP Integration [COMPLETED]

- [x] `load_image` tool: Accepts `path`, `address`, and `autoStart` boolean;
      returns the final load address and size.
- [x] `attach_cartridge` tool: Accepts `path` and `reset` boolean; returns
      cartridge metadata from `ICartridgeHandler::metadata()`.
- [x] `eject_cartridge` tool: Calls `ICartridgeHandler::eject()`.

### Phase 13.7: Integration Tests [PARTIALLY COMPLETED]

- [x] **Registry Test**: Verify `ImageLoaderRegistry` correctly dispatches to the
      right `IImageLoader` based on file extension.
- [x] **BIN Test**: Load a raw binary to a specific address; verify memory content
      via `IBus::peek8()`.
- [x] **CBM PRG Test** (cbm-loader plugin): Load a `.prg`; verify data lands at
      the address given in the 2-byte header.
- [x] **CBM Cartridge Overlay Test** (cbm-loader plugin): Attach a `.crt`;
      verify reads at $8000–$9FFF return cartridge data.
- [ ] **Atari XEX Test** (atari-loader plugin): Load a multi-segment `.xex`;
      verify each segment is written to its range and RUNAD is patched.
- [ ] **Atari Cartridge Overlay Test** (atari-loader plugin): Attach a `.car`;
      verify header parsing and ROM visible at the correct address window.
- [x] **Eject Test**: Detach a cartridge; verify RAM is visible again at the
      previous cartridge range.

---

## Phase 14: Tape (Datasette) Support

*Goal: Implement the Commodore Datasette (tape) interface, supporting the .tap 
pulse-encoded format for VIC-20, C64, and PET.*

### Phase 14.1: .tap Archive Parser (`src/plugins/cbm-loader/`)

- [x] Implement `TapArchive` class to parse "C64-TAPE-RAW" headers.
- [x] Decoder for pulse timings (converting .tap byte values to CPU cycles).
- [x] Support for both Version 0 and Version 1 .tap files.
- [x] Co-located with other CBM format parsers in the `cbm-loader` plugin.

### Phase 14.2: Datasette Device (`src/plugins/devices/datasette/`)

- [x] Implement `Datasette : public ISignalLine` (for the tape pulse output).
- [x] Sense line (connected to CPU port bash1 bit 4 on C64): indicates if a button 
      is pressed.
- [x] Motor control: responds to motor signal from CPU port.
- [ ] Write pulse capture (for saving to tape).
- [x] Internal timer/state machine to "play" the pulses into the CPU's IRQ/FLAG 
      lines at the correct cycle-exact intervals.

### Phase 14.3: Per-Machine Wiring

- [x] **C64**: Wire Datasette to CPU Port bash1 (Sense/Motor) and CIA #1 FLAG 
      line (Pulses).
- [ ] **VIC-20**: Wire to VIA #1.
- [ ] **PET**: Wire to PIA #1.

### Phase 14.4: Tape UI and Controls

- [x] **CLI**: `tape mount <file>`, `tape play`, `tape stop`, `tape rewind`.
- [x] **GUI**: "Tape Control" pane with tape counter, play/stop/rewind buttons, 
      signal and status LED.
- [x] **MCP**: `mount_tape`, `control_tape` tools.

---

## Phase 15: IEC Serial Bus and Disk Drive Support

*Goal: Implement the Commodore Serial Bus (IEC) and High-Level Emulation (HLE) 
for disk access (.d64, .g64, .p00). See .plan/iec.md.*

### Phase 15.1: KERNAL HLE (Level 1)

- [ ] Implement `KernalTrap` ExecutionObserver to monitor PC for KERNAL entry 
      points (, , ).
- [ ] Support for "Flat Directory" mapping (local host folders as disks).
- [ ] Fast-inject file data into RAM, bypassing bit-banging protocol.

### Phase 15.2: Virtual IEC Device (Level 2)

- [ ] Implement `VirtualIECBus` state machine.
- [ ] Handle ATN/CLK/DATA handshaking signals via `IPortDevice` on CIA #2.
- [ ] Stream bits from host-side .d64 or .prg files.

### Phase 15.3: Disk Image Support (`src/plugins/cbm-loader/`)

*CBM-specific disk image formats co-located with other Commodore format parsers
in the `cbm-loader` plugin.*

- [ ] `.d64` parser (sector/track mapping for 1541).
- [ ] `.g64` parser (GCR-encoded pulses for copy-protected images).
- [ ] `.t64` (tape image formatted as disk) support.

### Phase 15.4: Disk UI and Controls

- [ ] **CLI**: `disk mount <unit> <file>`, `disk eject <unit>`.
- [ ] **GUI**: "Drive Status" pane showing active track/sector and activity LED.
- [ ] **MCP**: `mount_disk`, `eject_disk` tools.

## Phase 15.5: Atari SIO and Disk Image Support

*Goal: Implement the Atari Serial I/O (SIO) bus and disk image support (.atr, .xfd).*

### Phase 15.5.1: Atari Disk Image Parsers (`src/plugins/atari-loader/`)

*Atari-specific disk image formats co-located with other Atari format parsers
in the `atari-loader` plugin.*

- [ ] **ATR Parser**: Implement a parser for Atari `.atr` disk images (16-byte
      header followed by raw sectors).
- [ ] **XFD Loader**: Support for headerless raw sector images (`.xfd`).
- [ ] **Sector Translation**: Map 128-byte (SD) and 256-byte (DD) sectors
      to Atari-specific track layouts.

### Phase 15.5.2: SIO HLE (Level 1)

- [ ] Implement `SioTrap` ExecutionObserver to monitor OS ROM for SIO entry 
      points ($E459).
- [ ] **Fast Disk Access**: Trap sector read/write requests and satisfy them 
      directly from host-side disk images, bypassing the serial protocol.

### Phase 15.5.3: Virtual SIO Device (Level 2)

- [ ] Implement `VirtualSIOBus` state machine.
- [ ] Handle DATA, COMMAND, PROCEED, and INTERRUPT lines via POKEY/PIA 
      emulation.
- [ ] Stream bits from host-side images into the POKEY serial registers.

## Phase 16: Commodore 128 Machine

*Goal: Implement the dual-CPU Commodore 128, including the 8502 and Z80 CPUs, the 
complex MMU for 128KB banking, and the 80-column VDC.*

### Phase 16.1: MOS 8502 CPU (`src/plugins/6502/main/cpu8502.h/cpp`)

- [ ] Subclass `MOS6510`.
- [ ] Implement **Fast Mode (2 MHz)**:
    - Add `m_isFast` internal state.
    - When bit 0 of the MMU configuration is set (via `ISignalLine`), double the 
      internal cycle count logic.
    - `step()` returns 1x or 2x cycles based on the fast mode state.
- [ ] **I/O port at $00/$01**:
    - Same as 6510, but ensure 2 MHz timing compatibility.
    - Wire `sigLoram`, `sigHiram`, `sigCharen` to the C128 MMU.
- [ ] Snapshot includes `m_isFast`, `m_ioDdr`, and `m_ioData`.

### Phase 16.2: Z80 CPU Core (`src/libcore/cpu_z80.h/cpp`)

- [ ] Implement `Z80` class inheriting from `ICore`.
- [ ] **Registers**: A, F, B, C, D, E, H, L, SP, PC, IX, IY, I, R, plus shadow 
      sets (A', F', etc.).
- [ ] **Instruction Decoder**: Complete table for official Z80 opcodes and 
      prefixes (CB, DD, ED, FD).
- [ ] **Disassembler**: Implement `disassembleOne` and `disassembleEntry` 
      specifically for Z80 syntax.
- [ ] `setDataBus`/`setIoBus`: Support Z80's separate IO address space 
      (though C128 maps IO to the memory bus).
- [ ] `step()`: Fetch-decode-execute one Z80 instruction.

### Phase 16.3: C128 MMU (8722) (`src/plugins/devices/c128_mmu/main/c128_mmu.h/cpp`)

- [ ] Implement `C128MMU : public IOHandler` at $D500–$D50B.
- [ ] **Configuration Register (CR)**:
    - Bank selection (Bank 0 / Bank 1) for CPU accesses.
    - ROM banking: BASIC, Editor, KERNAL visibility.
    - I/O window enable ($D000–$DFFF).
- [ ] **Page 0/1 Relocation**:
    - 8-bit registers for Page 0 and Page 1 high-byte pointers.
    - CPU reads/writes to $00xx/$01xx are redirected to the mapped page.
- [ ] **RAM Configuration (RCR)**:
    - Shared RAM settings (0, 1, 4, 16 KB at top/bottom).
- [ ] **Mode Configuration**:
    - Handle C128/C64 mode switch.
    - Handle CPU selection: drive Z80/8502 `HALT` lines.
- [ ] `reset()` defaults to Z80 active, Bank 0, ROMs enabled.

### Phase 16.4: MOS 8563 VDC (80-column) (`src/plugins/devices/vdc8563/main/vdc.h/cpp`)

- [ ] Implement `VDC8563 : public IOHandler` at $D600–$D601.
- [ ] **Register Interface**: Address Register ($D600) and Data Register ($D601).
- [ ] **VDC RAM**: Allocate 16 KB or 64 KB of dedicated video memory (not in 
      CPU bus).
- [ ] **VDC DMA**: Support for copying blocks between CPU RAM and VDC RAM.
- [ ] **Text Mode**: 80x25 character rendering with attributes (blink, 
      underline, reverse, color).
- [ ] `renderFrame()`: produces an RGBA buffer for the 80-column display.

### Phase 16.5: C128 Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"c128"`.
- [ ] **Bus Configuration**: `FlatMemoryBus` with two 64 KB RAM overlays 
      (Bank 0, Bank 1).
- [ ] **ROM Loading**:
    - C128 KERNAL ($E000), Editor ($C000), BASIC ($4000).
    - Z80 BIOS (mapped to $0000 at reset).
- [ ] **Peripheral Wiring**:
    - VIC-II ($D000), SID ($D400), CIA1 ($DC00), CIA2 ($DD00).
    - C128 Keyboard: Extend C64 matrix with extra keys (Numeric, ESC, TAB, etc.).
- [ ] `onReset`: Start Z80, load ROMs, set MMU to default.

### Phase 16.6: C128 Integration Tests

- [ ] **CPU Handover**: Write to MMU register to swap from Z80 to 8502; verify 
      execution continues on 8502.
- [ ] **RAM Banking**: Write to $2000 in Bank 0, switch to Bank 1, verify $2000 
      reads different data.
- [ ] **Page 0 Move**: Relocate Page 0 to $1000; verify `STA $00` hits physical 
      $1000.
- [ ] **VDC Block Copy**: Use VDC registers to copy data to VDC RAM and verify 
      via VDC read-back.
- [ ] **Fast Mode Timing**: Verify `cycles()` increments twice as fast when 
      FAST bit is set.

## Phase 17: MOS 65CE02 CPU

*Goal: An intermediate CPU that bridges 6502 (Phase 2) and 45GS02 (Phase 16).
The 65CE02 introduces the Z index register, the B (base page) register, 16-bit
stack, new addressing modes, and extended branch instructions.*

### Phase 17.1 MOS 65CE02 Core (`src/libcore/cpu65ce02.h/cpp`)

- [ ] Subclass `MOS6502`; add internal state: `Z` (8-bit index), `B` (base-page
      high byte, default $00), `SPH` (stack pointer high byte, making SP 16-bit).
- [ ] **Emulation/Native flag**: add `E` bit to status register. `E=1` means
      6502-compatible (emulation) mode; `E=0` means native 65CE02 mode. Reset
      starts in emulation mode (`E=1`).
- [ ] **Base page**: all zero-page accesses use `(B << 8) | zp_addr` as the
      effective address; B defaults to $00, making page 0 the default base page.
- [ ] **Register descriptor table**: extend 6502 table with entries for Z, B,
      SPH (flagged `REGFLAG_INTERNAL`), and E (status bit pseudo-register).
- [ ] **New addressing modes** (implement in the decode/execute path):
    - Base-page indirect Z-indexed: `(zp),z` — dereferences 16-bit pointer in
      base page, adds Z; used for the standard 65CE02 indirect mode.
    - Stack-pointer indirect Y-indexed: `(d,sp),y` — pointer at stack-relative
      offset, then add Y.
    - Absolute indirect Y-indexed: `(abs),y`.
- [ ] **New transfer instructions**: `TAZ`/`TZA` (A↔Z), `TAB`/`TBA` (A↔B),
      `TSY`/`TYS` (SP↔Y), `TYS` copies Y into SP high byte.
- [ ] **Push/pop word**: `PHW #imm16` (push 16-bit immediate), `PHW abs`
      (push 16-bit from memory). Stack is 16-bit (SP wraps within $0100–$01FF
      in emulation mode; full 16-bit in native mode).
- [ ] **Long branches**: `LBRA rel16`, `LBCC rel16`, `LBCS rel16`, `LBEQ rel16`,
      `LBNE rel16`, `LBMI rel16`, `LBPL rel16`, `LBVC rel16`, `LBVS rel16` —
      16-bit signed PC-relative offset.
- [ ] **16-bit memory operations**: `ASW abs` (arithmetic shift word left),
      `ROW abs` (rotate word left), `DEW zp` (decrement 16-bit word at base
      page), `INW zp` (increment 16-bit word at base page).
- [ ] **Emulation-mode control**: `CLE` (clear E, enter native mode),
      `SEE` (set E, return to emulation mode).
- [ ] **`step()`**: dispatch new opcodes; all 6502 opcodes retain original
      behaviour; new opcodes only active in their respective modes.
- [ ] **`isCallAt()`**: recognise `JSR abs` ($20) and the new `BSR rel16` ($63)
      as call sites.
- [ ] **`isReturnAt()`**: recognise `RTS` ($60), `RTI` ($40), `RTN #imm8`
      ($62 — return and remove N stack bytes).
- [ ] Snapshot: `saveState/loadState` includes Z, B, SPH, E in the POD blob.

### Phase 17.2 65CE02 Plugin (`src/plugins/65ce02/`)

- [ ] Create `src/plugins/65ce02/` mirroring the 6502 plugin structure.
- [ ] Implement `mmemuPluginInit` exposing `MOS65CE02` as a core and an updated
      `Disassembler65CE02` that covers all new opcodes and addressing modes.
- [ ] `Makefile` target: `lib/mmemu-plugin-65ce02.so`.
- [ ] `KickAssemblerBackend` already targets 65CE02 via `.cpu _65ce02`; no
      assembler changes needed.

### Phase 17.3 Unit Tests (`tests/test_cpu65ce02.cpp`)

- [ ] `TAZ`/`TZA`: verify Z register loaded from / stored to A.
- [ ] `TAB`: relocate base page to $10 (B=$10); subsequent zero-page read hits
      physical address $1000+offset.
- [ ] `PHW #imm16`: push word, verify SP decremented by 2 and bytes in stack.
- [ ] `LBNE rel16`: branch taken with 16-bit offset; branch not taken.
- [ ] `INW zp`: verify 16-bit word in base page increments correctly across
      byte boundary (low byte $FF → $00, carry into high byte).
- [ ] `(zp),z` addressing: store a 16-bit pointer in base page, set Z, load
      byte at effective address; verify correct value returned.
- [ ] `CLE`/`SEE`: verify E flag toggles in status register.
- [ ] Snapshot round-trip includes Z and B registers.

---

## Phase 18: 45GS02 CPU

*Goal: The full MEGA65 CPU, extending 65CE02 with 32-bit Quad operations, the
MAP instruction for 28-bit memory access, variable speed, math acceleration
registers, and Hypervisor mode. See `ref/CBM/Mega65/mega65-book.txt` Appendix K.*

### Phase 18.1 45GS02 Core (`src/libcore/cpu45gs02.h/cpp`)

- [ ] Subclass `MOS65CE02`.
- [ ] **On-chip I/O port ($00/$01)**: the 45GS02, like the 6510 before it,
      integrates a 6-bit I/O port directly into the CPU silicon. Reads and
      writes to addresses $00 and $01 are intercepted inside `read8`/`write8`
      **before** the address reaches the bus; no bus cycle is generated for them.
    - $00 (DDR): data-direction register. Bit=1 → pin is output; bit=0 → pin
      is input (reads back the external pull-up value, modelled as 1).
    - $01 (DATA): port data register.
      - **Read**: returns `(DDR & m_ioData) | (~DDR & 0x3F)` — output bits
        return the last written value; input bits float high (all 1s).
      - **Write**: stores to `m_ioData`; only DDR-configured output bits drive
        the external pins.
    - Pin assignments (identical to the C64/6510 port for software compatibility):
        - Bit 0 (LORAM): BASIC ROM visibility signal (output).
        - Bit 1 (HIRAM): KERNAL ROM visibility signal (output).
        - Bit 2 (CHAREN): I/O vs. CHAR ROM select signal (output).
        - Bit 3: Cassette motor control (output; not connected in MEGA65 hw).
        - Bit 4: Cassette button sense (input; floats high when no cassette).
        - Bit 5: Cassette data output (output; not connected in MEGA65 hw).
    - The CPU exposes three `ISignalLine*` outputs — `sigLoram`, `sigHiram`,
      `sigCharen` — that are driven on every write to $01. The `MapMmu`
      (Phase 19.2) subscribes to these lines to update ROM/IO overlay state,
      exactly as the C64 PLA does in Phase 11.2.
    - Power-on default: DDR=$00 (all inputs), DATA=$37 ($00111111) so that
      after the KERNAL sets DDR=$2F ($00101111) the banking lines present
      LORAM=HIRAM=CHAREN=1 (KERNAL+BASIC ROM visible, I/O active).
    - Snapshot: `m_ioDdr` and `m_ioData` are included in the POD blob.
- [ ] **Quad (Q) pseudo-register**: Q = {Z[7:0], Y[7:0], X[7:0], A[7:0]} as a
      virtual 32-bit register. Not stored separately — assembled on demand from
      the four 8-bit registers.
- [ ] **MAP state**: internal struct `MapState` holding the eight 8KB offset
      values (four for $0000–$7FFF, four for $8000–$FFFF) and enable bits. Used
      by the MAP MMU IOHandler (Phase 19.2) to translate virtual→physical.
- [ ] **MAP instruction** (`$5C`): loads the MAP state from A/X/Y/Z registers
      according to the 45GS02 encoding:
    - `LDA offset_lo; LDX offset_hi; LDY upper_lo; LDZ upper_hi; MAP`
    - Disables IRQ between MAP and EOM (NMI still accepted).
    - After MAP, CPU notifies the MAP MMU IOHandler to reconfigure its offset
      table; the IOHandler then adjusts `IBus` overlay regions accordingly.
- [ ] **EOM instruction** (`$EA` with prefix — opcode $02): End of Map; re-enables
      interrupts and commits the new MAP state.
- [ ] **Quad load/store**: `LDQ zp`, `LDQ abs`, `LDQ (zp),z`, `STQ zp`,
      `STQ abs`, `STQ (zp),z` — moves 4 bytes between Q and memory in
      little-endian order.
- [ ] **Quad arithmetic**: `ADCQ`, `SBCQ`, `ANDQ`, `ORQ`, `EORQ`, `CMPQ` —
      operate on the full Q register (32-bit A+X+Y+Z combined).
- [ ] **Quad shifts**: `ASLQ`, `LSRQ`, `ROLQ`, `RORQ` — 32-bit shift/rotate.
- [ ] **32-bit flat indirect**: `NOP` prefix ($EA) before `LDA (zp),z` or
      `STA (zp),z` activates 32-bit indirect mode: the base-page pointer is
      read as 4 bytes (28-bit physical address), Z is added as a byte offset,
      and the access bypasses MAP translation, going directly to the physical
      28-bit address. KickAssembler syntax: `lda ((zp)),z`.
- [ ] **Variable speed register** (`$D031` bit 4 = FAST): when the CPU reads
      this register via `IORegistry`, it adjusts an internal `m_speed` flag:
      - FAST=0 → 1 MHz / 3.5 MHz (PAL/NTSC); FAST=1 → 40 MHz (full speed).
      - In simulation, speed is modelled as a `uint32_t cyclesPerStep` scalar
        exposed via `regRead`; the host can use it for throttling.
- [ ] **Math acceleration registers** (stub as an `IOHandler` at $D770–$D77F,
      Phase 20 provides the full device; stub returns 0 for reads):
    - $D770–$D771: MULTINA (16-bit multiplicand A, written lo then hi).
    - $D772–$D773: MULTINB (16-bit multiplicand B).
    - $D774–$D777: MULTOUT (32-bit product, read-only, available next cycle).
    - $D778–$D77B: DIVINA (32-bit dividend).
    - $D77C–$D77D: DIVINB (16-bit divisor).
    - $D77E–$D77F: DIVOUT quotient (16-bit) and remainder (16-bit).
- [ ] **Hypervisor mode** (stub): entering hypervisor is triggered by a write
      to $D640–$D67F. The stub records the trap number and sets a
      `m_inHypervisor` flag; `step()` returns `ICORE_HALTED` until the host
      clears the flag via a new `exitHypervisor()` method. Full hypervisor
      virtualisation is out of scope for this phase.
- [ ] **`disassembleOne()` / `disassembleEntry()`**: extended to cover all
      45GS02 opcodes including Quad variants, MAP/EOM, and 32-bit flat indirect.
      Uses KickAssembler double-paren syntax `(( ))` for 32-bit indirect.
- [ ] **Register descriptor table**: extends 65CE02 table; adds Q (32-bit,
      `REGFLAG_PSEUDO`), MAP enable bitmask (`REGFLAG_INTERNAL`), speed scalar.
- [ ] Snapshot: POD blob extended with MAP state and speed flag.

### Phase 18.2 45GS02 Plugin (`src/plugins/45gs02/`)

- [ ] Create `src/plugins/45gs02/` plugin; `mmemuPluginInit` exposes
      `CPU45GS02` core and `Disassembler45GS02`.
- [ ] `Makefile` target: `lib/mmemu-plugin-45gs02.so`.
- [ ] `KickAssemblerBackend` selects `.cpu _45gs02` when this plugin's ISA is
      active; no other assembler changes needed.

### Phase 18.3 Unit Tests (`tests/test_cpu45gs02.cpp`)

- [ ] Quad load/store round-trip: `STQ` to memory, `LDQ` back, verify all four
      registers match.
- [ ] `ASLQ`: shift Q left by 1; verify 33rd bit carry propagation.
- [ ] `ADCQ`: 32-bit addition with carry in/out; verify sum and C flag.
- [ ] MAP instruction: write MAP state selecting bank 4 ($40000) for $A000–$BFFF
      region; verify that a subsequent read at $A000 via the MAP MMU returns
      data from physical $40000 rather than $A000.
- [ ] 32-bit flat indirect: store a 28-bit physical pointer in base page; use
      `lda ((ptr)),z` syntax (NOP prefix + `lda (ptr),z`); verify data from
      the remote physical address is returned.
- [ ] Speed register: write FAST bit to $D031; verify `regRead` speed scalar
      changes.
- [ ] **I/O port DDR**: write $2F to $00 (DDR); verify bits 0–2 and 5 are now
      outputs; write $37 to $01; read $01 back; verify bits 0–2 read $07
      (output values) and bit 4 reads 1 (input, floating high).
- [ ] **I/O port input float**: with DDR=$00 (all inputs), read $01; verify
      all six bits read as 1 (pull-up model).
- [ ] **I/O port signal lines**: write DDR=$07, DATA=$00 to $00/$01; verify
      `sigLoram->get()`, `sigHiram->get()`, `sigCharen->get()` all return false.
      Write DATA=$07; verify all three signals return true.
- [ ] **I/O port bus bypass**: confirm that a write to $00 does not appear in
      the bus write log (i.e., `IBus::writeCount()` does not increment).
- [ ] **I/O port snapshot**: write DDR=$2F, DATA=$37; save state; write
      DDR=$00, DATA=$00; restore state; verify DDR and DATA are $2F/$37.
- [ ] Hypervisor trap: write to $D640; verify `step()` returns `ICORE_HALTED`
      and `m_inHypervisor` is set.
- [ ] Snapshot round-trip: run 50 steps with a non-default MAP state; save;
      run 50 more; restore; verify MAP state and Q registers match the save.

---

## Phase 19: SparseMemoryBus(28) and MAP MMU

*Goal: A 28-bit sparse bus suitable for the MEGA65's 256 MB address space, plus
the MAP MMU IOHandler that translates 16-bit CPU virtual addresses to 28-bit
physical addresses. See arch.md §3.1 and the `SparseMemoryBus(28)` note.*

### Phase 19.1 `SparseMemoryBus` (`src/libmem/sparse_memory_bus.h/cpp`)

- [ ] Template (or configurable) `SparseMemoryBus` class implementing `IBus`.
- [ ] Internally uses a `std::unordered_map<uint32_t, uint8_t*>` of 4 KB pages,
      allocated lazily on first write (reads to unallocated pages return $FF).
- [ ] Constructor takes `addrBits` (28 for MEGA65; also used for 65C816 at 24).
- [ ] `addRegion(base28, size, data, writable)` — maps a pre-allocated buffer
      (e.g., ROM image) into the sparse space; reads/writes redirect to `data`.
      Writable=false silently drops writes (ROM protection).
- [ ] `read8(addr28)` / `write8(addr28, val)`: page lookup → buffer offset.
      Write log appended on every `write8`.
- [ ] `peek8(addr28)`: side-effect-free read, same as `read8` but does not
      trigger write-watch callbacks.
- [ ] `stateSize/saveState/loadState`: serialises only allocated pages (sparse
      snapshot); restores by re-allocating and populating those pages.
- [ ] `reset()`: deallocates all dynamically allocated pages; pre-mapped regions
      (ROM buffers) remain mapped but intact.

### Phase 19.2 MAP MMU IOHandler (`src/plugins/devices/map_mmu/`)

*The MAP MMU sits between the CPU's 16-bit address space and the 28-bit
`SparseMemoryBus`. It intercepts every CPU read/write and translates.*

- [ ] `MapMmu` implements `IOHandler` (responds to the full $0000–$FFFF range)
      but is wired as a *pass-through* bus adapter rather than a device:
    - Internally holds a reference to the downstream `SparseMemoryBus*`.
    - The CPU's `IBus*` slot points to the `MapMmu` wrapper, not directly to
      `SparseMemoryBus`; `MapMmu::ioRead/ioWrite` performs the translation.
- [ ] **MAP state**: eight 20-bit offsets (four for $0000–$7FFF, four for
      $8000–$FFFF, each covering an 8 KB block) plus a corresponding enable
      bitmask. Offsets are set by `CPU45GS02` calling `setMapState()`.
- [ ] **Translation**: for each CPU 16-bit address:
    - Determine which 8 KB block it falls in (bits 15:13 → index 0–7).
    - If that block's enable bit is set: `phys = (offset[i] << 8) | (vaddr & 0x1FFF)`.
    - If not enabled: `phys = (c64_bank << 16) | vaddr` (C64-style banking,
      upper bank byte from the 6510-compatible port at $00/$01).
- [ ] **ROM overlay fast path**: C64-compatibility banking (LORAM/HIRAM/CHAREN)
      is handled by a `C64BankController` (similar to Phase 11.2 C64 PLA) that
      intercepts writes to $00/$01 and updates `SparseMemoryBus` overlays for
      KERNAL ($E000–$FFFF), BASIC ($A000–$BFFF), and I/O/CHARROM ($D000–$DFFF).
- [ ] **I/O personality**: writes to $D02F (KEY register) cycle through
      C64 → C65 → MEGA65 personality; the active personality controls which
      device handlers are visible in the $D000–$DFFF window.
- [ ] `setMapState(const MapState&)` — called by `CPU45GS02::step()` when it
      executes MAP; stores new offsets and enable bits atomically.
- [ ] `reset()`: clears all MAP enables; C64 banking lines default to $37
      (LORAM=HIRAM=CHAREN=1 — KERNAL+BASIC visible, I/O active).

### Phase 19.3 Unit Tests (`tests/test_sparse_memory_bus.cpp`)

- [ ] Read from unallocated page returns $FF without crash.
- [ ] Write to page allocates it; subsequent read returns the written value.
- [ ] `addRegion()` with writable=false: write is silently dropped; read returns
      region data.
- [ ] Two non-overlapping regions at $20000 and $40000; verify each returns
      its own data independently.
- [ ] Snapshot: allocate two pages, write distinct values, save, overwrite,
      restore; verify original values recovered; unallocated page still $FF.
- [ ] MAP MMU translation: enable MAP for $A000–$BFFF block pointing to $40000;
      CPU read at $A000 reads from physical $40000; CPU read at $8000 (not
      mapped) reads from physical $8000.
- [ ] C64 bank switch: set HIRAM=0; verify $E000 no longer reads KERNAL ROM
      overlay (reads underlying RAM).

---

## Phase 20: MEGA65 Peripheral Chips

*Goal: VIC-IV, dual SID, F018B DMA controller, and math acceleration registers
as concrete `IOHandler` implementations.*

### Phase 20.1 VIC-IV (`src/plugins/devices/vic4/`)

*Extends VIC-II (Phase 11.4). The VIC-IV is backward-compatible at $D000–$D02E;
new registers are unlocked by the I/O personality at $D030 and above.*

- [ ] Inherits or wraps `VIC2`; re-uses sprite engine and VIC-II register file.
- [ ] **$D02F (KEY)**: personality unlock register. Write $47 ('G') then $53
      ('S') to enable MEGA65 extended registers; write any other value to lock.
      Managed by `MapMmu` but exposed to `VIC4` via a callback so the VIC-IV
      can enable its extended register window.
- [ ] **$D030 (VIC-III banking)**: ROM control bits (CRAM2K, ROM-B), V400 enable.
- [ ] **$D031 (VIC-III control)**: H640 (80-column mode), V400, FAST CPU bit
      (forwards to `CPU45GS02` speed scalar), PAL/NTSC select.
- [ ] **$D040–$D07F (VIC-IV extended registers)**:
    - $D040–$D043: character X/Y pixel position (16-bit each).
    - $D044: `CHRCOUNT` (character columns in current row).
    - $D045: extended control (H1280 super-wide, 16-bit char mode enable).
    - $D046: horizontal character count (used for 40/80-column switching).
    - $D047: vertical character count.
    - $D048–$D049: left/right border pixel positions.
    - $D04A–$D04B: top/bottom border raster positions (11-bit).
    - $D04C–$D04F: screen RAM base address (32-bit, 28-bit physical).
    - $D050–$D053: character set base address (32-bit).
    - $D054: sprite enable bits (extended, 8 sprites from $D015 + FCM flag).
    - $D055–$D057: additional sprite control (16-colour enable per sprite).
    - $D058–$D05B: colour RAM base address (32-bit; default $FF80000).
    - $D05C: sprite pointer table address (relative to screen RAM base).
    - $D05D: raster compare high bit (bit 10 of 11-bit raster counter).
    - $D05E: horizontal smooth scroll (extended, 4-bit).
    - $D05F: vertical smooth scroll (extended, 4-bit).
- [ ] **$D100–$D3FF (colour palette)**: 256 entries × 3 bytes (R, G, B in
      separate $D100/$D200/$D300 arrays). Default power-on palette matches the
      16-colour MEGA65 system palette (compatible with C64 colours 0–15).
- [ ] **Video modes** (controlled by $D031/$D054/$D045):
    - Standard 40-column text (VIC-II compatible).
    - 80-column text (H640 bit).
    - Multicolour text and bitmap (VIC-II compatible).
    - Full Colour Mode (FCM): each character cell uses 64 bytes of character
      data for a 8×8 full-colour glyph; screen RAM holds 16-bit character
      pointers. `renderFrame()` fetches from 28-bit character base address via
      `IBus::peek8()` on the `SparseMemoryBus`.
- [ ] **Raster counter**: 11-bit (0–2047), set via $D012 (bits 7:0) and $D05D
      (bit 10); compare fires IRQ via `ISignalLine`.
- [ ] **32 KB colour RAM** at 28-bit base $FF80000 (configurable via $D058–$D05B).
      Allocated as a pre-mapped `SparseMemoryBus` region; also mirrored at
      $D800–$DBFF (first 1 KB) for C64 compatibility.
- [ ] `renderFrame()`: implements the MEGA65 raster pipeline for the active
      video mode; fills an RGBA pixel buffer using register state and bus data.
      For FCM and 80-column modes, accesses the 28-bit address space.
- [ ] `tick(cycles)`: drives raster counter; fires raster IRQ.
- [ ] `ChipRegDescriptor` table: covers all $D000–$D07F registers and palette.

### Phase 20.2 F018B DMA Controller (`src/plugins/devices/dma_f018b/`)

*The MEGA65's DMA controller is an enhanced version of the C65 F018 chip. It
supports 28-bit source/destination addresses, chained job lists, and four
operations: copy, fill, swap, and mix.*

- [ ] **Register file** (at $D700–$D70F in MEGA65 personality):
    - $D700: DMA list address low byte (written to trigger job execution).
    - $D701: DMA list address high byte.
    - $D702: DMA list address bank byte (bits 19:16 of 28-bit list pointer).
    - $D703: DMA execute register — writing triggers DMA using the current
             list address; a write to $D700 also triggers (C65 compatibility).
    - $D704: DMA upper address byte (bits 27:20 of 28-bit list pointer).
    - $D705: Enhanced DMA options: modulus mode, slope, fractional stepping.
    - $D706–$D70F: reserved / enhanced per-job override bytes.
- [ ] **Job list format** (F018B enhanced):
    - Byte 0 (command): bits 1:0 = operation (0=copy, 1=fill, 2=swap, 3=mix);
      bit 2 = chain (1 = another job follows); bit 3 = interrupt on completion;
      bit 6 = enhanced format (extra header bytes follow).
    - Enhanced prefix (if bit 6 set): additional bytes for source/dest bank,
      stepping, and modulus override.
    - Bytes 1–2: count (16-bit, number of bytes).
    - Bytes 3–5: source address (24-bit; bank from $D704/$D702).
    - Bytes 6–8: destination address (24-bit).
    - Byte 9: chain/end byte (0 = end of list, 4 = next job follows).
- [ ] **Execution**: `ioWrite($D703, any)` fetches the job list from the 28-bit
      pointer via `SparseMemoryBus::read8()`; processes all chained jobs in a
      single `tick()` call (DMA freezes the CPU — set `m_dmaActive` flag which
      `CPU45GS02::step()` checks and spins on until cleared).
- [ ] **Operations**:
    - Copy: `src[i]` → `dst[i]` for `count` bytes; source/dest may overlap
      (copy direction chosen by address relationship).
    - Fill: `dst[i] = fill_byte` (fill_byte is the low byte of the source
      address field in fill mode).
    - Swap: atomically exchanges `src[i]` and `dst[i]` (read-modify-write).
- [ ] `reset()`: clears list address and active flag.
- [ ] `ChipRegDescriptor` table covering $D700–$D70F.

### Phase 20.3 Math Acceleration Registers (`src/plugins/devices/math_accel/`)

*Hardware multiply and divide completing in a single CPU cycle. Exposed as an
`IOHandler` at $D770–$D77F.*

- [ ] **Multiply**: write 16-bit MULTINA ($D770–$D771) and MULTINB ($D772–$D773);
      result appears immediately in MULTOUT ($D774–$D777, 32-bit read-only).
- [ ] **Divide**: write 32-bit DIVINA ($D778–$D77B) and 16-bit DIVINB ($D77C–$D77D);
      DIVOUT quotient ($D77E–$D77F) and remainder ($D77C–$D77D read-back) are
      available on the very next read (model as synchronous — no latency).
- [ ] Division by zero: set quotient and remainder to $FFFF (hardware behaviour).
- [ ] `ioRead` / `ioWrite` implement the register semantics; no `tick()` logic
      required (combinatorial hardware model).

### Phase 20.4 Dual SID (`src/plugins/devices/sid_pair/`)

- [ ] Thin wrapper that instantiates two `SID6581` objects (Phase 11.5) at
      $D400–$D41F (SID1) and $D420–$D43F (SID2).
- [ ] Stereo mix: SID1 output mixed right-biased, SID2 output mixed left-biased
      (MEGA65 default panning). Mix weights configurable via constructor.
- [ ] `IOHandler` spans $D400–$D43F; dispatch to SID1 or SID2 by address bit 5.
- [ ] `reset()`: resets both SIDs.
- [ ] `tick(cycles)`: ticks both SIDs.

### Phase 20.5 Unit Tests (`tests/test_mega65_chips.cpp`)

- [ ] **VIC-IV**: unlock extended registers via KEY; write $D04C–$D04F with a
      non-standard 28-bit screen address; verify `renderFrame()` fetches
      character codes from that address (use a mock `SparseMemoryBus` pre-seeded
      with known data).
- [ ] **VIC-IV palette**: write 3 bytes to $D100/$D200/$D300 for entry 1; read
      back each byte; verify values match.
- [ ] **VIC-IV raster**: tick past raster compare; verify IRQ signal fired.
- [ ] **DMA copy**: set up a known source region at $10000; trigger a copy DMA
      to $20000 for 256 bytes; verify destination matches source.
- [ ] **DMA fill**: trigger fill DMA at $30000 for 128 bytes with fill value
      $42; verify all 128 bytes are $42.
- [ ] **DMA chain**: two-job chain (fill then copy); verify both jobs executed
      in sequence in one trigger call.
- [ ] **Math multiply**: write MULTINA=$0100, MULTINB=$0100; read MULTOUT;
      verify result = $00010000 (256 × 256).
- [ ] **Math divide**: write DIVINA=$00010000, DIVINB=$0100; verify quotient
      = $0100, remainder = $0000.
- [ ] **Math divide by zero**: DIVINB=0; verify quotient = $FFFF.
- [ ] **Dual SID dispatch**: write to $D404 (SID1 voice 1 control) and $D424
      (SID2 voice 1 control) independently; verify both `SID6581` objects
      receive separate writes without crosstalk.

---

## Phase 21: MEGA65 Memory Map and Machine Factory

*Goal: A bootable MEGA65 simulation with the full 28-bit address space wired,
all devices registered, and I/O personality switching functional.*

### Phase 21.1 MEGA65 28-bit Memory Map

```
28-bit Range      Bank(s)  Description
$0000000–$00FFFFF  0–15    Chip RAM (16 × 64 KB = 1 MB)
  $000000–$0FFFF    0      Bank 0: zero page, stack, KERNAL vars, user RAM
  $010000–$01FFFF   1      Bank 1: DOS buffers, BASIC arrays
  $020000–$03FFFF   2–3    ROM area (MEGA65.ROM); write-protected at boot
  $040000–$05FFFF   4–5    Expansion chip RAM (128 KB for graphics/data)
  $060000–$0FFFFF   6–15   Additional chip RAM
$4000000–$7FFFFFF   —      Slow bus / cartridge port (unmapped by default)
$8000000–$87FFFFF   —      Attic RAM (8 MB; modelled as sparse pages)
$FF80000–$FF87FFF   —      Colour RAM (32 KB dedicated)
$FFD0000–$FFDFFFF   —      Direct I/O (28-bit access to VIC-IV, SID, DMA)
```

**16-bit CPU view (Bank 0 default, I/O personality MEGA65):**
```
$0000–$00FF   Base page (default; relocatable via B register)
$0100–$01FF   Stack (default; 16-bit SP in native mode)
$0200–$D7FF   User RAM (Bank 0 chip RAM)
$D000–$D02E   VIC-IV registers (VIC-II compatible)
$D02F         KEY register (personality unlock)
$D030–$D031   VIC-III banking + control
$D040–$D07F   VIC-IV extended registers (when unlocked)
$D100–$D3FF   Colour palette (R/G/B arrays, when unlocked)
$D400–$D41F   SID1
$D420–$D43F   SID2
$D600–$D6FF   UART, SD card controller, RTC (stub IOHandler)
$D700–$D70F   DMA controller (F018B)
$D770–$D77F   Math acceleration registers
$D800–$DBFF   Colour RAM mirror (first 1 KB of $FF80000)
$DC00–$DCFF   CIA1
$DD00–$DDFF   CIA2
$DE00–$DFFF   Cartridge I/O (stub)
$E000–$FFFF   KERNAL ROM (from MEGA65.ROM; overlay in Banks 2–3)
```

### Phase 21.2 Machine Factory (`src/libcore/machines/machine_mega65.cpp`)

- [ ] `SparseMemoryBus(28)` as the single system bus (`"system"`, 28-bit).
- [ ] `MapMmu` wired between `CPU45GS02` and `SparseMemoryBus`; CPU's `IBus*`
      slot set to `MapMmu`; `MapMmu` holds downstream `SparseMemoryBus*`.
- [ ] Load `MEGA65.ROM` (nominally 768 KB) via `romLoad()` at $20000 in the
      `SparseMemoryBus`; `addRegion(0x20000, romSize, data, writable=false)`.
- [ ] KERNAL (last 8 KB of ROM) also mirrored at $E000–$FFFF via a second
      `addRegion` call for the legacy 16-bit CPU view (MapMmu handles the
      translation transparently).
- [ ] `IORegistry` populated in MEGA65 personality order:
    - VIC-IV ($D000, masks to $D3FF).
    - SID pair ($D400, masks to $D43F).
    - UART/SD stub ($D600, 256 bytes — stub returning $FF for reads).
    - DMA F018B ($D700, masks to $D70F).
    - Math accelerator ($D770, masks to $D77F).
    - Colour RAM mirror ($D800, 1 KB, backed by $FF80000 region).
    - CIA1 ($DC00, 256 bytes).
    - CIA2 ($DD00, 256 bytes).
    - Cartridge I/O stub ($DE00, 512 bytes).
- [ ] `MachineDescriptor` id `"mega65"`:
    - Single CPU slot: `CPU45GS02`.
    - Single bus slot: `SparseMemoryBus(28)` named `"system"`.
    - `onInit`: `romLoad()` for MEGA65.ROM; populate `IORegistry`; wire CIA1
      port A/B to keyboard matrix; wire CIA2 port A bits 0–1 for VIC-IV bank
      select; connect DMA active flag to CPU stall signal.
    - `onReset`: reset MAP state (all offsets clear, C64 bank = $37); reset
      CPU; reset all IOHandler devices; reset KEY register to C64 personality.
- [ ] Register factory: `MachineRegistry::registerMachine("mega65", ...)`.

### Phase 21.3 I/O Personality Switching

- [ ] `MapMmu` tracks the current personality: `C64`, `C65`, or `MEGA65`.
- [ ] On personality change, `MapMmu` reconfigures which `IOHandler` entries
      are active in the $D000–$DFFF window:
    - C64: VIC-II registers only ($D000–$D02E); SID at $D400; CIA1/CIA2.
    - C65: adds VIC-III $D030/$D031 visibility; hides math/DMA.
    - MEGA65: full register map as above.
- [ ] Personality change does not affect RAM or ROM overlays — only which
      device handlers dispatch in the I/O window.
- [ ] `$D02F` write sequence (G=$47, S=$53) advances personality; any other
      write resets to C64 personality.

---

## Phase 22: MEGA65 Keyboard

*Goal: Wire the MEGA65 keyboard (C65-derived matrix) to CIA1.*

### Phase 22.1 C65 Keyboard Matrix (`src/plugins/devices/keyboard/main/keyboard_matrix_c65.h/cpp`)

- [ ] 8×8 matrix (64 positions), wired to CIA1 port A (column output) and
      CIA1 port B (row input) — same electrical topology as the C64 but with
      a different key layout.
- [ ] `keyDown(row, col)` / `keyUp(row, col)` API.
- [ ] Implements `IPortDevice` on both CIA1 ports; VIA reads return active-low
      row bits for the currently scanned column.
- [ ] **Restore key**: wired directly to CIA1 NMI line (same as C64); a
      dedicated `ISignalLine` for NMI is pulsed on Restore press.
- [ ] **CAPS LOCK LED**: `ISignalLine` output from the keyboard matrix to
      the host GUI for visual feedback. Host sets the LED state.
- [ ] Key mapping constants: expose a `MEGA65_KEY_*` enum for all physical keys
      including the extra MEGA65-specific keys (Mega key, NO SCROLL, ALT).

### Phase 22.2 Joystick Ports

- [ ] Two joystick ports wired to CIA1 port B (joystick 2) and CIA2 port A
      (joystick 1), active-low — identical to the C64 wiring. Reuse the
      `Joystick` class from Phase 10.4.

---

## Phase 23: MEGA65 Integration Tests

*Goal: Smoke tests that exercise the complete machine stack end-to-end.*

- [ ] **MAP test**: assemble a tiny program that executes `MAP` to redirect
      $A000–$BFFF to bank 4 ($40000); writes a sentinel byte at $A000;
      verifies via `SparseMemoryBus::peek8($40000)` that the byte appears at
      the physical address.
- [ ] **DMA fill test**: trigger a DMA fill of 256 bytes at $2000 with $55;
      verify via bus peek that all 256 bytes are $55; confirm CPU was stalled
      (DMA active flag was set then cleared).
- [ ] **VIC-IV raster test**: tick enough cycles to advance at least two raster
      lines; read raster counter via `IORegistry::dispatchRead($D012)`;
      verify it advanced.
- [ ] **Dual SID write test**: write distinct frequency values to SID1 ($D400)
      and SID2 ($D420) voice 1; read them back; confirm values are independent.
- [ ] **Math accelerator test**: write MULTINA=200 ($00C8), MULTINB=200 ($00C8)
      via `IORegistry::dispatchWrite`; read MULTOUT; verify = 40000 ($9C40).
- [ ] **Personality switch test**: start in C64 personality; write G/S to $D02F
      to advance to MEGA65 personality; write to $D04C (VIC-IV extended reg);
      verify the write reaches VIC-IV. Lock personality (write 0 to $D02F);
      verify $D04C write no longer dispatches to VIC-IV.
- [ ] **C64 bank switch test**: set CPU port $01 = $35 (HIRAM=0, KERNAL
      hidden); read $FFFC; verify it returns RAM content, not KERNAL vector.
- [ ] **Extension host verification**: verify `machineId == "mega65"` for pane
      filtering (Phase 10.5 `PluginPaneManager`).

---

## Phase 24: MEGA65 ROM Importer Plugin (`src/plugins/mega65Importer/`)

*Goal: Help users obtain the MEGA65.ROM file from a local xemu installation or
MEGA65 SD card image. Follows the structure of the Phase 10.7 VICE importer.*

### Phase 24.1 Plugin Identity and Wiring

- [ ] Plugin entry point `mmemuPluginInit` in
      `src/plugins/mega65Importer/main/plugin_main.cpp`; manifest declares:
    - `pluginId = "mega65-importer"`, `displayName = "MEGA65 ROM Importer"`.
    - `supportedMachineIds[] = { "mega65" }`.
- [ ] Registers CLI command, GUI pane, and MCP tool via `SimPluginHostAPI`.
- [ ] `Makefile` target: `lib/mmemu-plugin-mega65-importer.so`.

### Phase 24.2 Discovery Engine (`src/plugins/mega65Importer/main/rom_discovery.h/cpp`)

- [ ] `discoverSources("mega65")` searches well-known paths:
    - `~/.local/share/xemu/mega65/` (Linux xemu default).
    - `~/Library/Application Support/xemu/mega65/` (macOS xemu).
    - `%APPDATA%\xemu\mega65\` (Windows xemu).
    - `./roms/mega65/` (local override directory).
    - SD card images: scan for `MEGA65.ROM` in the root of any `.img` file
      found in those directories (read-only, does not mount the image —
      scans for the known 8-byte ROM header signature at offset 0).
- [ ] `romFilesFor("mega65")` returns a single spec:
      `{ srcRelPath: "MEGA65.ROM", destName: "mega65.rom", expectedSize: 786432 }`.
      (768 KB = $C0000; actual size validated against this expected value.)
- [ ] Returns empty vector if ROM file not found or size mismatch; never throws.

### Phase 24.3 Import Operation (`src/plugins/mega65Importer/main/rom_importer.h/cpp`)

- [ ] Reuses `ImportResult` / `importRoms()` structure from Phase 10.7.
- [ ] Validates the 8-byte ROM header signature at offset 0 after copy.
- [ ] Never overwrites existing file unless `overwrite=true`.
- [ ] Rolls back on size or signature mismatch.

### Phase 24.4 CLI, GUI, and MCP Integration

- [ ] **CLI**: `importroms mega65 [--list] [--source <n>] [--dest <path>] [--overwrite]`
      — identical argument semantics to the VICE importer command.
- [ ] **GUI pane**: `RomImportPane` with source dropdown, file list, Import
      button; displays ROM header version string from the ROM file if discovered.
      Pane hidden by `PluginPaneManager` when non-MEGA65 machine is active.
- [ ] **MCP tool**: `"import_roms"` with `machineId`, `sourceIndex`, `overwrite`
      parameters; returns JSON with success status and copied file list.

### Phase 24.5 Unit Tests (`tests/test_mega65_importer.cpp`)

- [ ] `discoverSourcesInPaths("mega65", paths)` with a temp-dir mock: file
      present at correct size → source found; wrong size → excluded; missing →
      not included.
- [ ] `importRoms()` copies file to temp dir; validates size and header signature.
- [ ] `importRoms()` with deliberate size mismatch rolls back and returns error.
- [ ] `importRoms()` does not overwrite existing file unless `overwrite=true`.

---

## Phase 25: Commodore Plus/4 and C16 (TED-based)

*Goal: Implement the TED-based Commodore machines (C16, C116, Plus/4).*

### Phase 25.1: MOS 7360 / 8360 TED (`src/plugins/devices/ted7360/`)

- [ ] Implement `TED7360 : public IOHandler`.
- [ ] **Register File**: 64 registers ($FF00–$FF3F) controlling video, sound, 
      timers, and memory banking.
- [ ] **Internal Timers**:
    - Three 16-bit interval timers (Timer 1, 2, 3).
    - Timer 3 also drives the cursor blink and system clock.
- [ ] **Banking Logic**:
    - Integrated ROM/RAM banking control (BASIC, KERNAL, Function ROMs).
    - `ISignalLine` outputs for LORAM/HIRAM equivalents.
- [ ] **Sound**:
    - Two square wave channels (10-bit frequency).
    - Channel 2 can be switched to digital noise.
    - Master volume control.
- [ ] **I/O**:
    - Keyboard matrix scanning (shared with video registers).
    - Joystick port reading.

### Phase 25.2: TED Video and Palette (`src/plugins/devices/ted_video/`)

- [ ] Implement `TedVideo` inheriting from `IVideoOutput`.
- [ ] **121-Color Palette**:
    - 15 hues (plus black) and 8 luminance levels.
    - Implement the luminance scaling logic in the RGBA renderer.
- [ ] **Video Modes**:
    - Standard 40-column text.
    - Multicolor text.
    - Multicolor bitmap.
    - Extended background color.
- [ ] **Raster Pipeline**:
    - Raster counter and compare (triggering IRQ).
    - Horizontal and vertical smooth scrolling.
- [ ] `renderFrame()`: produces RGBA buffer using TED registers and memory.

### Phase 25.3: MOS 6551 ACIA (`src/plugins/devices/acia6551/`)

- [ ] Implement `ACIA6551 : public IOHandler` at $FD00.
- [ ] **Registers**: Control, Command, Status, and Data.
- [ ] **Baud Rate Generator**: Support for standard rates (50 to 19200 baud).
- [ ] **Interrupts**: IRQ on transmit buffer empty or receive buffer full.

### Phase 25.4: Plus/4 Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"plus4"` and `"c16"`.
- [ ] **Memory Map**:
    - C16: 16 KB RAM ($0000–$3FFF).
    - Plus/4: 64 KB RAM ($0000–$FDFF).
    - ROMs: BASIC ($8000), KERNAL ($E000).
- [ ] **Keyboard**: Implement the 8x8 matrix scanned via TED registers.
- [ ] `onReset`: Load ROMs, reset TED and ACIA.

### Phase 25.5: Plus/4 Integration Tests

- [ ] **TED Color Test**: Cycle through all 121 colors and verify RGBA output.
- [ ] **TED Timer Test**: Program Timer 1 for IRQ and verify `sigIrq` pulses.
- [ ] **TED Banking Test**: Switch between RAM and ROM at $8000 via TED 
      registers; verify data change.
- [ ] **ACIA Loopback Test**: Connect TX to RX and verify data round-trip.

---

## Phase 26: Atari 8-bit Family (400/800/XL/XE) [PARTIALLY COMPLETED]

*Goal: Cycle-accurate emulation of the ANTIC, GTIA, and POKEY trio.*

- [x] **ROM Collection**: System ROMs (OS-A, OS-B, XL, BASIC, CHAR) collected in `roms/atari/`.
- [x] **ANTIC DMA Engine** (`src/plugins/devices/antic/`)
- [x] **Display List Interpreter**:
    - [x] Fetch-decode-execute display list instructions.
    - [x] Support for basic modes (mapped in code).
    - [x] Handle `DLI` (Display List Interrupt) bit in instructions.
    - [ ] Handle `HSCROL` and `VSCROL` smooth scrolling.
- [x] **DMA Timing**:
    - [x] "Steal" cycles from the CPU for playfield, sprite, and display list fetches.
    - [x] Implement the `HALT` signal line back to `ICore`.
- [x] **NMI Generation**: Vertical Blank Interrupt (VBI) and Display List 
      Interrupt (DLI).

### Phase 26.2: GTIA Color and PMG (`src/plugins/devices/gtia/`) [COMPLETED]

- [x] Implement `GTIA : public IOHandler`.
- [x] **Color Palette**:
    - [x] 256 colors (16 hues x 16 luminances).
    - [x] Map GTIA color registers ($D012–$D01A) to RGBA.
- [x] **Player-Missile Graphics (PMG)**:
    - [x] 4 Players and 4 Missiles (acting as a 5th player).
    - [x] Priority logic structure (ready for renderer).
    - [x] Collision detection registers ($D000–$D00F).
- [x] **Console Switches**: Read Start, Select, Option buttons via $D01F.

### Phase 26.3: POKEY Audio and IO (`src/plugins/devices/pokey/`) [COMPLETED]

- [x] Implement `POKEY : public IOHandler`.
- [x] **Audio Subsystem**:
    - [x] 4 independent square-wave channels.
    - [x] Polynomial-counter noise generators (4, 5, 9, 17-bit).
    - [x] Frequency division and 16-bit channel linking.
- [x] **Timers**: 3 interval timers with IRQ support.
- [x] **Keyboard**:
    - [x] Serial scan matrix for the Atari keyboard (skeleton).
    - [x] Handle BREAK and Control keys (skeleton).
- [x] **Serial Port (SIO)**: Support for SIO bus interrupts and basic data transfer (skeleton).

### Phase 26.4: Atari Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"atari800"`, `"atari800xl"`, and `"atari65xe"`.
- [ ] **PIA 6520**:
    - Port A: Joystick ports (4 for Atari 800, 2 for XL/XE).
    - Port B: XL/XE memory banking (OS ROM, BASIC ROM, RAM expansion).
- [ ] **Memory Map**:
    - ROMs: OS ($C000–$FFFF), BASIC ($A000–$BFFF).
    - I/O: ANTIC ($D400), GTIA ($D000), POKEY ($D200), PIA ($D300).
- [ ] **Video Renderer**: `IVideoOutput` combining ANTIC playfield and GTIA PMG 
      with priority and collisions.

### Phase 26.5: Atari Integration Tests

- [ ] **Display List Test**: Load a simple display list and verify ANTIC 
      addresses generated via `IBus` log.
- [ ] **PMG Priority Test**: Place a Player over a Playfield and verify RGBA 
      pixel priority matches GTIA settings.
- [ ] **POKEY Audio Test**: Program a channel and verify `pullSamples` returns 
      the expected waveform.
- [ ] **XL/XE Banking Test**: Toggle PIA Port B bit 0 and verify OS ROM 
      visibility at $C000.

---

## Phase 27: Atari 2600 (VCS)

*Goal: Ultra-lean scanline-based architecture requiring perfect timing.*

### Phase 27.1: TIA Video and Audio (`src/plugins/devices/tia/`)

- [ ] Implement `TIA : public IOHandler`.
- [ ] **Scanline Engine**:
    - No frame buffer; logic must render a single scanline pixel-by-pixel.
    - `WSYNC` logic: halt CPU until the end of the current scanline.
    - **Sprites**: Player 0, Player 1, Missile 0, Missile 1, Ball.
    - **Playfield**: 20-bit register-based playfield (mirrored or repeated).
    - Collision registers: Hardware-level pixel collision bit-latches.
- [ ] **Audio Subsystem**:
    - 2 independent channels (square/noise).
    - 5-bit frequency, 4-bit volume, 4-bit waveform.
- [ ] **Input**: Paddle and joystick button latching.

### Phase 27.2: RIOT 6532 (`src/plugins/devices/riot6532/`)

- [ ] Implement `RIOT6532 : public IOHandler`.
- [ ] **RAM**: 128 bytes of built-in RAM.
- [ ] **I/O Ports**: Dual 8-bit ports for joysticks and console switches.
- [ ] **Interval Timer**:
    - 8-bit counter with 1, 8, 64, or 1024 clock pulse intervals.
    - Interrupt support on underflow.

### Phase 27.3: Atari 2600 Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"vcs"`.
- [ ] **Memory Map**:
    - TIA: $00–$7F.
    - RIOT RAM: $80–$FF.
    - RIOT I/O/Timer: $280–$29F.
    - Cartridge: $F000–$FFFF (typical).
- [ ] **Banking**: Support for standard Atari bank-switching (F8, F6, etc.) 
      via `IBus` overlay.
- [ ] **Timing Model**: Cycle-exact `step()` loop; TIA must be updated every 
      3 CPU clock cycles (the 2600 has a 3:1 ratio).

### Phase 27.4: Atari 2600 Integration Tests

- [ ] **WSYNC Test**: Write to `WSYNC` and verify `cycles()` jump to next 
      scanline boundary.
- [ ] **Collision Test**: Set Player 0 and Player 1 at the same X-coord and 
      verify the P0-P1 collision bit is set.
- [ ] **RIOT Timer Test**: Set timer to 1024-count and verify it decrements 
      at the correct rate.
- [ ] **Playfield Render Test**: Program the playfield and verify the RGBA 
      buffer contains the expected pattern on a single scanline.

---

## Phase 28: Apple II Family

*Goal: Implementation of the classic Apple architecture and its unique Disk II.*

### Phase 28.1: Apple II Soft-Switches and Video (`src/plugins/devices/apple2_video/`)

- [ ] Implement `Apple2Video : public IVideoOutput, public IOHandler`.
- [ ] **Soft-Switches ($C000–$C0FF)**:
    - Many hardware functions are toggled via reads/writes to this range.
    - Video modes: Text, Lo-Res, Hi-Res.
    - Page 1/Page 2 display.
    - **Language Card**: ROM/RAM switch for $D000–$FFFF.
- [ ] **Keyboard**: Implement the latch at $C000 and strobe-clear at $C010.
- [ ] **Speaker**: Simple toggle at $C030; generate audible click via `IAudioOutput`.
- [ ] **Video Renderer**:
    - Apple II colors (NTSC artifacting model for Hi-Res).
    - Support for 40/80-column and Double-Hi-Res on IIe/IIc models.

### Phase 28.2: Disk II Controller (`src/plugins/devices/disk2/`)

- [ ] Implement `Disk2Controller : public IOHandler` at $C0E0–$C0EF.
- [ ] **State Machine**: Implement the 8-state stepper motor phases.
- [ ] **Drive Control**: SELECT, DRIVE, MOTOR ON, R/W.
- [ ] **Data Stream**:
    - Raw GCR-encoded bits from a virtual disk image (.dsk or .nib).
    - Shift-register emulation for read/write.
- [ ] **Disk Image Support**: Implement a simple `.dsk` to GCR converter.

### Phase 28.3: Apple IIe/IIc MMU (`src/plugins/devices/apple2e_mmu/`)

- [ ] Implement `Apple2eMMU : public IOHandler`.
- [ ] **Auxiliary RAM Banking**: Map a 64 KB auxiliary bank for 128 KB systems.
- [ ] Support for main/aux bank switches (80STORE, RAMRD, RAMWRT).
- [ ] **Alternative Character Set**: Support for the IIe/IIc character sets.

### Phase 28.4: Apple IIgs (16-bit) (`src/plugins/machines/apple2gs/`)

- [ ] `MachineDescriptor` for `"apple2gs"`.
- [ ] **CPU Slot**: `MOS65816` in native mode.
- [ ] **Ensoniq ES5503 DOC**:
    - 32 independent wavetable voices.
    - Dedicated audio RAM ($E10000–$E1003F).
- [ ] **Video Generation Chip (VGC)**:
    - Super Hi-Res 320x200 and 640x200.
    - Per-scanline palettes (16 colors per scanline).
- [ ] **Mega II**: Compatibility layer to provide the exact timing of a 
      IIe within the IIgs architecture.

### Phase 28.5: Apple II Integration Tests

- [ ] **Soft-Switch Test**: Toggle Hi-Res via $C057 and verify `IVideoOutput` 
      mode change.
- [ ] **Disk II Stepper Test**: Pulse the stepper phases and verify the 
      head position increments.
- [ ] **Language Card Test**: Switch $D000–$FFFF to RAM; write/read back; 
      switch back to ROM and verify data.
- [ ] **Speaker Click Test**: Rapidly toggle $C030 and verify `IAudioOutput` 
      samples.

---

## Phase 29: BBC Micro / Acorn Electron

*Goal: Fast, modular architecture with the unique Tube second-processor bus.*

### Phase 29.1: BBC Video Subsystem (`src/plugins/devices/bbc_video/`)

- [ ] Implement `BBCVideo : public IVideoOutput, public IOHandler`.
- [ ] **MC6845 CRTC**: Register-based timing generator at $FE00–$FE01.
- [ ] **Video ULA**:
    - Palette control (8 colors, including Mode 7 flashing).
    - Mode switching (Modes 0–6: 2/4/16-color tile/bitmap).
    - Clock speed switching for various modes.
- [ ] **SAA5050 Teletext**:
    - Implement the hardware character generator for Mode 7.
    - Support for Teletext attributes (double height, flash, alphanumeric/graphic).
- [ ] **Video RAM**: Dynamic mapping depending on mode (e.g., Mode 0–2 uses 20KB).

### Phase 29.2: System and User VIAs (`src/plugins/machines/bbc/`)

- [ ] Instantiate two `MOS6522` objects (Phase 10.1).
- [ ] **System VIA ($FE40)**:
    - Keyboard scanning (via Port A/B).
    - Sound latch control (SN76489).
    - Vertical sync interrupt.
- [ ] **User VIA ($FE60)**:
    - User port I/O.
    - Printer port.
    - ADC control.
- [ ] **SN76489 PSG**: 3-channel square wave + noise sound chip.

### Phase 29.3: The Tube Interface (`src/plugins/devices/bbc_tube/`)

- [ ] Implement `BBCTube : public IOHandler` at $FEE0–$FEE7.
- [ ] **Communication Protocol**: Implement the 8-bit bidirectional data 
      and status registers.
- [ ] **Second Processor Support**:
    - Connect a second `CpuSlot` (e.g., Z80, 6502, ARM) to the system via the Tube.
    - Handle bus synchronization and interrupts.

### Phase 29.4: BBC Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"bbcb"`, `"bbca"`, `"electron"`.
- [ ] **Paged ROM Mapping**:
    - 4-bit latch at $FE30 selects one of 16 ROM banks ($8000–$BFFF).
    - Implement `PagedMemoryBus` for this range.
- [ ] **Memory Map**:
    - RAM: $0000–$7FFF.
    - ROM/RAM Paged: $8000–$BFFF.
    - OS ROM: $C000–$FFFF.
    - I/O: $FC00–$FEFF.
- [ ] `onReset`: Load OS and Paged ROMs; reset VIAs and CRTC.

### Phase 29.5: BBC Integration Tests

- [ ] **Mode 7 Test**: Write Teletext codes to $7C00 and verify `renderFrame` 
      produces the correct SAA5050 glyphs.
- [ ] **VIA Latch Test**: Write to System VIA and verify SN76489 registers 
      are updated.
- [ ] **Paged ROM Test**: Toggle $FE30 and verify different ROM data 
      appears at $8000.
- [ ] **Tube Transfer Test**: Perform a simple status check/data write 
      to the Tube registers.

---

## Phase 30: NES / Famicom

*Goal: Tile-based rendering and the classic 5-channel sound.*

### Phase 30.1: Ricoh 2C02 PPU (`src/plugins/devices/ppu2c02/`)

- [ ] Implement `PPU2C02 : public IOHandler`.
- [ ] **Register Interface**: $2000–$2007 (Ctrl, Mask, Status, OAMAddr, 
      OAMData, Scroll, PPUAddr, PPUData).
- [ ] **Tile Pipeline**:
    - Background: Nametables, Attribute tables, Pattern tables.
    - Sprites: OAM (64 sprites, 8 per scanline limit).
    - Palette: 32 entries (BG/Sprite).
- [ ] **Scrolling**:
    - Implement "Loopy's" X/Y scroll latch and address logic.
    - Handle vertical and horizontal mirroring.
- [ ] **Interrupts**: VBlank NMI.

### Phase 30.2: Ricoh 2A03 APU (`src/plugins/devices/apu2a03/`)

- [ ] Implement `APU2A03 : public IOHandler`.
- [ ] **Sound Channels**:
    - 2 Pulse waves (4 duty cycles).
    - Triangle wave.
    - Noise generator.
    - **DMC (Delta Modulation Channel)**: Sample-based sound with DMA from RAM.
- [ ] **Frame Counter**: Internal 240Hz/192Hz clock for APU interrupts.

### Phase 30.3: NES Mappers (`src/libcore/nes_mappers.h/cpp`)

- [ ] Implement common mappers:
    - **NROM (0)**: Basic 16KB/32KB PRG, 8KB CHR.
    - **MMC1 (1)**: Serial bank switching for PRG and CHR.
    - **MMC3 (4)**: Scanline counter (via PPU A12 line) and fine banking.
- [ ] `NESMapper` base class with virtual `read/write` hooks for `IBus`.

### Phase 30.4: NES Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"nes"`, `"famicom"`.
- [ ] **Memory Map**:
    - RAM: $0000–$07FF.
    - PPU Registers: $2000–$2007.
    - APU/IO: $4000–$4017.
    - PRG ROM: $8000–$FFFF (mapped by Mapper).
- [ ] **Input**: Shift-register read for 8-button controllers.
- [ ] `onReset`: Reset CPU, PPU, APU, Mapper.

### Phase 30.5: NES Integration Tests

- [ ] **VBlank Test**: Run until VBlank and verify NMI fires.
- [ ] **PPU Register Test**: Write to `PPUADDR` and verify subsequent 
      `PPUDATA` read returns the correct memory byte.
- [ ] **Mapper MMC1 Test**: Perform the serial-write sequence to swap 
      PRG banks; verify data at $8000.
- [ ] **APU Tone Test**: Program a pulse channel and verify square wave 
      period in `pullSamples`.

---

## Phase 31: SNES / Super Famicom

*Goal: 16-bit 65816 core with complex tile modes and HDMA.*

### Phase 31.1: Ricoh 5A22 CPU (65816) (`src/libcore/cpu65816.h/cpp`)

- [ ] Implement `MOS65816 : public ICore`.
- [ ] **Native/Emulation Modes**:
    - Emulation: 6502-compatible ($0100–$01FF stack, 8-bit registers).
    - Native: 16-bit Accumulator (A/B), 16-bit X, Y, and SP.
- [ ] **24-bit Addressing**: Support for Bank ($K) and Direct ($D) registers.
- [ ] **Math Unit**: Support for hardware multiply/divide via $4200–$421F.
- [ ] **DMA/HDMA**:
    - 8 DMA channels for block transfers.
    - Horizontal-blank DMA (HDMA) for per-scanline register updates.

### Phase 31.2: SNES PPU1 and PPU2 (`src/plugins/devices/snes_ppus/`)

- [ ] Implement `SNES_PPUs : public IVideoOutput, public IOHandler`.
- [ ] **Tile Modes (0-7)**:
    - Mode 7: Affine transformations (matrix scaling, rotation, shearing).
    - Mosaic and Windowing effects.
- [ ] **Layering**: Up to 4 background layers with priority and transparency.
- [ ] **Color Math**: Master Brightness and color addition/subtraction.
- [ ] **Sprites (OBJ)**: 128 sprites, sizes from 8x8 up to 64x64.

### Phase 31.3: SNES APU (SPC700 + DSP) (`src/plugins/devices/snes_apu/`)

- [ ] Implement `SPC700` class inheriting from `ICore`.
- [ ] **Sony DSP**:
    - 8-channel ADPCM sample decoding.
    - Gaussian interpolation and Echo effects.
- [ ] **APU RAM**: 64 KB dedicated sound RAM.
- [ ] **Communication Ports ($2140–$2143)**: Four 8-bit ports for CPU-APU comms.

### Phase 31.4: SNES Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"snes"`.
- [ ] **Memory Map**:
    - WRAM: $7E0000–$7FFFFF (128 KB).
    - Registers: $2100–$43FF.
    - ROM (LoROM/HiROM): Banks $00–$3F and $80–$BF.
- [ ] **Input**: Support for SNES joypads (16-bit serial read).
- [ ] `onReset`: Reset 65816, PPU, APU.

### Phase 31.5: SNES Integration Tests

- [ ] **Native Mode Transition**: Execute `CLC; XCE` and verify 16-bit register 
      writes.
- [ ] **DMA Transfer Test**: Configure a DMA channel to copy from ROM to 
      WRAM; verify data.
- [ ] **PPU Mode 7 Test**: Enable Mode 7 and verify `renderFrame` applies 
      matrix transforms to background tiles.
- [ ] **APU Port Test**: Write to $2140 and verify `SPC700` reads the byte 
      from its local register view.

---

## Phase 32: Commander X16

*Goal: Modern 65C02-based machine with VERA graphics and YM2151 sound.*

### Phase 32.1: VERA (Video Embedded Retro Adapter) (`src/plugins/devices/vera/`)

- [ ] Implement `VERA : public IVideoOutput, public IOHandler`.
- [ ] **Register Interface**: $9F20–$9F2F.
- [ ] **Dual-Port VRAM**: 128 KB of internal video RAM accessible via 
      auto-incrementing data ports.
- [ ] **Video Layers**:
    - Two independent layers supporting tile, bitmap, and character modes.
    - 128 hardware sprites (up to 64x64, 4/8-bpp).
- [ ] **Palette**: 256 entries (12-bit RGB).
- [ ] **Audio Subsystem**:
    - PCM playback: 16-bit mono/stereo samples from VRAM.
    - PSG: 16 channels of programmable waveforms (pulse, sawtooth, noise).
- [ ] **SPI Controller**: Interaction with SD card via VERA registers.

### Phase 32.2: YM2151 OPM Sound (`src/plugins/devices/ym2151/`)

- [ ] Implement `YM2151 : public IOHandler` at $9F40.
- [ ] **FM Synthesis**:
    - 8 independent voices.
    - 4 operators per voice with various algorithms.
    - Precise envelope and LFO modulation.
- [ ] **Audio Output**: 2 channels (Stereo).

### Phase 32.3: X16 Machine Factory and Memory Map

- [ ] `MachineDescriptor` for `"x16"`.
- [ ] **CPU Slot**: `W65C02` (8-bit CMOS version).
- [ ] **Banking**:
    - Banked RAM: $A000–$BFFF (512 KB to 2 MB).
    - Banked ROM: $C000–$FFFF (512 KB).
    - Controlled by registers at $0000/$0001 (VIA1-based mapping).
- [ ] **Input**: PS/2 keyboard and mouse via VIA1/VIA2.
- [ ] `onReset`: Load KERNAL and BASIC ROMs; reset VERA and YM2151.

### Phase 32.4: X16 Integration Tests

- [ ] **VERA VRAM Test**: Write to VRAM via `DATA0` and verify address 
      auto-increment logic.
- [ ] **Banked RAM Test**: Switch between RAM banks 0 and 1; verify data 
      isolation at $A000.
- [ ] **YM2151 Write Test**: Write to an FM register and verify `pullSamples` 
      contains FM-modulated output.
- [ ] **VERA Sprite Test**: Enable a sprite and verify `renderFrame` RGBA 
      buffer includes the sprite data.

---

## Cross-Cutting GUI Standards

Applied to all new panes in Phase 8.

- [x] **Fixed-width fonts** for all hex and register values.
- [x] **Consistent colour palette** for change highlights (e.g., light red background for changed values).
- [x] **Side-effect-free reads** (via `peek8`) for all UI rendering.
- [x] **Asynchronous refresh**: UI updates at 30-60 Hz, not on every instruction step (too slow).
- [x] **DPI Awareness**: All panes and icons must scale correctly on High-DPI displays (macOS Retina, 4K Windows).
- [x] **Device Inspectable**: Every `IOHandler` provides a `ChipRegDescriptor` table for the Device Inspector pane.
