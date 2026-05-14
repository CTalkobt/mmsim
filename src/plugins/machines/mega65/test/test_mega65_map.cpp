#include "test_harness.h"
#include "plugins/devices/map_mmu/main/map_mmu.h"
#include "plugins/45gs02/main/cpu45gs02.h"
#include "libmem/main/sparse_memory_bus.h"

// Helper: set up a 45GS02 CPU wired through MapMmu to a SparseMemoryBus
struct MapTestFixture {
    SparseMemoryBus physBus;
    MapMmu mmu;
    MOS45GS02 cpu;

    MapTestFixture() : physBus("phys", 28), mmu("mmu", &physBus) {
        cpu.setDataBus(&mmu);
        cpu.setCodeBus(&mmu);
        cpu.setMapMmu(&mmu);
        // Reset vector → $0200 (code area away from block 0 mapping tests)
        physBus.write8(0xFFFC, 0x00);
        physBus.write8(0xFFFD, 0x02);
        cpu.reset();
    }

    // Write a small program at addr and reset PC there
    void loadCode(uint16_t addr, const uint8_t* code, size_t len) {
        for (size_t i = 0; i < len; i++)
            physBus.write8(addr + i, code[i]);
        cpu.setPc(addr);
    }
};

// Test 1: MAP instruction sets enables from X[7:4] (lower) and Z[7:4] (upper)
TEST_CASE(mega65_map_enable_bits) {
    MapTestFixture f;

    // A=$00, X=$30 (enables blocks 0,1), Y=$00, Z=$C0 (enables blocks 6,7)
    // Lower offset = ((X & 0x0F) << 8) | A = 0
    // Upper offset = ((Z & 0x0F) << 8) | Y = 0
    uint8_t code[] = {
        0xA9, 0x00,   // LDA #$00
        0xA2, 0x30,   // LDX #$30  (enables = 0011 for blocks 0,1)
        0xA0, 0x00,   // LDY #$00
        0xA3, 0xC0,   // LDZ #$C0  (enables = 1100 for blocks 6,7)
        0x5C,         // MAP
    };
    f.loadCode(0x0200, code, sizeof(code));

    for (int i = 0; i < 5; i++) f.cpu.step();

    const MapState& ms = f.mmu.getMapState();
    // Lower enables from X[7:4] = 0x3 → blocks 0,1
    // Upper enables from Z[7:4] = 0xC → blocks 6,7
    ASSERT_EQ(ms.enables, 0x03 | (0x0C << 4));  // 0xC3
}

// Test 2: MAP instruction computes per-block offsets correctly
TEST_CASE(mega65_map_per_block_offsets) {
    MapTestFixture f;

    // Lower offset = ((X & 0x0F) << 8) | A = ((0x02) << 8) | 0x00 = 0x200
    // Enable all lower blocks: X[7:4] = 0xF → X = 0xF2
    // Upper offset = ((Z & 0x0F) << 8) | Y = ((0x03) << 8) | 0x00 = 0x300
    // Enable all upper blocks: Z[7:4] = 0xF → Z = 0xF3
    uint8_t code[] = {
        0xA9, 0x00,   // LDA #$00
        0xA2, 0xF2,   // LDX #$F2
        0xA0, 0x00,   // LDY #$00
        0xA3, 0xF3,   // LDZ #$F3
        0x5C,         // MAP
    };
    f.loadCode(0x0200, code, sizeof(code));

    for (int i = 0; i < 5; i++) f.cpu.step();

    const MapState& ms = f.mmu.getMapState();
    ASSERT_EQ(ms.enables, 0xFF);

    // Each block's stored offset = block_base/256 + mb_offset
    // Block 0: base=$0000 → stored = 0x00 + 0x200 = 0x200
    // Block 1: base=$2000 → stored = 0x20 + 0x200 = 0x220
    // Block 2: base=$4000 → stored = 0x40 + 0x200 = 0x240
    // Block 3: base=$6000 → stored = 0x60 + 0x200 = 0x260
    ASSERT_EQ(ms.offsets[0], 0x200);
    ASSERT_EQ(ms.offsets[1], 0x220);
    ASSERT_EQ(ms.offsets[2], 0x240);
    ASSERT_EQ(ms.offsets[3], 0x260);

    // Block 4: base=$8000 → stored = 0x80 + 0x300 = 0x380
    // Block 5: base=$A000 → stored = 0xA0 + 0x300 = 0x3A0
    // Block 6: base=$C000 → stored = 0xC0 + 0x300 = 0x3C0
    // Block 7: base=$E000 → stored = 0xE0 + 0x300 = 0x3E0
    ASSERT_EQ(ms.offsets[4], 0x380);
    ASSERT_EQ(ms.offsets[5], 0x3A0);
    ASSERT_EQ(ms.offsets[6], 0x3C0);
    ASSERT_EQ(ms.offsets[7], 0x3E0);
}

// Test 3: MAP + data access end-to-end
TEST_CASE(mega65_map_end_to_end_data_access) {
    MapTestFixture f;

    // Place test data at physical $030000
    f.physBus.write8(0x030000, 0x42);

    // Map block 4 ($8000-$9FFF) with mb_offset so that $8000 → phys $030000
    // stored_offset[4] = block_base/256 + mb_offset = 0x80 + mb_offset
    // We want stored_offset[4] = 0x300, so mb_offset = 0x300 - 0x80 = 0x280
    // A = low byte = 0x80, X[3:0] = high byte = 0x02, X[7:4] = enable bit 0 (block 4) = 0x1
    // Wait — enables for upper half come from Z, not X.
    // Lower enables = X[7:4], upper enables = Z[7:4]
    // We want to enable block 4 (bit 0 of upper enables) → Z[7:4] = 0x1 → Z = 0x1?
    // Upper offset = ((Z & 0x0F) << 8) | Y = ((Z[3:0]) << 8) | Y
    // mb_offset = 0x280, so Y = 0x80, Z[3:0] = 0x02, Z[7:4] = 0x01
    // Z = 0x12
    // Code runs from $0200 (block 1, unmapped) — safe.
    uint8_t code[] = {
        0xA9, 0x00,   // LDA #$00
        0xA2, 0x00,   // LDX #$00  (no lower enables)
        0xA0, 0x80,   // LDY #$80  (upper offset low byte)
        0xA3, 0x12,   // LDZ #$12  (enable block 4, offset high nybble = 2)
        0x5C,         // MAP
        0xAD, 0x00, 0x80,  // LDA $8000 (should read from phys $030000)
    };
    f.loadCode(0x0200, code, sizeof(code));

    for (int i = 0; i < 5; i++) f.cpu.step();  // Load regs + MAP
    f.cpu.step();  // LDA $8000

    ASSERT_EQ(f.cpu.regRead(0), 0x42);  // A should be $42
}

// Test 4: EOM does NOT clear MAP state
TEST_CASE(mega65_eom_preserves_map_state) {
    MapTestFixture f;

    // Set up a mapping via MAP then execute EOM
    uint8_t code[] = {
        0xA9, 0x00,   // LDA #$00
        0xA2, 0x10,   // LDX #$10  (enable block 0, offset=0)
        0xA0, 0x00,   // LDY #$00
        0xA3, 0x00,   // LDZ #$00
        0x5C,         // MAP
        0x7C,         // EOM
    };
    f.loadCode(0x0200, code, sizeof(code));

    for (int i = 0; i < 6; i++) f.cpu.step();  // Load regs + MAP + EOM

    const MapState& ms = f.mmu.getMapState();
    // EOM should NOT have cleared the enables
    ASSERT_EQ(ms.enables & 0x01, 0x01);
}

// Test 5: MAP without MapMmu doesn't crash
TEST_CASE(mega65_map_no_mmu_no_crash) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);
    MOS45GS02 cpu;
    cpu.setDataBus(&mmu);
    cpu.setCodeBus(&mmu);
    // Deliberately do NOT call cpu.setMapMmu()

    physBus.write8(0xFFFC, 0x00);
    physBus.write8(0xFFFD, 0x02);
    cpu.reset();

    uint8_t code[] = { 0x5C, 0x7C };  // MAP, EOM
    for (size_t i = 0; i < sizeof(code); i++)
        physBus.write8(0x0200 + i, code[i]);

    cpu.step();  // MAP — should not crash
    cpu.step();  // EOM — should not crash
}

// Test 2: MAP instruction with address translation
TEST_CASE(mega65_map_instruction_simple_translation) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write test data at physical address $040000
    physBus.write8(0x040000, 0x42);

    // Set up MapState manually
    // Block 5 ($A000-$BFFF): offset = 0x400 → physical $40000
    MapState state = {};
    state.offsets[5] = 0x400;  // 0x400 << 8 = 0x40000
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Now read via virtual address $A000 should get physical $40000
    ASSERT_EQ(mmu.read8(0xA000), 0x42);
}

// Test 3: MAP instruction disabled passthrough
TEST_CASE(mega65_map_instruction_disabled) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write test data at virtual/physical address $8000
    physBus.write8(0x008000, 0xAA);

    // MapState with no blocks enabled
    MapState state = {};
    state.enables = 0;
    mmu.setMapState(state);

    // Should passthrough to physical address $8000
    ASSERT_EQ(mmu.read8(0x8000), 0xAA);
}

// Test 4: Multiple MAP blocks enabled
TEST_CASE(mega65_map_instruction_multiple_blocks) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write test data at different physical addresses
    physBus.write8(0x020000, 0x11);  // Block 2
    physBus.write8(0x040000, 0x22);  // Block 5
    physBus.write8(0x060000, 0x33);  // Block 6

    // Set up MapState
    MapState state = {};
    state.offsets[2] = 0x200;  // $4000-$5FFF → physical $20000
    state.offsets[5] = 0x400;  // $A000-$BFFF → physical $40000
    state.offsets[6] = 0x600;  // $C000-$DFFF → physical $60000
    state.enables = (1 << 2) | (1 << 5) | (1 << 6);
    mmu.setMapState(state);

    // Verify all translations
    ASSERT_EQ(mmu.read8(0x4000), 0x11);
    ASSERT_EQ(mmu.read8(0xA000), 0x22);
    ASSERT_EQ(mmu.read8(0xC000), 0x33);
}

// Test 5: Address translation within a block
TEST_CASE(mega65_map_instruction_block_offset_in_range) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write test pattern at physical $50000-$50007
    for (int i = 0; i < 8; i++) {
        physBus.write8(0x050000 + i, 0x10 + i);
    }

    // Map virtual $A000-$BFFF to physical $50000
    MapState state = {};
    state.offsets[5] = 0x500;
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Read within block: virtual $A000-$A007 should map to physical $50000-$50007
    for (int i = 0; i < 8; i++) {
        uint8_t val = mmu.read8(0xA000 + i);
        ASSERT_EQ(val, 0x10 + i);
    }
}

// Test 6: MAP state save/restore
TEST_CASE(mega65_map_instruction_state_persistence) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Set initial state
    MapState state = {};
    state.offsets[3] = 0x123;
    state.offsets[5] = 0x456;
    state.enables = (1 << 3) | (1 << 5);
    mmu.setMapState(state);

    // Verify state was stored
    const MapState& readBack = mmu.getMapState();
    ASSERT_EQ(readBack.offsets[3], 0x123);
    ASSERT_EQ(readBack.offsets[5], 0x456);
    ASSERT_EQ(readBack.enables, (1 << 3) | (1 << 5));
}

// Test 7: Reset clears MAP state
TEST_CASE(mega65_map_instruction_reset) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Set state
    MapState state = {};
    state.offsets[5] = 0x400;
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Reset
    mmu.reset();

    // Verify state is cleared
    const MapState& readBack = mmu.getMapState();
    ASSERT_EQ(readBack.enables, 0);
    for (int i = 0; i < 8; ++i) {
        ASSERT_EQ(readBack.offsets[i], 0);
    }
}

// Test 8: All 8 blocks mapped simultaneously
TEST_CASE(mega65_map_instruction_all_blocks) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write data for all 8 blocks
    for (int i = 0; i < 8; ++i) {
        uint32_t phys = (0x1000 + i) << 8;
        physBus.write8(phys, 0x10 + i);
    }

    // Map all blocks
    MapState state = {};
    for (int i = 0; i < 8; ++i) {
        state.offsets[i] = 0x1000 + i;
        state.enables |= (1 << i);
    }
    mmu.setMapState(state);

    // Verify all blocks are accessible
    ASSERT_EQ(mmu.read8(0x0000), 0x10);  // Block 0: $0000-$1FFF
    ASSERT_EQ(mmu.read8(0x2000), 0x11);  // Block 1: $2000-$3FFF
    ASSERT_EQ(mmu.read8(0x4000), 0x12);  // Block 2: $4000-$5FFF
    ASSERT_EQ(mmu.read8(0x6000), 0x13);  // Block 3: $6000-$7FFF
    ASSERT_EQ(mmu.read8(0x8000), 0x14);  // Block 4: $8000-$9FFF
    ASSERT_EQ(mmu.read8(0xA000), 0x15);  // Block 5: $A000-$BFFF
    ASSERT_EQ(mmu.read8(0xC000), 0x16);  // Block 6: $C000-$DFFF
    ASSERT_EQ(mmu.read8(0xE000), 0x17);  // Block 7: $E000-$FFFF
}

// Test 9: Write through MAP
TEST_CASE(mega65_map_instruction_write_translation) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Map block 5
    MapState state = {};
    state.offsets[5] = 0x400;
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Write via virtual address
    mmu.write8(0xA000, 0x55);

    // Verify via physical address
    ASSERT_EQ(physBus.read8(0x040000), 0x55);
}

// Test 10: Mixed enabled/disabled blocks
TEST_CASE(mega65_map_instruction_mixed_blocks) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write test data
    physBus.write8(0x000000, 0xAA);  // Unmapped block 0
    physBus.write8(0x000200, 0xBB);  // Will be mapped via block 1
    physBus.write8(0x040000, 0xCC);  // Unmapped block 2

    // Only map block 1
    MapState state = {};
    state.offsets[1] = 0x002;  // offset 0x002 maps to physical 0x000200
    state.enables = (1 << 1);
    mmu.setMapState(state);

    // Block 0: unmapped, should passthrough
    ASSERT_EQ(mmu.read8(0x0000), 0xAA);

    // Block 1: virtual 0x2000 maps to physical 0x000200
    ASSERT_EQ(mmu.read8(0x2000), 0xBB);

    // Block 2: unmapped, should passthrough
    physBus.write8(0x004000, 0xDD);
    ASSERT_EQ(mmu.read8(0x4000), 0xDD);
}

// Test 11: ROM overlay with MAP translation
TEST_CASE(mega65_map_instruction_rom_overlay) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Create ROM data
    uint8_t romData[256] = {0};
    for (int i = 0; i < 256; i++) {
        romData[i] = (uint8_t)(0xC0 + i);
    }

    // Add ROM overlay at physical $F0000
    physBus.addRegion(0x0F0000, 256, romData, false);

    // Map block 7 to physical $F0000
    MapState state = {};
    state.offsets[7] = 0xF00;
    state.enables = (1 << 7);
    mmu.setMapState(state);

    // Read ROM data through virtual address
    ASSERT_EQ(mmu.read8(0xE000), 0xC0);
    ASSERT_EQ(mmu.read8(0xE001), 0xC1);
}

// Test 12: Cross-block boundary read (should not cross)
TEST_CASE(mega65_map_instruction_block_boundary) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Block 1 maps to physical $020000, Block 2 maps to physical $030000
    MapState state = {};
    state.offsets[1] = 0x200;
    state.offsets[2] = 0x300;
    state.enables = (1 << 1) | (1 << 2);
    mmu.setMapState(state);

    // Write distinct values at block boundaries
    physBus.write8(0x021FFF, 0x11);  // End of block 1
    physBus.write8(0x030000, 0x22);  // Start of block 2

    // Read at boundary
    ASSERT_EQ(mmu.read8(0x3FFF), 0x11);  // Block 1 last byte
    ASSERT_EQ(mmu.read8(0x4000), 0x22);  // Block 2 first byte
}

// Test 13: 28-bit address space access
TEST_CASE(mega65_map_instruction_28bit_addressing) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Access high physical address (requires 28-bit space)
    uint32_t highAddr = 0x0FFFFFFC;  // Max 28-bit address
    physBus.write8(highAddr, 0xFF);

    // This should work with SparseMemoryBus (28-bit support)
    ASSERT_EQ(physBus.read8(highAddr), 0xFF);
}

// Test 14: MapMmu copy state snapshot
TEST_CASE(mega65_map_instruction_state_save_restore) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Set state
    MapState state1 = {};
    state1.offsets[4] = 0x100;
    state1.offsets[7] = 0x700;
    state1.enables = (1 << 4) | (1 << 7);
    mmu.setMapState(state1);

    // Get state
    const MapState& readBack = mmu.getMapState();
    MapState state2 = readBack;

    // Modify MapMmu state
    MapState state3 = {};
    mmu.setMapState(state3);

    // Restore from saved copy
    mmu.setMapState(state2);

    // Verify restoration
    const MapState& restored = mmu.getMapState();
    ASSERT_EQ(restored.offsets[4], 0x100);
    ASSERT_EQ(restored.offsets[7], 0x700);
    ASSERT_EQ(restored.enables, (1 << 4) | (1 << 7));
}

// Test 15: MAP state serialization (save/load from buffer)
TEST_CASE(mega65_map_state_serialization) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Set state
    MapState state = {};
    state.offsets[5] = 0x400;
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Get size and save
    size_t stateSize = mmu.stateSize();
    ASSERT_NE(stateSize, 0);

    uint8_t* buffer = new uint8_t[stateSize];
    mmu.saveState(buffer);

    // Clear state
    mmu.reset();
    ASSERT_EQ(mmu.getMapState().enables, 0);

    // Restore state
    mmu.loadState(buffer);
    const MapState& restored = mmu.getMapState();
    ASSERT_EQ(restored.offsets[5], 0x400);
    ASSERT_EQ(restored.enables, (1 << 5));

    delete[] buffer;
}

// Test 16: Large address offset (20-bit offset value)
TEST_CASE(mega65_map_instruction_large_offset) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Use a large 20-bit offset
    uint32_t largeOffset = 0xFFFFF;  // Max 20-bit value
    physBus.write8((largeOffset << 8), 0xAB);

    MapState state = {};
    state.offsets[0] = largeOffset;  // Will map $0000-$1FFF to very high physical address
    state.enables = (1 << 0);
    mmu.setMapState(state);

    // Read through translation
    ASSERT_EQ(mmu.read8(0x0000), 0xAB);
}

// Test 17: Sequential reads across multiple blocks
TEST_CASE(mega65_map_instruction_sequential_reads) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write sequence: block 5 and 6
    for (int i = 0; i < 0x4000; i++) {  // 16KB total
        physBus.write8(0x040000 + i, (uint8_t)(i & 0xFF));
        physBus.write8(0x060000 + i, (uint8_t)((i + 1) & 0xFF));
    }

    MapState state = {};
    state.offsets[5] = 0x400;
    state.offsets[6] = 0x600;
    state.enables = (1 << 5) | (1 << 6);
    mmu.setMapState(state);

    // Sequential reads
    for (int i = 0; i < 256; i++) {
        uint8_t val = mmu.read8(0xA000 + i);
        ASSERT_EQ(val, (uint8_t)(i & 0xFF));
    }
}

// Test 18: Write protection through ROM overlay
TEST_CASE(mega65_map_instruction_rom_write_protected) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Create read-only ROM
    uint8_t romData[256] = {0};
    for (int i = 0; i < 256; i++) {
        romData[i] = 0x42;
    }
    physBus.addRegion(0x0E0000, 256, romData, false);  // Read-only

    // Map block 7 to ROM
    MapState state = {};
    state.offsets[7] = 0xE00;
    state.enables = (1 << 7);
    mmu.setMapState(state);

    // Read should work
    ASSERT_EQ(mmu.read8(0xE000), 0x42);

    // Write should be handled by bus (ROM write-protection)
    mmu.write8(0xE000, 0xFF);
    // Value should remain unchanged (ROM is protected)
    ASSERT_EQ(mmu.read8(0xE000), 0x42);
}

// Test 19: Peek (non-destructive read) through MAP
TEST_CASE(mega65_map_instruction_peek) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    physBus.write8(0x040000, 0x99);

    MapState state = {};
    state.offsets[5] = 0x400;
    state.enables = (1 << 5);
    mmu.setMapState(state);

    // Peek should return same value as read
    ASSERT_EQ(mmu.peek8(0xA000), mmu.read8(0xA000));
    ASSERT_EQ(mmu.peek8(0xA000), 0x99);
}

// Test 20: Full address range coverage
TEST_CASE(mega65_map_instruction_full_coverage) {
    SparseMemoryBus physBus("phys", 28);
    MapMmu mmu("mmu", &physBus);

    // Write a test pattern at physical 0x0000-0x1FFF
    for (int i = 0; i < 0x2000; i++) {
        physBus.write8(i, (uint8_t)(i & 0xFF));
    }

    // Map all blocks to physical 0x0000 (all blocks offset 0)
    MapState state = {};
    for (int i = 0; i < 8; i++) {
        state.offsets[i] = 0;  // All blocks map to physical 0x0000
        state.enables |= (1 << i);
    }
    mmu.setMapState(state);

    // Spot check addresses - should all map to the same physical region
    ASSERT_EQ(mmu.read8(0x0000), 0x00);
    ASSERT_EQ(mmu.read8(0x1000), 0x00);  // Within mapped region
    ASSERT_EQ(mmu.read8(0x5000), (uint8_t)(0x5000 & 0xFF));  // Maps to phys (0x5000 & 0x1FFF)
}
