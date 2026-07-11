#pragma once

#include "EqChannelState.h"
#include "EqFilterBank.h"
#include "EqParams.h"

#include <utility>

class EqProcessor {
public:
    void set_sample_rate(double sample_rate) {
        bank_.set_sample_rate(sample_rate);
    }

    void set_params(const EqParams& params) {
        bank_.set_params(params);
    }

    void reset() {
        left_.reset();
        right_.reset();
    }

    std::pair<double, double> process(double left, double right) {
        left *= bank_.input_gain();
        right *= bank_.input_gain();

        left = left_.process(left, bank_);
        right = right_.process(right, bank_);

        return { left, right };
    }

private:
    EqFilterBank bank_;
    EqChannelState left_;
    EqChannelState right_;
};
