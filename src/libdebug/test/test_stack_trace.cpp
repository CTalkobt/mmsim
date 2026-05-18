#include "test_harness.h"
#include "libdebug/main/stack_trace.h"

TEST_CASE(stack_trace_push_pop) {
    StackTrace st;
    ASSERT_EQ(st.depth(), 0);

    st.push(StackPushType::CALL, 0x1000, 0x2000);
    ASSERT_EQ(st.depth(), 1);

    st.push(StackPushType::PHA, 0x1003, 0x42);
    ASSERT_EQ(st.depth(), 2);

    st.pop();
    ASSERT_EQ(st.depth(), 1);

    st.pop();
    ASSERT_EQ(st.depth(), 0);

    // Pop on empty should be safe
    st.pop();
    ASSERT_EQ(st.depth(), 0);
}

TEST_CASE(stack_trace_recent) {
    StackTrace st;
    st.push(StackPushType::CALL, 0x1000, 0x2000);
    st.push(StackPushType::PHA,  0x2000, 0xAA);
    st.push(StackPushType::PHX,  0x2001, 0xBB);

    // recent(0) = all, most-recent-first
    auto all = st.recent(0);
    ASSERT_EQ((int)all.size(), 3);
    ASSERT(all[0].type == StackPushType::PHX);
    ASSERT_EQ(all[0].value, (uint32_t)0xBB);
    ASSERT(all[2].type == StackPushType::CALL);
    ASSERT_EQ(all[2].value, (uint32_t)0x2000);

    // recent(2) = last 2
    auto top2 = st.recent(2);
    ASSERT_EQ((int)top2.size(), 2);
    ASSERT(top2[0].type == StackPushType::PHX);
    ASSERT(top2[1].type == StackPushType::PHA);

    // recent(10) = all (n > depth)
    auto big = st.recent(10);
    ASSERT_EQ((int)big.size(), 3);
}

TEST_CASE(stack_trace_clear) {
    StackTrace st;
    st.push(StackPushType::CALL, 0x1000, 0x2000);
    st.push(StackPushType::PHP, 0x2000, 0x30);
    ASSERT_EQ(st.depth(), 2);

    st.clear();
    ASSERT_EQ(st.depth(), 0);
    ASSERT(st.recent().empty());
}

TEST_CASE(stack_push_type_names) {
    ASSERT(std::string(stackPushTypeName(StackPushType::CALL)) == "CALL");
    ASSERT(std::string(stackPushTypeName(StackPushType::PHA))  == "PHA");
    ASSERT(std::string(stackPushTypeName(StackPushType::PHX))  == "PHX");
    ASSERT(std::string(stackPushTypeName(StackPushType::PHY))  == "PHY");
    ASSERT(std::string(stackPushTypeName(StackPushType::PHP))  == "PHP");
    ASSERT(std::string(stackPushTypeName(StackPushType::PHZ))  == "PHZ");
    ASSERT(std::string(stackPushTypeName(StackPushType::BRK))  == "BRK");
}
