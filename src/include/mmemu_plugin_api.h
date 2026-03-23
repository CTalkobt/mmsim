#pragma once

// mmemu public plugin ABI — distributed with all plugin repos
//
// This header defines the contract between the mmemu host and commercial
// CPU / device plugins loaded at runtime via dlopen / LoadLibrary.
//
// Planned contents (see .plan/arch.md §6.1):
//   SimPluginManifest    Top-level plugin descriptor returned by the init symbol
//   SimPluginHostAPI     Host services provided to plugins (logging, IBus helpers)
//   CorePluginInfo       Per-CPU-core descriptor (ICoreCtor, license_class, ...)
//   DevicePluginInfo     Per-device IOHandler factory
//   MachinePluginInfo    Per-machine MachineDescriptor factory
//
// Every plugin .so must export:
//   SimPluginManifest *mmemu_plugin_init(const SimPluginHostAPI *host);
