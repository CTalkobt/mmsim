# Skill: Program Injection in mmemu

This skill details how to inject and run custom assembly code via MCP.

## Workflow

1.  **Select Target Machine:** Use `list_machines` to find an appropriate machine (e.g., `c64`).
2.  **Ensure Safe Memory:** Choose an address for the code that doesn't conflict with current ROM/RAM usage. On C64, `$C000-$CFFF` (4096-53247) is a common free RAM area.
3.  **Inject Code:** Use `write_memory` with an array of bytes.
4.  **Point the PC:** Use `set_pc` to set the PC to the start of the injected code.
5.  **Execute:** Use `run_cpu` with a reasonable `max_steps` to run the code.
6.  **Verify:** Use `read_registers` or `read_memory` to check the results.

## Example (C64/VIC20)

Injecting a simple infinite loop (`$4C $00 $C0` which is `JMP $C000`):
- `machine_id`: "c64"
- `addr`: 49152 (0xC000)
- `bytes`: [76, 0, 192]

Setting the PC and running:
1. `set_pc(machine_id="c64", addr=49152)`
2. `run_cpu(machine_id="c64", max_steps=1000)`

## Tools Used
- `write_memory`
- `set_pc`
- `run_cpu`
- `read_memory`
- `read_registers`
