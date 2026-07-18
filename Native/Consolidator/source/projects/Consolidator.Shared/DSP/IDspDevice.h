#pragma once

#include <span>

namespace consolidator::dsp {

class IDspDevice {
public:
    virtual ~IDspDevice() = default;

    virtual double ProcessSample(double input) = 0;

    virtual void ProcessBlock(std::span<double> samples) {
        for (double& sample : samples) {
            sample = ProcessSample(sample);
        }
    }

    virtual void Reset() = 0;
};

} // namespace consolidator::dsp
