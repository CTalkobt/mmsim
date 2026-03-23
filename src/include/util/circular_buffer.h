#pragma once

#include <vector>
#include <cstddef>
#include <stdexcept>

/**
 * A simple circular buffer (ring buffer) implementation.
 */
template <typename T>
class CircularBuffer {
public:
    CircularBuffer(size_t capacity = 1000)
        : m_capacity(capacity), m_buffer(capacity) {
    }

    void push(const T& item) {
        if (m_capacity == 0) return;
        m_buffer[m_head] = item;
        m_head = (m_head + 1) % m_capacity;
        if (m_size < m_capacity) {
            m_size++;
        }
    }

    void clear() {
        m_head = 0;
        m_size = 0;
    }

    size_t size() const {
        return m_size;
    }

    size_t capacity() const {
        return m_capacity;
    }

    const T& operator[](size_t index) const {
        if (index >= m_size) {
            throw std::out_of_range("CircularBuffer index out of range");
        }
        if (m_size < m_capacity) {
            return m_buffer[index];
        } else {
            return m_buffer[(m_head + index) % m_capacity];
        }
    }

    T& operator[](size_t index) {
        if (index >= m_size) {
            throw std::out_of_range("CircularBuffer index out of range");
        }
        if (m_size < m_capacity) {
            return m_buffer[index];
        } else {
            return m_buffer[(m_head + index) % m_capacity];
        }
    }

private:
    size_t m_capacity;
    size_t m_size = 0;
    size_t m_head = 0;
    std::vector<T> m_buffer;
};
