#pragma once

// mmemu plugin_loader — Runtime .so / .dll plugin management
//
// Planned contents (see .plan/arch.md §6.2):
//   plugin_loader.h/cpp  PluginLoader: load(), isLoaded(), createCore(),
//                        createMachine(), enumerate(), unloadAll()
//
// Plugin search order:
//   1. Directory alongside the binary
//   2. ~/.config/mmemu/plugins/  (Linux/macOS)  |  %APPDATA%\mmemu\plugins\ (Windows)
//   3. Paths from MMEMU_PLUGIN_PATH environment variable.
