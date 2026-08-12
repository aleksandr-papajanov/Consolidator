#include "Core/Analysis/EqualizerResponseBuilder.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::core
{

namespace
{

using analysis::BiquadResponseSection;
using dsp::FilterState;
using settings::FilterKind;

constexpr std::array<FilterKind, 7> kFilterKinds{
    FilterKind::GainFilter, FilterKind::Tilt, FilterKind::LowShelf,
    FilterKind::HighShelf, FilterKind::Bell, FilterKind::Bell,
    FilterKind::Bell};

BiquadResponseSection MakeSection(
    FilterKind kind,
    const FilterState& state,
    double sampleRate,
    double gainDb)
{
    const auto frequency = static_cast<double>(state.frequencyHz.value);
    const auto q = static_cast<double>(state.q.value);
    const auto omega = 2.0 * std::numbers::pi * frequency / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);

    if (kind == FilterKind::GainFilter)
    {
        return {std::pow(10.0, gainDb / 20.0), 0.0, 0.0, 0.0, 0.0};
    }

    if (kind == FilterKind::Bell)
    {
        const auto inverseA0 = 1.0 / (1.0 + alpha / amplitude);
        return {
            (1.0 + alpha * amplitude) * inverseA0,
            -2.0 * cosine * inverseA0,
            (1.0 - alpha * amplitude) * inverseA0,
            -2.0 * cosine * inverseA0,
            (1.0 - alpha / amplitude) * inverseA0};
    }

    const auto gain = amplitude;
    const auto gainRootTerm = 2.0 * std::sqrt(gain) * alpha;
    if (kind == FilterKind::LowShelf)
    {
        const auto inverseA0 = 1.0 / ((gain + 1.0) +
            (gain - 1.0) * cosine + gainRootTerm);
        return {
            gain * ((gain + 1.0) - (gain - 1.0) * cosine + gainRootTerm) * inverseA0,
            2.0 * gain * ((gain - 1.0) - (gain + 1.0) * cosine) * inverseA0,
            gain * ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * inverseA0,
            -2.0 * ((gain - 1.0) + (gain + 1.0) * cosine) * inverseA0,
            ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * inverseA0};
    }

    const auto inverseA0 = 1.0 / ((gain + 1.0) -
        (gain - 1.0) * cosine + gainRootTerm);
    return {
        gain * ((gain + 1.0) + (gain - 1.0) * cosine + gainRootTerm) * inverseA0,
        -2.0 * gain * ((gain - 1.0) + (gain + 1.0) * cosine) * inverseA0,
        gain * ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * inverseA0,
        2.0 * ((gain - 1.0) - (gain + 1.0) * cosine) * inverseA0,
        ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * inverseA0};
}

void AppendFilter(
    analysis::FrequencyResponseRequest& request,
    FilterKind kind,
    const FilterState& state,
    double sampleRate,
    double gainDb)
{
    if (request.stageCount >= request.stages.size() || state.bypass.value)
        return;

    request.stages[request.stageCount++] = MakeSection(
        kind, state, sampleRate, gainDb);
}

} // namespace

analysis::FrequencyResponseRequest EqualizerResponseBuilder::Build(
    const StateStore& stateStore,
    double sampleRate,
    std::uint64_t revision) const noexcept
{
    analysis::FrequencyResponseRequest request;
    request.sampleRate = sampleRate;
    request.revision = revision;
    const auto& chain = stateStore.GetChain();
    if (sampleRate <= 0.0 || chain.equalizer.bypass.value)
        return request;

    // Match ProcessingStateResolver's chain order:
    // input gain, saturator, compressor, equalizer, output gain.
    // A solo on an earlier chain device prevents EQ from being active.
    constexpr std::size_t kEqualizerChainIndex = 3;
    std::size_t lastSoloChainIndex = 5;
    if (chain.saturator.solo.value)
        lastSoloChainIndex = 1;
    if (chain.compressor.solo.value)
        lastSoloChainIndex = 2;
    if (chain.equalizer.solo.value)
        lastSoloChainIndex = kEqualizerChainIndex;
    if (lastSoloChainIndex < kEqualizerChainIndex)
        return request;

    const auto& banks = chain.equalizers;
    bool bankSolo = false;
    for (const auto& bank : banks)
        bankSolo = bankSolo || bank.solo.value;

    for (const auto& bank : banks)
    {
        if (bank.bypass.value || (bankSolo && !bank.solo.value))
            continue;

        bool filterSolo = false;
        for (const auto& filter : bank.filters)
            filterSolo = filterSolo || filter.solo.value;

        for (std::size_t index = 0; index < bank.filters.size(); ++index)
        {
            const auto& filter = bank.filters[index];
            if (filterSolo && !filter.solo.value)
                continue;

            if (kFilterKinds[index] == FilterKind::Tilt)
            {
                AppendFilter(request, FilterKind::LowShelf, filter,
                             sampleRate, -filter.gainDb.value * 0.5);
                AppendFilter(request, FilterKind::HighShelf, filter,
                             sampleRate, filter.gainDb.value * 0.5);
            }
            else
            {
                AppendFilter(request, kFilterKinds[index], filter,
                             sampleRate, filter.gainDb.value);
            }
        }
    }
    return request;
}

} // namespace consolidator::core
