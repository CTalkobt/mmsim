#include "test_harness.h"
#include "trace_buffer.h"

TEST_CASE(trace_buffer_basic) {
    TraceBuffer buffer(3);
    ASSERT(buffer.size() == 0);

    TraceEntry e1; e1.addr = 0x1000;
    TraceEntry e2; e2.addr = 0x2000;
    TraceEntry e3; e3.addr = 0x3000;

    buffer.push(e1);
    buffer.push(e2);
    buffer.push(e3);

    ASSERT(buffer.size() == 3);
    // When size < capacity, it returns by index directly
    ASSERT(buffer.at(0).addr == 0x1000);
    ASSERT(buffer.at(1).addr == 0x2000);
    ASSERT(buffer.at(2).addr == 0x3000);
}

TEST_CASE(trace_buffer_wrap) {
    TraceBuffer buffer(3);
    TraceEntry e1; e1.addr = 0x1000;
    TraceEntry e2; e2.addr = 0x2000;
    TraceEntry e3; e3.addr = 0x3000;
    TraceEntry e4; e4.addr = 0x4000;

    buffer.push(e1);
    buffer.push(e2);
    buffer.push(e3);
    buffer.push(e4); // Should overwrite e1

    ASSERT(buffer.size() == 3);
    // head was 0, after 3 pushes it's 0 again. After 4th push, it's 1.
    // buffer = [e4, e2, e3], head = 1.
    // CircularBuffer[0] when size == capacity returns buffer[(head + index) % capacity]
    // index 0 -> buffer[(1+0)%3] = buffer[1] = e2
    // index 1 -> buffer[(1+1)%3] = buffer[2] = e3
    // index 2 -> buffer[(1+2)%3] = buffer[0] = e4

    ASSERT(buffer.at(0).addr == 0x2000);
    ASSERT(buffer.at(1).addr == 0x3000);
    ASSERT(buffer.at(2).addr == 0x4000);
}
