# Assembler Backend Architecture

mmsim supports pluggable assembler backends, allowing different assemblers to be used
for different machines or ISAs.

## Resolution Precedence

When an assembly operation is performed, the assembler is resolved using a 3-level chain:

1. **Runtime Override** (highest) — set via `config assembler <name>` in CLI, or per-dialog in GUI
2. **Machine Preferred** — from the machine JSON `"assembler"` field (e.g., `"ca45"` for MEGA65)
3. **ISA Default** (lowest) — registered via plugin `registerToolchain()` by ISA name

## Configuration

Assembler backends are configured via `config.json`, searched in order:
- `./config.json`
- `~/.local/share/mmsim/config.json`
- `/usr/local/share/mmsim/config.json`

Example (`config.json.example`):
```json
{
    "tools": {
        "assemblers": {
            "ca45": {
                "command": "ca45",
                "extraOptions": "",
                "workingDir": "/tmp"
            }
        }
    }
}
```

Fields:
- `command` — path to the assembler executable (or `java -jar ...` for JAR-based)
- `extraOptions` — additional CLI flags passed to the assembler
- `workingDir` — working directory for subprocess execution

## CLI Usage

```
config assembler          # Show current assembler and list available
config assembler ca45     # Set runtime override to ca45

asm $2000                 # Interactive line-mode assembly at $2000
> LDA #$02
> NOP
.                         # End with "."

asm file myprogram.s      # Assemble source file and load .prg into memory
```

## GUI Usage

The Assemble dialog (Ctrl+A or Tools → Assemble) includes an assembler picker dropdown.
Select "(default)" for automatic resolution, or pick a specific backend.

## Adding a New Assembler

1. **Implement `IAssembler`** (`src/libtoolchain/main/iassembler.h`):
   - `name()` — return a unique identifier (e.g., "myasm")
   - `isaSupported(isa)` — return true for supported ISA strings
   - `assemble(src, output)` — file-based assembly (required)
   - `assembleLine(line, buf, bufsz, addr)` — optional line-mode (return -1 if not supported)

2. **Register in your plugin's `mmemuPluginInit()`**:
   ```cpp
   host->registerAssemblerByName("myasm", []() -> IAssembler* {
       return new MyAssembler();
   });
   ```

3. **Add configuration** to `config.json`:
   ```json
   "myasm": { "command": "/path/to/myasm", "extraOptions": "" }
   ```

4. **Optional**: Set as machine default in machine JSON:
   ```json
   { "id": "mymachine", "assembler": "myasm", ... }
   ```

## Built-in Assemblers

| Name | ISAs | Line Mode | File Mode | Notes |
|------|------|-----------|-----------|-------|
| 6502asm | 6502, 6510 | Yes | No | Built-in single-line assembler |
| ca45 | 45GS02, 6502, 65CE02 | No | Yes | External `ca45` binary required |
