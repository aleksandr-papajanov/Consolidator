#include "Core/Analysis/AnalysisCurveInputBuilder.h"
#include "Core/Routing/ProcessingState.h"

#include <cstddef>

namespace consolidator::core
{

analysis::CurveInput AnalysisCurveInputBuilder::Build(
    const ChainState& chain,
    double sampleRate,
    std::uint64_t revision) const noexcept
{
    analysis::CurveInput input;
    const auto processing = ResolveProcessingState(chain);
    input.sampleRate = sampleRate;
    input.revision = revision;
    input.equalizerActive = processing.equalizerActive;

    const auto copyDetector = [](const auto& source, auto& target,
                                 bool deviceActive, const auto& active)
    {
        target.active = deviceActive;
        for (std::size_t index = 0; index < target.filters.size(); ++index)
        {
            const auto& sourceFilter = source.filters[index];
            auto& targetFilter = target.filters[index];
            targetFilter.frequencyHz = sourceFilter.frequencyHz.value;
            targetFilter.q = sourceFilter.q.value;
            targetFilter.gainDb = sourceFilter.gainDb.value;
            targetFilter.bypass = !active[index];
            targetFilter.solo = sourceFilter.solo.value;
        }
    };
    copyDetector(chain.compressor.detector, input.compressorDetector,
                 processing.compressorActive || chain.compressor.detector.listen.value,
                 processing.compressorDetectorFiltersActive);
    copyDetector(chain.saturator.detector, input.saturatorDetector,
                 processing.saturatorActive || chain.saturator.detector.listen.value,
                 processing.saturatorDetectorFiltersActive);

    for (std::size_t bankIndex = 0; bankIndex < input.banks.size(); ++bankIndex)
    {
        const auto& sourceBank = chain.equalizers[bankIndex];
        auto& targetBank = input.banks[bankIndex];
        targetBank.bypass = !processing.equalizerBanksActive[bankIndex];
        targetBank.solo = sourceBank.solo.value;
        for (std::size_t filterIndex = 0;
             filterIndex < targetBank.filters.size(); ++filterIndex)
        {
            const auto& source = sourceBank.filters[filterIndex];
            auto& target = targetBank.filters[filterIndex];
            target.frequencyHz = source.frequencyHz.value;
            target.q = source.q.value;
            target.gainDb = source.gainDb.value;
            target.bypass = !processing.equalizerFiltersActive[bankIndex][filterIndex];
            target.solo = source.solo.value;
        }
    }
    return input;
}

} // namespace consolidator::core
