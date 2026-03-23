# CHANGELOG.md - mmsim (Multi-Machine Simulator)

All notable changes to this project will be documented in this file.

## [0.1.0-dev] - 2026-03-23

### Added
- **libmem — Address Bus Abstraction (Phase 1):**
    - Implemented `IBus` abstract interface for memory and I/O access.
    - Implemented `FlatMemoryBus` with support for heap-allocated storage, configurable address masking, and ROM overlays.
    - Added high-performance write-logging to `FlatMemoryBus` for debugger synchronization.
    - Added state snapshot/restore support to memory buses.
    - Created `tests/test_flatmembus.cpp` with comprehensive coverage for basic R/W, masking, and overlays.
- **libcore — ICore and MOS 6502 (Phase 2):**
    - Implemented `ICore` abstract CPU interface, featuring register descriptors and convenience accessors.
    - Implemented a fully functional `MOS6502` CPU core supporting all official NMOS opcodes and common undocumented instructions.
    - Implemented all 6502 addressing modes including the infamous indirect JMP bug for cycle-accurate behavior.
    - Added `MachineDescriptor`, `CpuSlot`, and `BusSlot` structures for data-driven machine composition.
    - Implemented `MachineRegistry` for dynamic registration and creation of machine presets.
    - Created `tests/test_cpu6502.cpp` to verify instruction accuracy, status flags, and state snapshots.
- **libtoolchain — Disassembler and Assembler (Phase 3):**
    - Implemented `IDisassembler` interface and `DisasmEntry` for detailed instruction metadata.
    - Implemented `Disassembler6502` with support for symbol resolution and control-flow detection.
    - Implemented `IAssembler` interface with support for configurable backend metadata.
    - Implemented `KickAssemblerBackend` for integration with external Java-based tools.
    - Implemented `Assembler6502` native mini-assembler for single-line instruction support.
    - Implemented `SymbolTable` and `SourceMap` for mapping between machine code and source labels/locations.
    - Added `BaseDisassembler` for default delegation to CPU core disassembly.
    - Created `tests/test_disasm6502.cpp` to verify disassembly and symbol resolution.
- **libdebug — Debug Infrastructure (Phase 4):**
    - Implemented `ExecutionObserver` interface for monitoring CPU execution and memory events.
    - Implemented `BreakpointList` supporting EXEC, READ_WATCH, and WRITE_WATCH breakpoints.
    - Implemented `TraceBuffer` for cycle-accurate execution history.
    - Implemented `DebugContext` for managing debug sessions, breakpoints, and system-wide snapshots.
    - Added support for diffing memory states between snapshots.
    - Created `tests/test_debug.cpp` to verify breakpoint logic, tracing, and snapshot integrity.
- **CLI Target Implementation (Phase 5):**
    - Implemented interactive REPL with command history and parsing.
    - Added support for dynamic machine creation and selection.
    - Implemented core debugging commands: `step`, `regs`, `m` (memory dump), `f` (memory fill), `disasm`.
    - Added direct CPU instruction execution using the `.` prefix (e.g., `.lda #$42`).
- **Utility & Performance:**
    - Implemented a generic `CircularBuffer<T>` template for high-performance fixed-capacity logging.
    - Refactored `FlatMemoryBus` write-log and `TraceBuffer` execution log to use `CircularBuffer`, eliminating $O(N)$ removal overhead.
- **Project Infrastructure:**
    - Unified the test suite into a single `mmemu-tests` binary using `tests/test_main.cpp`.
    - Added `tests/test_harness.h` for simple, lightweight unit testing.
- **Project Foundation (Phase 0):**
    - Created directory skeleton for core libraries: `src/libmem/`, `src/libcore/`, `src/libdevices/`, `src/libtoolchain/`, and `src/libdebug/`.
    - Created `roms/` and `tests/` directories.
    - Added `.gitkeep` and `.gitignore` files to ensure proper tracking of empty directories and ignored ROM files.
- **Library Build System:**
    - Extensively updated `Makefile` to support modular builds.
    - Added targets for building static archives (`make libs`) and running tests (`make test`).
    - Configured binaries (`mmemu-cli`, `mmemu-mcp`, `mmemu-gui`) to link against internal libraries.
- **ROM Loader Utility:**
    - Implemented `romLoad` in `src/libcore/rom_loader.h/cpp` for validated binary loading.
- **Documentation:**
    - Created `GEMINI.md` for project overview and quick-start instructions.
    - Created `STYLEGUIDE.md` to establish project-wide coding conventions.

### Changed
- **Directory Reorganization:**
    - Moved global `include/` directory to `src/include/` to centralize all source-related files.
    - Updated `Makefile` and `GEMINI.md` to reflect the new header search paths.
- **Style Alignment:**
    - Updated all architectural specifications (`.plan/arch.md`) and roadmaps (`.plan/todo.md`) to use `camelCase` for variable and function names as per the new style guide.
    - Refactored placeholder headers in `src/` to follow `camelCase` and `PascalCase` conventions.
    - Renamed several identifiers in internal stubs for consistency.

### Fixed
- Fixed build system to correctly handle library object dependencies and include paths.
- Fixed a bug in the 6502 `Mode::REL` implementation where branch targets were calculated incorrectly.
