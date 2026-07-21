#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

class RollingStatistics final {
public:
    static constexpr std::size_t MaximumSampleCount = 256;

    void SetCapacity(std::size_t value) {
        capacity = std::clamp<std::size_t>(value, 1, MaximumSampleCount);
        Clear();
    }

    void Add(double value) {
        if (size == capacity) {
            const auto previous = values[next];
            sum -= previous;
            squareSum -= previous * previous;
        }
        else {
            ++size;
        }
        values[next] = value;
        sum += value;
        squareSum += value * value;
        next = (next + 1) % capacity;
    }

    double Mean() const {
        return size == 0 ? 0.0 : sum / static_cast<double>(size);
    }

    double StandardDeviation() const {
        if (size < 2) return 0.0;
        const auto mean = Mean();
        return std::sqrt(std::max(0.0, squareSum / static_cast<double>(size) - mean * mean));
    }

    std::size_t Size() const {
        return size;
    }

    void Clear() {
        values.fill(0.0);
        size = 0;
        next = 0;
        sum = 0.0;
        squareSum = 0.0;
    }

private:
    std::array<double, MaximumSampleCount> values{};
    std::size_t capacity = 1;
    std::size_t size = 0;
    std::size_t next = 0;
    double sum = 0.0;
    double squareSum = 0.0;
};
