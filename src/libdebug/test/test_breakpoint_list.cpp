#include "test_harness.h"
#include "libdebug/main/breakpoint_list.h"

TEST_CASE(bplist_add_and_check_exec) {
    BreakpointList bl;
    int id = bl.add(0x1000, BreakpointType::EXEC);
    ASSERT(id > 0);
    ASSERT(bl.hasExecBreakpoints());

    auto* bp = bl.checkExec(0x1000, nullptr);
    ASSERT(bp != nullptr);
    ASSERT_EQ(bp->addr, (uint32_t)0x1000);
    ASSERT_EQ(bp->hitCount, 1);

    // Miss
    ASSERT(bl.checkExec(0x2000, nullptr) == nullptr);
}

TEST_CASE(bplist_add_and_check_write_watch) {
    BreakpointList bl;
    int id = bl.add(0xD020, BreakpointType::WRITE_WATCH);
    ASSERT(id > 0);
    ASSERT(!bl.hasExecBreakpoints());

    auto* bp = bl.checkWrite(0xD020, nullptr);
    ASSERT(bp != nullptr);
    ASSERT_EQ(bp->hitCount, 1);

    // Exec check should not match
    ASSERT(bl.checkExec(0xD020, nullptr) == nullptr);
    // Read check should not match
    ASSERT(bl.checkRead(0xD020, nullptr) == nullptr);
}

TEST_CASE(bplist_add_and_check_read_watch) {
    BreakpointList bl;
    bl.add(0x0400, BreakpointType::READ_WATCH);

    auto* bp = bl.checkRead(0x0400, nullptr);
    ASSERT(bp != nullptr);
    ASSERT_EQ(bp->hitCount, 1);

    ASSERT(bl.checkWrite(0x0400, nullptr) == nullptr);
}

TEST_CASE(bplist_remove) {
    BreakpointList bl;
    int id1 = bl.add(0x1000, BreakpointType::EXEC);
    int id2 = bl.add(0x2000, BreakpointType::EXEC);
    ASSERT_EQ((int)bl.breakpoints().size(), 2);

    bl.remove(id1);
    ASSERT_EQ((int)bl.breakpoints().size(), 1);
    ASSERT(bl.checkExec(0x1000, nullptr) == nullptr);
    ASSERT(bl.checkExec(0x2000, nullptr) != nullptr);

    // Remove nonexistent — should be safe
    bl.remove(999);
    ASSERT_EQ((int)bl.breakpoints().size(), 1);

    bl.remove(id2);
    ASSERT(!bl.hasExecBreakpoints());
}

TEST_CASE(bplist_enable_disable) {
    BreakpointList bl;
    int id = bl.add(0x1000, BreakpointType::EXEC);

    // Disable
    bl.setEnabled(id, false);
    ASSERT(bl.checkExec(0x1000, nullptr) == nullptr);

    // Re-enable
    bl.setEnabled(id, true);
    ASSERT(bl.checkExec(0x1000, nullptr) != nullptr);

    // Disable nonexistent id — should be safe
    bl.setEnabled(999, false);
}

TEST_CASE(bplist_hit_count_and_clear) {
    BreakpointList bl;
    int id = bl.add(0x1000, BreakpointType::EXEC);

    bl.checkExec(0x1000, nullptr);
    bl.checkExec(0x1000, nullptr);
    bl.checkExec(0x1000, nullptr);
    ASSERT_EQ(bl.breakpoints()[0].hitCount, 3);

    bl.clearHitCounts();
    ASSERT_EQ(bl.breakpoints()[0].hitCount, 0);
}

TEST_CASE(bplist_set_condition) {
    BreakpointList bl;
    int id = bl.add(0x1000, BreakpointType::EXEC);

    bl.setCondition(id, "A == $42");
    ASSERT(bl.breakpoints()[0].condition == "A == $42");

    // Set condition on nonexistent — safe
    bl.setCondition(999, "test");
}

TEST_CASE(bplist_multiple_at_same_addr) {
    BreakpointList bl;
    int id1 = bl.add(0x1000, BreakpointType::EXEC);
    int id2 = bl.add(0x1000, BreakpointType::WRITE_WATCH);

    // Exec check finds first
    ASSERT(bl.checkExec(0x1000, nullptr) != nullptr);
    // Write check finds second
    ASSERT(bl.checkWrite(0x1000, nullptr) != nullptr);
}

TEST_CASE(bplist_unique_ids) {
    BreakpointList bl;
    int id1 = bl.add(0x1000, BreakpointType::EXEC);
    int id2 = bl.add(0x2000, BreakpointType::EXEC);
    int id3 = bl.add(0x3000, BreakpointType::WRITE_WATCH);
    ASSERT(id1 != id2);
    ASSERT(id2 != id3);
    ASSERT(id1 != id3);
}
