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
    double gainDb)
{
    if (request.stageCount >= request.stages.size() || state.bypass)
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

EqualizerCurveRequest FrequencyResponseRequestBuilder::Build(
    const CurveInput& input,
    AnalysisView view) const noexcept
{
    EqualizerCurveRequest result;
    for (auto& request : result.filters)
    {
        request.sampleRate = input.sampleRate;
        request.revision = input.revision;
    }
    result.combined.sampleRate = input.sampleRate;
    result.combined.revision = input.revision;
    result.allBanksCombined.sampleRate = input.sampleRate;
    result.allBanksCombined.revision = input.revision;
    if (!input.equalizerActive || !input.chainAllowsEqualizer ||
        input.sampleRate <= 0.0)
    {
        return result;
    }

    bool bankSolo = false;
    for (const auto& candidate : input.banks)
    {
        bankSolo = bankSolo || candidate.solo;
    }

    constexpr auto& kinds = dsp::kStandardEqualizerLayout;
    for (const auto& candidate : input.banks)
    {
        AppendBankFilters(
            result.allBanksCombined,
            candidate,
            bankSolo,
            input.sampleRate,
            kinds);
    }

    const auto bankIndex = static_cast<std::size_t>(view.bankId);
    if (bankIndex >= input.banks.size())
    {
        return result;
    }

    const auto& bank = input.banks[bankIndex];
    if (bank.bypass || (bankSolo && !bank.solo))
    {
        return result;
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
        auto& filterRequest = result.filters[index];
        if (kinds[index] == dsp::EqualizerFilterKind::Tilt)
        {
            AppendFilter(
                filterRequest,
                dsp::BiquadType::LowShelf,
                filter,
                input.sampleRate,
                -filter.gainDb * 0.5);
            AppendFilter(
                filterRequest,
                dsp::BiquadType::HighShelf,
                filter,
                input.sampleRate,
                filter.gainDb * 0.5);
            AppendFilter(result.combined, dsp::BiquadType::LowShelf, filter,
                         input.sampleRate, -filter.gainDb * 0.5);
            AppendFilter(result.combined, dsp::BiquadType::HighShelf, filter,
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
            AppendFilter(filterRequest, kind, filter, input.sampleRate,
                         filter.gainDb);
            AppendFilter(result.combined, kind, filter, input.sampleRate,
                         filter.gainDb);
        }
    }
    return result;
}

} // namespace consolidator::analysis
