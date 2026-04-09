# MCP Server: Roadmap & Ideal LLM Skills

This document outlines the current limitations of the mmemu Model Context Protocol (MCP) server and identifies "ideal" LLM skills that would be possible with future enhancements to the C++ MCP implementation.

## Current Limitations

While the MCP server provides essential tools for memory manipulation, CPU control, and machine management, it currently lacks several advanced features that would significantly enhance an LLM's autonomy and debugging efficiency.

### 1. Lack of Symbolic Awareness
- **Current State:** All MCP tools require raw hex addresses (e.g., `read_memory(addr=49152)`).
- **Missing Tool:** `load_symbols`, `resolve_symbol`, `get_symbols`.
- **Ideal Skill:** **`SYMBOLIC_DEBUGGING.md`**. An LLM could set breakpoints on `CHROUT` or watch a `game_score` variable without needing to manually look up addresses in a `.lst` or `.sym` file.

### 2. No Visual Feedback (Headless only)
- **Current State:** The LLM can only "see" memory content, not the final rendered pixels or sprite positions.
- **Missing Tool:** `capture_screenshot`, `get_frame_buffer`.
- **Ideal Skill:** **`VISUAL_VALIDATION.md`**. An LLM could run code and visually verify that a sprite is at the correct coordinates or that the color palette is correct by analyzing a returned image resource.

### 3. Inaccessible Execution History
- **Current State:** Only the *current* state (PC, registers) is visible.
- **Missing Tool:** `get_trace_buffer`, `search_trace`.
- **Ideal Skill:** **`TIME_TRAVEL_DEBUGGING.md`**. An LLM could inspect the last 100 instructions executed *before* a crash, allowing it to diagnose the root cause of an infinite loop or a stack overflow after it has already occurred.

### 4. High Latency for Complex Assertions
- **Current State:** To perform a "Run Until" operation, the LLM must iterate multiple times between `run_cpu` and `read_memory`.
- **Missing Tool:** `run_until(condition)`, `assert_memory(addr, value)`.
- **Ideal Skill:** **`AUTOMATED_TESTING.md`**. An LLM could perform atomic tests: "Run until register A is $05, then stop and verify that the screen buffer is clear." This would be significantly faster than manual stepping.

### 5. No Integrated Assembly
- **Current State:** The LLM must manually convert 6502 mnemonics to byte arrays for `write_memory`.
- **Missing Tool:** `assemble(source, addr)`, `inject_code(mnemonic, addr)`.
- **Ideal Skill:** **`HOT_RELOAD_DEVELOPMENT.md`**. An LLM could write assembly code directly and have the MCP server compile and inject it into the running machine state in a single step.

### 6. Single-Bus Memory Access
- **Current State:** `read_memory` and `write_memory` operate on the machine's default bus.
- **Missing Tool:** Bus selection parameters for all memory tools (e.g., `bus="vram"`, `bus="io"`).
- **Ideal Skill:** **`MULTI_BUS_INSPECTION.md`**. This would allow for proper inspection of Harvard architectures where code and data reside in separate address spaces, or for viewing "shadow" memory hidden behind I/O banks.

---

## Implementation Strategy

To unlock these skills, the `src/mcp/main/main.cpp` server should be expanded to bridge existing logic in `libdebug` (TraceBuffer), `libtoolchain` (Assembler/Symbols), and `libdevices` (Video capture) into the JSON-RPC interface.
