#pragma once

#include "EqFilterBank.h"
#include "EqParams.h"

#include <vector>

class EqModel {
public:
    static std::vector<double> buildCurve(
        const std::vector<double>& freqs,
        const EqParams& p,
        double sampleRate = 48000.0
    ) {
        EqFilterBank bank;
        bank.set_sample_rate(sampleRate);
        bank.set_params(p);
        return bank.response_curve(freqs);
    }
};
