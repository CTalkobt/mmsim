#pragma once

// mmemu libmem — Abstract memory / address bus model
//
// Planned contents (see .plan/arch.md §3, §7):
//   ibus.h              IBus abstract interface (read8/write8, BusConfig, BusRole)
//   memory_bus.h/cpp    Memory6502Bus, FlatMemoryBus, SparseMemoryBus, PagedMemoryBus
//                       HarvardCodeBus, HarvardDataBus
