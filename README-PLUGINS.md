# mmemu Plugin Index

This document provides a categorized index of all dynamically loadable plugins available in the mmemu ecosystem.

---

## 1. Processor Plugins (CPU)
Plugins that implement the `ICore` interface, providing fetch-decode-execute logic and register sets.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-6502.so` | MOS 6502 (NMOS) with illegal opcode support; also provides MOS 6510. | [README-6502.md](README-6502.md) · [README-6510.md](README-6510.md) |

---

## 2. Device Plugins (I/O & Video)
Plugins that implement the `IOHandler` interface for memory-mapped hardware.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-via6522.so` | MOS 6522 Versatile Interface Adapter (VIA). | [README-6522.md](README-6522.md) |
| `lib/mmemu-plugin-vic6560.so` | MOS 6560 VIC-I Video/Sound generator. | [README-6560.md](README-6560.md) |
| `lib/mmemu-plugin-kbd-vic20.so` | Standard VIC-20 8x8 Keyboard Matrix. | [README-KBD-VIC20.md](README-KBD-VIC20.md) |
| `lib/mmemu-plugin-c64-pla.so` | C64 PLA banking controller (LORAM/HIRAM/CHAREN). | [README-C64PLA.md](README-C64PLA.md) |
| `lib/mmemu-plugin-cia6526.so` | MOS 6526 CIA: timers, TOD clock, I/O ports, IRQ. | [README-6526.md](README-6526.md) |
| `lib/mmemu-plugin-vic2.so` | MOS 6567/6569 VIC-II video chip with sprite engine. | [README-VIC2.md](README-VIC2.md) |
| `lib/mmemu-plugin-sid6581.so` | MOS 6581 SID sound chip: 3 voices + SVF filter. | [README-SID.md](README-SID.md) |

---

## 3. Machine Plugins (Presets)
Plugins that implement `MachineDescriptor` factories to compose CPUs and Devices into complete systems.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-vic20.so` | Commodore VIC-20 (stock and +3K/+8K/+16K/+32K RAM expansions). | [README-VIC20.md](README-VIC20.md) |
| `lib/mmemu-plugin-c64.so` | Commodore 64 — 6510, VIC-II, dual CIA, SID, PLA banking. | [README-C64.md](README-C64.md) |

---

## 4. Distribution Strategy
The `lib/` directory is standardized to contain only the dynamic plugins (`.so` files) listed above. This allows for conditional distribution:
- **Core Only**: Ship only the host binaries.
- **VIC-20 Support**: Include the 6502, 6522, 6560, kbd-vic20, and vic20 plugins. Registers machine IDs `vic20`, `vic20+3k`, `vic20+8k`, `vic20+16k`, `vic20+32k`.
- **C64 Support**: Include the 6502 (with 6510), c64-pla, cia6526, vic2, sid6581, and c64 plugins. Registers machine ID `c64`.
- **Developer Tools**: All static libraries and debug symbols are kept in `lib/internal/` and are not required for standard distribution.
