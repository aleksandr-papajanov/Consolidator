#pragma once

#include "Analysis/AnalysisView.h"
#include "Analysis/CurveInput.h"
#include "Analysis/FrequencyResponse/FrequencyResponseTypes.h"

namespace consolidator::analysis
{

struct EqualizerCurveRequest
{
    std::array<FrequencyResponseRequest, 7> filters{};
    FrequencyResponseRequest combined;
    FrequencyResponseRequest allBanksCombined;
};

struct DetectorCurveRequest
{
    std::array<FrequencyResponseRequest, 2> filters{};
    FrequencyResponseRequest combined;
};

struct AnalysisCurveRequest
{
    EqualizerCurveRequest equalizer;
    DetectorCurveRequest compressorDetector;
    DetectorCurveRequest saturatorDetector;
};

// Builds individual filter requests and one combined request for the view bank.
class FrequencyResponseRequestBuilder final
{
  public:
    [[nodiscard]] AnalysisCurveRequest Build(
        const CurveInput& input,
        AnalysisView view) const noexcept;
};

} // namespace consolidator::analysis
