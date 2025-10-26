#pragma once

#include <atomic>
#include <vector>
#include <optional>

namespace piano_recorder {

template <typename T>
class SpscRingBuffer {
public:
    explicit SpscRingBuffer(size_t capacity)
        : _buffer(capacity), _head(0), _tail(0) {}

    bool push(const T& value) {
        size_t head = _head;
        size_t tail = _tail;

        if (tail - head == _buffer.size()) {
            return false; // full
        }

        _buffer[to_idx(head)] = value;
        _head = head + 1;
        return true;
    }

    void force_push(const T& value) {
        size_t head = _head;
        size_t tail = _tail;

        if (tail - head == _buffer.size()) {
            tail += 1;
        }

        _buffer[to_idx(head)] = value;

        _tail = tail;
        _head = head + 1;
    }

    std::optional<T> pop() {
        size_t head = _head;
        size_t tail = _tail;

        if (head == tail) {
            return {}; // empty
        }

        T value = _buffer[to_idx(tail)];
        _tail = tail + 1;
        return value;
    }

    bool empty(void) const {
        return _head == _tail;
    }

    bool full(void) const {
        return size() == _buffer.size();
    }

    size_t size(void) const {
        return _tail - _head;
    }

private:
    size_t to_idx(size_t gidx) { return gidx % _buffer.size(); }

    std::vector<T> _buffer;
    std::atomic<size_t> _head;
    std::atomic<size_t> _tail;
};

} // namespace piano_recorder

