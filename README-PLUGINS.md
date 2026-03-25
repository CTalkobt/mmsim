# mmemu Plugin Index

This document provides a categorized index of all dynamically loadable plugins available in the mmemu ecosystem.

---

## 1. Processor Plugins (CPU)
Plugins that implement the `ICore` interface, providing fetch-decode-execute logic and register sets.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-6502.so` | MOS 6502 (NMOS) with illegal opcode support. | [README-6502.md](README-6502.md) |

---

## 2. Device Plugins (I/O & Video)
Plugins that implement the `IOHandler` interface for memory-mapped hardware.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-via6522.so` | MOS 6522 Versatile Interface Adapter (VIA). | [README-6522.md](README-6522.md) |
| `lib/mmemu-plugin-vic6560.so` | MOS 6560 VIC-I Video/Sound generator. | [README-6560.md](README-6560.md) |

---

## 3. Machine Plugins (Presets)
Plugins that implement `MachineDescriptor` factories to compose CPUs and Devices into complete systems.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-vic20.so` | Commodore VIC-20 Machine Configuration. | [README-VIC20.md](README-VIC20.md) |

---

## 4. Distribution Strategy
The `lib/` directory is standardized to contain only the dynamic plugins (`.so` files) listed above. This allows for conditional distribution:
- **Core Only**: Ship only the host binaries.
- **VIC-20 Support**: Include the 6502, 6522, 6560, and vic20 plugins.
- **Developer Tools**: All static libraries and debug symbols are kept in `lib/internal/` and are not required for standard distribution.
