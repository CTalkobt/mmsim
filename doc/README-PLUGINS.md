# mmemu Plugin Index

This document provides a categorized index of all dynamically loadable plugins available in the mmemu ecosystem, along with documentation for the Plugin API.

---

## 1. Plugin Index

### 1.1 Processor Plugins (CPU)
Plugins that implement the `ICore` interface.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-6502.so` | MOS 6502 (NMOS) with illegal opcode support; also provides MOS 6510. | [README-6502.md](README-6502.md) · [README-6510.md](README-6510.md) |

### 1.2 Device Plugins (I/O & Video)
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
| `lib/mmemu-plugin-pia6520.so` | MOS 6520 PIA: dual 8-bit I/O ports, handshake lines. | [README-6520.md](README-6520.md) |
| `lib/mmemu-plugin-crtc6545.so` | MOS 6545/6845 CRTC video controller. | [README-6545.md](README-6545.md) |
| `lib/mmemu-plugin-pet-video.so` | PET Video Subsystem (Discrete and CRTC models). | [README-PET-VIDEO.md](README-PET-VIDEO.md) |

### 1.3 Machine Plugins (Presets)
Plugins that implement `MachineDescriptor` factories.

| Plugin File | Description | Documentation |
|-------------|-------------|---------------|
| `lib/mmemu-plugin-vic20.so` | Commodore VIC-20 (stock and RAM expansions). | [README-VIC20.md](README-VIC20.md) |
| `lib/mmemu-plugin-c64.so` | Commodore 64 — full chip set and banking. | [README-C64.md](README-C64.md) |
| `lib/mmemu-plugin-pet.so` | Commodore PET Series (2001, 4032, 8032). | [README-PET.md](README-PET.md) |

### 1.4 Utility & Loader Plugins
Plugins providing support services like ROM importing or file format parsing.

| Plugin File | Description |
|-------------|-------------|
| `lib/mmemu-plugin-vice-importer.so` | Discovers and imports ROMs from a local VICE installation. |
| `lib/mmemu-plugin-cbm-loader.so` | Implements `.prg` program loading and `.crt` cartridge support. |

---

## 2. Plugin API (C ABI)

mmemu uses a stable C-compatible ABI for plugins. Every plugin must export a single entry point:

```c
extern "C" SimPluginManifest* mmemuPluginInit(const SimPluginHostAPI* host);
```

### 2.1 The Host API (`SimPluginHostAPI`)
The host provides a set of function pointers for plugins to interact with the core emulator without linking against C++ classes.

- **Logging**: `log`, `getLogger`, `logNamed`. Named loggers are visible in the CLI/GUI "log" commands.
- **Registry Access**: `createCore`, `createDevice`, `createMachine`, `createDisassembler`, `createAssembler`.
- **Image Handling**: `findImageLoader`, `createCartridgeHandler`.
- **Extension Points**:
    - `registerPane`: Add a new tab/window to the GUI.
    - `registerCommand`: Add a new command to the CLI.
    - `registerMcpTool`: Add a new tool to the AI-facing MCP server.

### 2.2 The Manifest (`SimPluginManifest`)
The plugin describes its contents via this struct returned from `mmemuPluginInit`.

- **Metadata**: `pluginId`, `displayName`, `version`.
- **Dependencies**: `deps` (null-terminated list of required plugin IDs).
- **Filtering**: `supportedMachineIds` (null-terminated list of machines this plugin is relevant for).
- **Factories**: Arrays of `CorePluginInfo`, `DevicePluginInfo`, `MachinePluginInfo`, etc.

---

## 3. Distribution Strategy
The `lib/` directory contains the dynamic plugins (`.so` files). Standard distribution bundles include:
- **VIC-20 Support**: `6502`, `via6522`, `vic6560`, `kbd-vic20`, `vic20`, `cbm-loader`, `vice-importer`.
- **C64 Support**: `6502`, `c64-pla`, `cia6526`, `vic2`, `sid6581`, `c64`, `cbm-loader`, `vice-importer`.
- **PET Support**: `6502`, `pia6520`, `via6522`, `crtc6545`, `pet-video`, `pet`, `cbm-loader`, `vice-importer`.
