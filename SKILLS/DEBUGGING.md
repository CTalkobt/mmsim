# Skill: Debugging in mmemu

This skill provides strategies for using the MCP tools to debug execution in the mmemu simulator.

## Core Debugging Workflow

1.  **Inspect State:** Use `read_registers` to get the current CPU state (PC, Accumulator, Index registers, Flags).
2.  **Disassemble:** Use `disassemble` at the current PC to understand what the next instruction is.
3.  **Trace Stack:** Use `get_stack` to see the current call stack and identify where you are in a subroutine.
4.  **Set Breakpoints:** Use `set_breakpoint` at suspicious addresses to stop execution before a problem occurs.
5.  **Watch Memory:** Use `set_watchpoint` with `type="write"` on memory locations (like $D020 for C64 border color or zero-page variables) to catch unexpected modifications.
6.  **Step Execution:** Use `step_cpu` to advance instruction by instruction and observe state changes.

## Identifying Crashes

-   **PC in ROM:** If the PC is in a high address range ($E000-$FFFF on C64/VIC20), the code is likely executing KERNAL or BASIC ROM.
-   **Infinite Loops:** If `run_cpu` hits `max_steps` and the PC hasn't moved much, check for a `JMP` to the same address.
-   **Stack Overflow/Underflow:** Check `get_stack`. If the stack pointer is wrapping around or the trace looks corrupted, a subroutine may be missing an `RTS` or has mismatched `PHA`/`PLA`.

## Tools Used
- `read_registers`
- `disassemble`
- `get_stack`
- `set_breakpoint`
- `set_watchpoint`
- `step_cpu`
- `run_cpu`
