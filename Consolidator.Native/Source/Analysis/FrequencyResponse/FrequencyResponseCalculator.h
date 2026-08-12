#pragma once

#include "Analysis/FrequencyResponse/FrequencyResponseTypes.h"

namespace consolidator::analysis
{

// Calculates theoretical cascaded biquad magnitude response on a display grid.
class FrequencyResponseCalculator final
{
public:
    void Calculate(
        const FrequencyResponseRequest& request,
        FrequencyResponseSnapshot& output) const noexcept;
};

} // namespace consolidator::analysis
