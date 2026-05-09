# MCP Server Status & Outstanding Work

## Overview

The mmemu-mcp (Model Context Protocol) server provides Claude Code and other MCP clients with programmatic access to the emulation system. It exposes debugging, machine control, and device management capabilities through a standardized interface.

**Status:** Functional with 40 tools implemented and all tests passing. Ready for extension.

---

## Current Implementation (40 Tools) ✅

### Machine Management
- `create_machine` - Instantiate a machine preset
- `list_machines` - List available machine types
- `reset_machine` - Reset CPU and memory to initial state

### Memory Operations
- `read_memory` - Hex dump memory range
- `write_memory` - Write bytes to memory
- `fill_memory` - Fill range with constant value
- `copy_memory` - Copy memory region
- `swap_memory` - Swap two memory ranges
- `search_memory` - Find byte/ASCII patterns in memory

### CPU Control & State
- `step_cpu` - Execute N instructions
- `run_cpu` - Run until breakpoint (requires timeout)
- `set_pc` - Set program counter
- `read_registers` - Dump all CPU registers

### Debugging
- `set_breakpoint` - Execution breakpoint at address
- `set_watchpoint` - Memory read/write watchpoint
- `enable_breakpoint` / `disable_breakpoint` - Toggle without deleting
- `delete_breakpoint` - Remove by ID
- `list_breakpoints` - Show all breakpoints/watchpoints with status
- `get_stack` - Dump stack with frame analysis
- `disassemble` - Disassemble instruction range

### Symbol Management
- `add_symbol` - Register label for address
- `list_symbols` - List all defined symbols
- `remove_symbol` - Delete symbol
- `clear_symbols` - Clear all symbols
- `load_symbols` - Load symbol file (.sym)

### Storage & Peripherals
- `load_image` - Load program/disk image
- `mount_disk` - Attach disk image
- `eject_disk` - Remove disk
- `mount_tape` - Attach tape image
- `control_tape` - Play/stop/rewind tape
- `record_tape` - Record to tape
- `save_tape_recording` - Export tape recording
- `attach_cartridge` - Insert cartridge
- `eject_cartridge` - Remove cartridge

### Device Control
- `get_device_info` - Query device state/properties
- `list_devices` - List attached I/O devices
- `press_key` - Simulate keyboard press
- `type_string` - Type string into keyboard buffer

### Logging & Monitoring
- `list_loggers` - Show available log channels
- `set_log_level` - Adjust verbosity for channel

### Resources
- `machine_state` resource - Snapshot of running machines and cycle counts

---

## Outstanding Work / Missing Features

### 1. Assembler Support ❌ (HIGH PRIORITY)

**Scope:** Add code assembly capability to MCP

**Context:**
- CLI has `asm` command for assembling code
- Toolchain registry supports ca45 and KickAssembler backends
- No MCP tool currently wires assembler to clients

**What needs to be done:**
- Create `asm` tool that takes source code and returns assembled .prg
- Accept assembler selection (auto-detect via file extension, or parameter)
- Parse ca45 symbol output if available
- Return assembled binary + optional symbol table
- Handle assembler errors gracefully (return error response with diagnostic)

**Files to modify:**
- `src/mcp/main/main.cpp` - Add tool handler
- `src/mcp/test/mcp_test.py` - Add test case

**Acceptance criteria:**
- `asm` tool available in tools/list
- Can assemble simple ca45 .s format code
- Can assemble KickAssembler .asm format code
- Returns JSON with `{ "prg": [bytes], "symbols": {...}, "errors": [...] }`
- MCP test passes for both assembler backends

---

### 2. Enhanced Search Navigation ❌ (MEDIUM PRIORITY)

**Scope:** Add memory search history and navigation

**Context:**
- CLI has `search`, `findnext`, `findprior` commands
- MCP has `search_memory` but no history/navigation
- Users need to search for next occurrence without re-scanning

**What needs to be done:**
- Maintain search context per machine (last pattern, position)
- Add `find_next` tool to find next occurrence
- Add `find_previous` tool to go back one
- Store search history internally (not in MCP state, local to server)

**Files to modify:**
- `src/mcp/main/main.cpp` - Add handlers + search context storage
- Enhance `search_memory` to save context

**Acceptance criteria:**
- After `search_memory`, `find_next` returns next match without respecifying pattern
- `find_previous` navigates backwards through matches
- Calling `search_memory` with different pattern resets context

---

### 3. MEGA65-Specific Features ⚠️ (MEDIUM PRIORITY)

**Scope:** Expose MEGA65 hardware features through MCP

**Context:**
- Phase 21 completed MAP instruction and Key register implementation
- No MCP tools to interact with these features
- Users cannot test MEGA65-specific behavior remotely

**What needs to be done:**
- Add `get_map_state` tool - Read current MAP configuration
- Add `set_map_state` tool - Configure address translation (read A/X/Y/Z from memory, call CPU MAP instruction)
- Add `get_personality` tool - Read current I/O personality mode
- Add `set_personality` tool - Switch I/O mode via $D02F
- Query F018B DMA status and configuration

**Files to modify:**
- `src/mcp/main/main.cpp` - Add tool handlers
- Verify IMapController interface is accessible to MCP

**Acceptance criteria:**
- Tools work with both `mega65` and `rawMega65` machines
- MAP state query returns offsets and enables masks
- Setting personality affects subsequent I/O

---

### 4. Trace Buffer Integration ⚠️ (MEDIUM PRIORITY)

**Scope:** Expose instruction/breakpoint trace functionality

**Context:**
- Debug system has trace buffer (DebugContext::traceBuffer)
- Can log instruction execution and breakpoint hits
- No MCP tool to query or analyze traces

**What needs to be done:**
- Add `get_trace_buffer` tool - Retrieve buffered trace entries
- Add `clear_trace` tool - Reset trace buffer
- Add `set_trace_filter` tool - Configure what gets traced (instructions, breakpoints, memory access)
- Return structured trace data with address, opcode, registers, cycle count

**Files to modify:**
- `src/mcp/main/main.cpp` - Add handlers
- Verify DebugContext::traceBuffer() is accessible

**Acceptance criteria:**
- `get_trace_buffer` returns array of trace entries with format: `{ "addr": 0x0100, "cycles": 42, "opcode": "LDA #$00", ... }`
- Filtering works (can request instruction-only, breakpoint-only traces)
- Trace buffer can be cleared without affecting breakpoints

---

### 5. Plugin Tool Integration ✓/⚠️ (LOW PRIORITY)

**Scope:** Verify and test plugin-provided tools

**Context:**
- PluginToolRegistry exists for plugins to register MCP tools
- Dispatch mechanism implemented
- No test coverage for plugin tools

**What needs to be done:**
- Test that plugin tools appear in `tools/list` output
- Verify plugin tool dispatch works (call a plugin tool via MCP)
- Document plugin tool API for plugin authors
- Handle plugin tool errors correctly

**Files to modify:**
- `src/mcp/test/mcp_test.py` - Add plugin tool test case
- Plugin author documentation

**Acceptance criteria:**
- MCP test verifies at least one plugin-provided tool is callable
- Plugin tools follow same error/result schema as built-in tools

---

### 6. Minor Quality Improvements (LOW PRIORITY)

**Better error messages:**
- Return more diagnostic information (e.g., assembler diagnostics for `asm` errors)
- Validate address expressions earlier with detailed error on failure

**Conditional breakpoints:**
- Expose breakpoint condition evaluation in MCP schema
- Allow setting breakpoint with `condition` parameter (e.g., "A == $42")

**Performance tools:**
- `profile_cpu` - Sample instruction execution for hotspot analysis
- `measure_region` - Measure cycle count for address range

---

## Implementation Priority & Effort Matrix

| Feature | Priority | Effort | User Impact | Recommendation |
|---------|----------|--------|-------------|-----------------|
| Assembler Support | HIGH | 4 hours | Enables code generation in Claude | **DO NEXT** |
| Search Navigation | MEDIUM | 2 hours | Better UX for memory search | Do after assembler |
| MEGA65 Features | MEDIUM | 3 hours | Unblock MEGA65 testing | Parallel with assembler |
| Trace Integration | MEDIUM | 2.5 hours | Debug visibility | Do after assembler |
| Plugin Tools Test | LOW | 1 hour | Code confidence | Include in next PR |
| Better Errors | LOW | 1.5 hours | Polish | Incremental improvement |
| Conditional BP | LOW | 2 hours | Advanced debugging | Future phase |
| Performance Tools | LOW | 3 hours | Optimization | Research phase |

---

## Recommended Next Step

**Implement Assembler Support** - This unblocks Claude Code's ability to generate and load code into MEGA65 and other machines. It requires wiring the existing toolchain assembler backends (ca45, KickAssembler) to a new MCP tool.

**Effort:** ~4 hours, 1 day of work  
**Benefit:** Full feature parity with CLI + enables remote code generation  
**Blocks:** MEGA65 feature testing (can wait for that), trace integration (independent)

---

## Test Command Reference

```bash
# Run all MCP tests
make test-mcp

# Run MCP server standalone (for manual testing)
bin/mmemu-mcp < test_input.json

# Check MCP tool schema
bin/mmemu-mcp | grep -A 5 "asm"
```

---

## Notes

- All 250 unit tests pass (including MCP symbol test)
- MCP server builds without errors
- Plugin tool registry is wired but untested
- ca45 and KickAssembler backends exist in toolchain, just need MCP wiring
