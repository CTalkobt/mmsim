# CBM KERNAL HLE Plugin

The `cbm-hle` plugin provides High-Level Emulation (HLE) for Commodore KERNAL disk operations. It intercepts calls to the KERNAL jump table and satisfies them directly using the host's filesystem, bypassing the serial bus protocol.

## Features

- **Instant Load**: Intercepts `LOAD` ($FFD5) and injects file data directly into RAM.
- **Flat Directory Mapping**: Maps a host directory to the virtual disk drive.
- **Bypass Protocol**: No bit-banging required; works even if no disk drive is emulated.

## Intercepted Vectors

- `$FFD5` (LOAD): Loads a file from the host filesystem.
- `$FFD8` (SAVE): (Reserved for future implementation).

## Configuration

The HLE can be configured via the CLI using the `hle` command:

- `hle on`: Enable KERNAL HLE.
- `hle off`: Disable KERNAL HLE.
- `hle path <path>`: Set the host directory to use for file access.

## Implementation Details

The plugin implements the `ExecutionObserver` interface and registers itself with the host. It monitors the Program Counter (PC) for the entry points of specific KERNAL routines.

When a routine is intercepted:
1. It reads KERNAL variables from standard RAM locations ($BA, $B9, $B7, $BB/$BC).
2. It performs the requested operation using host-side I/O.
3. It updates the CPU registers (A, X, Y) and status byte ($90) to simulate success or failure.
4. It manually performs an `RTS` by pulling the return address from the stack and updating the PC.
5. It returns `false` to the CPU core to abort the execution of the original ROM routine.

## Consistency with Virtual IEC

The KERNAL HLE operates at **Level 1 (API-level)**, while the Virtual IEC device operates at **Level 2 (Protocol-level)**.

- If HLE is **enabled** and the device number matches, the HLE trap triggers first and the operation completes instantly.
- If HLE is **disabled**, or if the device number does not match any host mapping, execution continues into the KERNAL ROM, which will then use the serial bus protocol to communicate with any attached devices (including the `VirtualIECBus` device).
