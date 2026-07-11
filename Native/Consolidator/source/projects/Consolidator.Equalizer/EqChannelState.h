#pragma once

#include "EqBiquad.h"
#include "EqFilterBank.h"

#include <array>

class EqChannelState {
public:
    void reset() {
        for (auto& state : states_) {
            state.reset();
        }
    }

    double process(double input, const EqFilterBank& bank) {
        const auto& coeffs = bank.coefficients();

        for (int i = 0; i < EqFilterBank::filter_count; ++i) {
            input = states_[i].process(input, coeffs[i]);
        }

        return input;
    }

private:
    std::array<EqBiquadState, EqFilterBank::filter_count> states_{};
};
