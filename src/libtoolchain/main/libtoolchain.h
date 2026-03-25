#pragma once

// mmemu libtoolchain — Assembler and disassembler interfaces
//
// Planned contents (see .plan/arch.md §8):
//   idisasm.h              IDisassembler + DisasmEntry
//   iassembler.h           IAssembler + AssemblerResult + AssemblerRegistry
//   disassembler_6502.cpp  Disassembler6502 wrapping existing 6502 disasm logic
