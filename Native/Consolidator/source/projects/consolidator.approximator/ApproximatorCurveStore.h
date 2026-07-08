#pragma once

#include "c74_min.h"

#include <cmath>
#include <vector>

struct TargetCurve {
    std::vector<double> values;
    std::vector<double> frequencies;
};

class ApproximatorCurveStore {
public:
    void set_target(const c74::min::atoms& args) {
        curve_.values.clear();
        curve_.values.reserve(args.size());

        for (const auto& a : args) {
            curve_.values.push_back(static_cast<double>(a));
        }

        curve_.frequencies = make_log_freqs(curve_.values.size(), 20.0, 20000.0);
    }

    void clear() {
        curve_.frequencies.clear();
        curve_.values.clear();
    }

    bool empty() const {
        return curve_.values.empty();
    }

    const TargetCurve& curve() const {
        return curve_;
    }

    const std::vector<double>& freqs() const {
        return curve_.frequencies;
    }

    const std::vector<double>& target_db() const {
        return curve_.values;
    }

private:
    static std::vector<double> make_log_freqs(size_t count, double min_hz, double max_hz) {
        std::vector<double> result;
        result.reserve(count);

        const double min_log = std::log(min_hz);
        const double max_log = std::log(max_hz);

        for (size_t i = 0; i < count; ++i) {
            const double t = count <= 1
                ? 0.0
                : static_cast<double>(i) / static_cast<double>(count - 1);

            result.push_back(std::exp(min_log + t * (max_log - min_log)));
        }

        return result;
    }

    TargetCurve curve_;
};
