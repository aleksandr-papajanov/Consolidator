#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>

namespace consolidator::dsp {

class SmoothedParameter final {
public:
    struct Step {
        double value;
        bool changed;
    };

    explicit SmoothedParameter(double value = 0.0, std::size_t smoothingSamples = 1)
        : target(value), current(value), activeTarget(value),
          smoothingSamples(std::max<std::size_t>(1, smoothingSamples)) {}

    void SetTarget(double value) {
        target.store(value, std::memory_order_release);
    }

    Step Next() {
        const auto requested = target.load(std::memory_order_acquire);
        if (requested != activeTarget) {
            activeTarget = requested;
            remainingSamples = smoothingSamples;
            increment = (activeTarget - current) / static_cast<double>(remainingSamples);
        }
        if (remainingSamples == 0) return { current, false };
        current += increment;
        if (--remainingSamples == 0) current = activeTarget;
        return { current, true };
    }

    double Target() const {
        return target.load(std::memory_order_acquire);
    }

private:
    std::atomic<double> target;
    double current;
    double activeTarget;
    double increment = 0.0;
    std::size_t smoothingSamples;
    std::size_t remainingSamples = 0;
};

} // namespace consolidator::dsp
