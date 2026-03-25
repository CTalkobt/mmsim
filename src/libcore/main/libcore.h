#pragma once

// mmemu libcore — Abstract CPU and machine model
//
// Planned contents (see .plan/arch.md §4, §5):
//   icore.h          ICore abstract CPU interface + RegDescriptor table
//   icore_factory.h  CorePluginInfo, ICoreCtor — plugin entry point
//   machine_desc.h   MachineDescriptor, CpuSlot, BusSlot, IORegistry
//   rom_loader.h     romLoad utility
//   machines/        Per-machine descriptor factories (raw6502, c64, c128, mega65, x16)
