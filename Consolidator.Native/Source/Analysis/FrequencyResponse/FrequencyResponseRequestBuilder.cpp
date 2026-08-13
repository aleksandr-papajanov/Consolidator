#include "Analysis/FrequencyResponse/FrequencyResponseRequestBuilder.h"

#include <algorithm>

#include "Dsp/Processors/Equalizer/Filters/BiquadDesigner.h"
#include "Dsp/Processors/Equalizer/EqualizerLayout.h"

namespace consolidator::analysis
{

namespace
{

BiquadResponseSection MakeSection(
    dsp::BiquadType kind,
    const CurveFilterState& state,
    double sampleRate,
    double gainDb)
{
    const auto coefficients = dsp::BiquadDesigner::Calculate(
        kind, state.frequencyHz, state.q, gainDb, sampleRate);
    return {coefficients.b0, coefficients.b1, coefficients.b2,
            coefficients.a1, coefficients.a2};
}

void AppendFilter(
    FrequencyResponseRequest& request,
    dsp::BiquadType kind,
    const CurveFilterState& state,
    double sampleRate,
    double gainDb,
    bool respectBypass = true)
{
    if (request.stageCount >= request.stages.size() ||
        (respectBypass && state.bypass))
    {
        return;
    }
    request.stages[request.stageCount++] = MakeSection(
        kind, state, sampleRate, gainDb);
}

void AppendBankFilters(
    FrequencyResponseRequest& request,
    const CurveBankState& bank,
    bool bankSolo,
    double sampleRate,
    const std::array<dsp::EqualizerFilterKind, 7>& kinds)
{
    if (bank.bypass || (bankSolo && !bank.solo))
    {
        return;
    }

    bool filterSolo = false;
    for (const auto& filter : bank.filters)
    {
        filterSolo = filterSolo || filter.solo;
    }

    for (std::size_t index = 0; index < bank.filters.size(); ++index)
    {
        const auto& filter = bank.filters[index];
        if (filterSolo && !filter.solo)
        {
            continue;
        }

        if (kinds[index] == dsp::EqualizerFilterKind::Tilt)
        {
            AppendFilter(request, dsp::BiquadType::LowShelf, filter,
                         sampleRate, -filter.gainDb * 0.5);
            AppendFilter(request, dsp::BiquadType::HighShelf, filter,
                         sampleRate, filter.gainDb * 0.5);
        }
        else
        {
            const auto kind = kinds[index] == dsp::EqualizerFilterKind::Gain
                ? dsp::BiquadType::Gain
                : kinds[index] == dsp::EqualizerFilterKind::LowShelf
                    ? dsp::BiquadType::LowShelf
                    : kinds[index] == dsp::EqualizerFilterKind::HighShelf
                        ? dsp::BiquadType::HighShelf
                        : dsp::BiquadType::Bell;
            AppendFilter(request, kind, filter, sampleRate, filter.gainDb);
        }
    }
}

} // namespace

AnalysisCurveRequest FrequencyResponseRequestBuilder::Build(
    const CurveInput& input,
    AnalysisView view) const noexcept
{
    AnalysisCurveRequest result;
    auto initializeDetector = [&](DetectorCurveRequest& detector)
    {
        for (auto& request : detector.filters)
        {
            request.sampleRate = input.sampleRate;
            request.revision = input.revision;
        }
        detector.combined.sampleRate = input.sampleRate;
        detector.combined.revision = input.revision;
    };
    for (auto& request : result.equalizer.filters)
    {
        request.sampleRate = input.sampleRate;
        request.revision = input.revision;
    }
    result.equalizer.combined.sampleRate = input.sampleRate;
    result.equalizer.combined.revision = input.revision;
    result.equalizer.allBanksCombined.sampleRate = input.sampleRate;
    result.equalizer.allBanksCombined.revision = input.revision;
    initializeDetector(result.compressorDetector);
    initializeDetector(result.saturatorDetector);
    if (input.equalizerActive && input.sampleRate > 0.0)
    {
        bool bankSolo = false;
        for (const auto& candidate : input.banks)
            bankSolo = bankSolo || candidate.solo;

        constexpr auto& kinds = dsp::kStandardEqualizerLayout;
        for (const auto& candidate : input.banks)
        {
            AppendBankFilters(result.equalizer.allBanksCombined, candidate,
                              bankSolo, input.sampleRate, kinds);
        }

        const auto bankIndex = static_cast<std::size_t>(view.bankId);
        if (bankIndex < input.banks.size())
        {
            const auto& bank = input.banks[bankIndex];
            if (!bank.bypass && !(bankSolo && !bank.solo))
            {
                bool filterSolo = false;
                for (const auto& filter : bank.filters)
                    filterSolo = filterSolo || filter.solo;
                for (std::size_t index = 0; index < bank.filters.size(); ++index)
                {
                    const auto& filter = bank.filters[index];
                    if (filterSolo && !filter.solo)
                        continue;
                    auto& filterRequest = result.equalizer.filters[index];
                    if (kinds[index] == dsp::EqualizerFilterKind::Tilt)
                    {
                        AppendFilter(filterRequest, dsp::BiquadType::LowShelf,
                                     filter, input.sampleRate, -filter.gainDb * 0.5);
                        AppendFilter(filterRequest, dsp::BiquadType::HighShelf,
                                     filter, input.sampleRate, filter.gainDb * 0.5);
                        AppendFilter(result.equalizer.combined,
                                     dsp::BiquadType::LowShelf, filter,
                                     input.sampleRate, -filter.gainDb * 0.5);
                        AppendFilter(result.equalizer.combined,
                                     dsp::BiquadType::HighShelf, filter,
                                     input.sampleRate, filter.gainDb * 0.5);
                    }
                    else
                    {
                        const auto kind = kinds[index] == dsp::EqualizerFilterKind::Gain
                            ? dsp::BiquadType::Gain
                            : kinds[index] == dsp::EqualizerFilterKind::LowShelf
                                ? dsp::BiquadType::LowShelf
                                : kinds[index] == dsp::EqualizerFilterKind::HighShelf
                                    ? dsp::BiquadType::HighShelf
                                    : dsp::BiquadType::Bell;
                        AppendFilter(filterRequest, kind, filter,
                                     input.sampleRate, filter.gainDb);
                        AppendFilter(result.equalizer.combined, kind, filter,
                                     input.sampleRate, filter.gainDb);
                    }
                }
            }
        }
    }
    auto appendDetector = [&](DetectorCurveRequest& request,
                              const DetectorCurveState& state,
                              const auto& kinds)
    {
        if (!state.active || input.sampleRate <= 0.0)
            return;
        for (std::size_t index = 0; index < state.filters.size(); ++index)
        {
            const auto& filter = state.filters[index];
            const auto kind = kinds[index] == dsp::DetectorFilterKind::LowShelf
                ? dsp::BiquadType::LowShelf : dsp::BiquadType::Bell;
            AppendFilter(request.filters[index], kind, filter,
                          input.sampleRate, filter.gainDb, false);
            AppendFilter(request.combined, kind, filter,
                          input.sampleRate, filter.gainDb);
        }
    };
    appendDetector(result.compressorDetector, input.compressorDetector,
                   dsp::kCompressorDetectorLayout);
    appendDetector(result.saturatorDetector, input.saturatorDetector,
                   dsp::kSaturatorDetectorLayout);
    return result;
}

} // namespace consolidator::analysis
