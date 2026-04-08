# Skill: Hardware Inspection in mmemu

This skill focuses on inspecting the hardware state and memory-mapped I/O in the simulator.

## Workflow

1.  **Locate I/O Area:** Use `read_memory` to inspect the I/O area ($D000-$DFFF on C64/VIC20, $E800-$E8FF on PET).
2.  **Monitor Registers:** Use `set_watchpoint` on critical hardware registers (e.g., $D012 for VIC-II raster line) to see when they are accessed.
3.  **Pattern Matching:** Use `search_memory` with a hex pattern to find sprite data, character data, or common hardware sequences.
4.  **IOHandler Details:** Use `list_loggers` and `set_log_level` to enable verbose logging for specific hardware (e.g., `set_log_level(target="vic2", level="debug")`) to see internal device operations.

## Common I/O Locations

- **C64:**
    - `$D000-$D02E`: VIC-II (Video)
    - `$D400-$D41C`: SID (Sound)
    - `$DC00-$DC0F`: CIA 1 (I/O, IRQ)
    - `$DD00-$DD0F`: CIA 2 (I/O, NMI)
- **VIC-20:**
    - `$9000-$900F`: VIC (Video/Sound)
    - `$9110-$912F`: VIA (I/O, IRQ)

## Tools Used
- `read_memory`
- `set_watchpoint`
- `search_memory`
- `list_loggers`
- `set_log_level`
