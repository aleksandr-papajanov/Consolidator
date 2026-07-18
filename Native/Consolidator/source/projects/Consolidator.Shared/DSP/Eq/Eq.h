#pragma once

#include "IEqFilter.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class Eq final : public IEqFilter {
public:
    void AddFilter(std::unique_ptr<IEqFilter> filter) {
        if (filter) filters.push_back(std::move(filter));
    }

    void Clear() {
        filters.clear();
    }

    double ProcessSample(double input) override {
        for (const auto& filter : filters) {
            input = filter->ProcessSample(input);
        }
        return input;
    }

    double GetMagnitudeDb(double frequencyHz) const override {
        double magnitudeDb = 0.0;
        for (const auto& filter : filters) {
            magnitudeDb += filter->GetMagnitudeDb(frequencyHz);
        }
        return magnitudeDb;
    }

    double GetPhaseRadians(double frequencyHz) const override {
        double phase = 0.0;
        for (const auto& filter : filters) {
            phase += filter->GetPhaseRadians(frequencyHz);
        }
        return phase;
    }

    void Reset() override {
        for (const auto& filter : filters) {
            filter->Reset();
        }
    }

    std::size_t FilterCount() const {
        return filters.size();
    }

private:
    std::vector<std::unique_ptr<IEqFilter>> filters;
};

} // namespace consolidator::dsp
