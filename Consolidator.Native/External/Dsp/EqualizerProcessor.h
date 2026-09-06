#pragma once

#include <array>
#include <cstddef>

#include "DspBiquad.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class EqualizerProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    static constexpr std::size_t kBankCount = 7;
    static constexpr std::size_t kFilterCount = 7;

    std::array<DspBiquadCascade, kBankCount * kFilterCount> filters_{};
};

} // namespace consolidator::max::dsp
