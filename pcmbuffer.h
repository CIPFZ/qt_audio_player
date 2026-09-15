#pragma once
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <stdexcept>
#include <vector>

// SPSC ring: one decoder producer, one audio callback consumer. No locks or allocations.
class PcmBuffer
{
public:
    explicit PcmBuffer(uint32_t capacity = 131072) : m_data(capacity), m_capacity(capacity)
    {
        // A power of two keeps modulo addressing correct when the uint32 counters wrap.
        if (capacity == 0 || (capacity & (capacity - 1)) != 0)
            throw std::invalid_argument("PCM ring capacity must be a power of two");
    }
    uint32_t available() const
    {
        return m_written.load(std::memory_order_acquire) - m_read.load(std::memory_order_acquire);
    }
    uint32_t write(const float *data, uint32_t count)
    {
        const auto written = m_written.load(std::memory_order_relaxed);
        const auto read = m_read.load(std::memory_order_acquire);
        count = std::min(count, m_capacity - (written - read));
        for (uint32_t i = 0; i < count; ++i) m_data[(written + i) % m_capacity] = data[i];
        m_written.store(written + count, std::memory_order_release);
        return count;
    }
    uint32_t read(float *output, uint32_t count, float gain)
    {
        const auto read = m_read.load(std::memory_order_relaxed);
        const auto written = m_written.load(std::memory_order_acquire);
        const auto copied = std::min(count, written - read);
        for (uint32_t i = 0; i < copied; ++i)
            output[i] = std::clamp(m_data[(read + i) % m_capacity] * gain, -1.0f, 1.0f);
        std::fill(output + copied, output + count, 0.0f);
        m_read.store(read + copied, std::memory_order_release);
        return copied;
    }
    // Both producer and consumer must be stopped before reset.
    void reset() { m_read.store(0); m_written.store(0); }
private:
    static_assert(std::atomic<uint32_t>::is_always_lock_free, "Audio requires lock-free counters");
    std::vector<float> m_data;
    const uint32_t m_capacity;
    alignas(64) std::atomic<uint32_t> m_read{0};
    alignas(64) std::atomic<uint32_t> m_written{0};
};
