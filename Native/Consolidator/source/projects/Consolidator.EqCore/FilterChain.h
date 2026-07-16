#pragma once

#include "EqBiquad.h"
#include "EqFrequencyGrid.h"
#include "FilterSpec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

class FilterChain {
public:
    static constexpr size_t max_filters = 8;

    void set_sample_rate(double sample_rate) {
        if (sample_rate_ != sample_rate) {
            sample_rate_ = sample_rate;
            dirty_ = true;
        }
    }

    void clear() {
        for (auto& slot : slots_) {
            slot.active = false;
            slot.kind = RuntimeKind::none;
            slot.state_l.reset();
            slot.state_r.reset();
            slot.state_l2.reset();
            slot.state_r2.reset();
            slot.coefficients = {};
            slot.coefficients2 = {};
            slot.gain_scalar = 1.0;
        }

        dirty_ = true;
    }

    void set_filter(size_t index, const FilterSpec& spec) {
        if (index >= max_filters) {
            return;
        }

        slots_[index].spec = spec;
        slots_[index].active = true;
        dirty_ = true;
    }

    void remove_filter(size_t index) {
        if (index >= max_filters) {
            return;
        }

        auto& slot = slots_[index];
        slot.active = false;
        slot.kind = RuntimeKind::none;
        slot.state_l.reset();
        slot.state_r.reset();
        slot.state_l2.reset();
        slot.state_r2.reset();
        slot.coefficients = {};
        slot.coefficients2 = {};
        slot.gain_scalar = 1.0;
        dirty_ = true;
    }

    const FilterSpec& filter(size_t index) const {
        return slots_[index].spec;
    }

    std::vector<double> response_curve(const std::vector<double>& freqs) const {
        rebuild_if_needed();

        std::vector<double> result(freqs.size(), 0.0);

        for (const auto& slot : slots_) {
            if (!slot.active) {
                continue;
            }

            for (size_t i = 0; i < freqs.size(); ++i) {
                result[i] += filter_response_db(slot.spec, freqs[i], sample_rate_);
            }
        }

        return result;
    }

    std::pair<double, double> process(double left, double right) {
        rebuild_if_needed();

        for (auto& slot : slots_) {
            if (!slot.active) {
                continue;
            }

            switch (slot.kind) {
                case RuntimeKind::gain:
                    left *= slot.gain_scalar;
                    right *= slot.gain_scalar;
                    break;
                case RuntimeKind::tilt:
                    left = slot.state_l.process(left, slot.coefficients);
                    left = slot.state_l2.process(left, slot.coefficients2);
                    right = slot.state_r.process(right, slot.coefficients);
                    right = slot.state_r2.process(right, slot.coefficients2);
                    break;
                case RuntimeKind::biquad:
                    left = slot.state_l.process(left, slot.coefficients);
                    right = slot.state_r.process(right, slot.coefficients);
                    break;
                case RuntimeKind::none:
                    break;
            }
        }

        return { left, right };
    }

private:
    enum class RuntimeKind {
        none,
        gain,
        biquad,
        tilt
    };

    struct Slot {
        FilterSpec spec;
        RuntimeKind kind = RuntimeKind::none;
        EqBiquadCoefficients coefficients{};
        EqBiquadCoefficients coefficients2{};
        EqBiquadState state_l;
        EqBiquadState state_r;
        EqBiquadState state_l2;
        EqBiquadState state_r2;
        double gain_scalar = 1.0;
        bool active = false;
    };

    void rebuild_if_needed() const {
        if (!dirty_) {
            return;
        }

        for (auto& slot : slots_) {
            if (!slot.active) {
                continue;
            }

            switch (slot.spec.type) {
                case FilterType::gain:
                    slot.kind = RuntimeKind::gain;
                    slot.gain_scalar = std::pow(10.0, slot.spec.gainDb / 20.0);
                    break;
                case FilterType::tilt:
                    slot.kind = RuntimeKind::tilt;
                    slot.coefficients = EqBiquad::low_shelf(slot.spec.pivotHz, 0.707, -slot.spec.gainDb, sample_rate_);
                    slot.coefficients2 = EqBiquad::high_shelf(slot.spec.pivotHz, 0.707, slot.spec.gainDb, sample_rate_);
                    break;
                case FilterType::low_shelf:
                case FilterType::high_shelf:
                case FilterType::peak:
                default:
                    slot.kind = RuntimeKind::biquad;
                    slot.coefficients = filter_coefficients(slot.spec, sample_rate_);
                    break;
            }
        }

        dirty_ = false;
    }

    mutable double sample_rate_ = EqCurveGrid::default_sample_rate;
    mutable bool dirty_ = true;
    mutable std::array<Slot, max_filters> slots_{};
};
