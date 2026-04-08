# Skill: Machine Management in mmemu

This skill details how to manage multiple machines and load binary images.

## Workflow

1.  **List Available Machines:** Use `list_machines` to see all supported targets (e.g., `c64`, `vic20`, `pet2001`).
2.  **Initialize a Machine:** Use `create_machine` with a valid ID to start a fresh instance.
3.  **Load Program Image:** Use `load_image` to load a `.prg` or `.bin` file into memory. Set `auto_start=true` if you want the machine to jump to the start address immediately.
4.  **Attach Cartridge:** Use `attach_cartridge` for machines like the C64 to load specialized software that uses ROM mapping.
5.  **Reset/Reboot:** Use `reset_machine` to return a machine to its power-on state while keeping it initialized.

## Multi-Machine Sessions

- You can have multiple machines running simultaneously in a single MCP session.
- Always provide the correct `machine_id` when calling any tool to ensure you're addressing the right machine instance.

## Tools Used
- `list_machines`
- `create_machine`
- `load_image`
- `attach_cartridge`
- `reset_machine`
