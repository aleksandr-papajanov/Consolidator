#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "DspBiquad.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class DetectorProcessor final : public DspProcessor
{
public:
    explicit DetectorProcessor(std::size_t filterOffset) noexcept;

    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    static constexpr std::size_t kFilterCount = 2;

    std::size_t filterOffset_{};
    std::array<DspBiquadCascade, kFilterCount> filters_{};
    std::vector<double> left_;
    std::vector<double> right_;
};

} // namespace consolidator::max::dsp
