#include "Core/Domain/State/StateStore.h"

#include "Core/Settings/DspDeviceSettings.h"

namespace consolidator::core
{

namespace
{

dsp::FilterState MakeFilterState(const core::settings::FilterSettings& settings)
{
    return dsp::FilterState{
        dsp::ParameterState<float>{
            dsp::ParameterId::Frequency,
            static_cast<float>(settings.frequencyHz.defaultValue),
            static_cast<float>(settings.frequencyHz.min),
            static_cast<float>(settings.frequencyHz.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Q,
            static_cast<float>(settings.q.defaultValue),
            static_cast<float>(settings.q.min),
            static_cast<float>(settings.q.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Gain,
            static_cast<float>(settings.gainDb.defaultValue),
            static_cast<float>(settings.gainDb.min),
            static_cast<float>(settings.gainDb.max)},
        dsp::StateMarker<bool>{settings.bypass.defaultValue},
        dsp::StateMarker<bool>{false}};
}

dsp::GainState MakeGainState(const core::settings::GainSettings& settings)
{
    return dsp::GainState{
        dsp::ParameterState<float>{
            dsp::ParameterId::Gain,
            static_cast<float>(settings.gainDb.defaultValue),
            static_cast<float>(settings.gainDb.min),
            static_cast<float>(settings.gainDb.max)},
        dsp::StateMarker<bool>{settings.bypass.defaultValue}};
}

dsp::SaturatorState MakeSaturatorState(
    const core::settings::SaturatorSettings& settings)
{
    return dsp::SaturatorState{
        dsp::ParameterState<float>{
            dsp::ParameterId::Drive,
            static_cast<float>(settings.drive.defaultValue),
            static_cast<float>(settings.drive.min),
            static_cast<float>(settings.drive.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Gain,
            static_cast<float>(settings.outputDb.defaultValue),
            static_cast<float>(settings.outputDb.min),
            static_cast<float>(settings.outputDb.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Mix,
            static_cast<float>(settings.mix.defaultValue),
            static_cast<float>(settings.mix.min),
            static_cast<float>(settings.mix.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Type,
            1.0f,
            0.0f,
            8.0f},
        dsp::StateMarker<bool>{settings.bypass.defaultValue},
        dsp::StateMarker<bool>{false},
        {}};
}

dsp::CompressorState MakeCompressorState(
    const core::settings::CompressorSettings& settings)
{
    return dsp::CompressorState{
        dsp::ParameterState<float>{
            dsp::ParameterId::Threshold,
            static_cast<float>(settings.thresholdDb.defaultValue),
            static_cast<float>(settings.thresholdDb.min),
            static_cast<float>(settings.thresholdDb.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Ratio,
            static_cast<float>(settings.ratio.defaultValue),
            static_cast<float>(settings.ratio.min),
            static_cast<float>(settings.ratio.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Attack,
            static_cast<float>(settings.attackMs.defaultValue),
            static_cast<float>(settings.attackMs.min),
            static_cast<float>(settings.attackMs.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Release,
            static_cast<float>(settings.releaseMs.defaultValue),
            static_cast<float>(settings.releaseMs.min),
            static_cast<float>(settings.releaseMs.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Gain,
            static_cast<float>(settings.outputDb.defaultValue),
            static_cast<float>(settings.outputDb.min),
            static_cast<float>(settings.outputDb.max)},
        dsp::ParameterState<float>{
            dsp::ParameterId::Mix,
            static_cast<float>(settings.mix.defaultValue),
            static_cast<float>(settings.mix.min),
            static_cast<float>(settings.mix.max)},
        dsp::StateMarker<bool>{settings.bypass.defaultValue},
        dsp::StateMarker<bool>{false},
        {}};
}

} // namespace

ChainState MakeChainState(const core::settings::DspSettings& settings)
{
    static_assert(
        core::settings::DspSettings::kBankCount == InstanceState::kBankCount);
    static_assert(
        core::settings::EqualizerSettings::kBandCount == 7);
    static_assert(
        core::settings::DetectorFilterSettings::kBandCount == 2);

    ChainState chain{
        MakeGainState(settings.inputGain),
        MakeSaturatorState(settings.saturator),
        MakeCompressorState(settings.compressor),
        dsp::EqualizerState{
            dsp::StateMarker<bool>{false},
            dsp::StateMarker<bool>{false}},
        {},
        MakeGainState(settings.outputGain)};

    for (std::size_t bankIndex = 0;
         bankIndex < settings.banks.size();
         ++bankIndex)
    {
        chain.equalizers[bankIndex].bypass =
            dsp::StateMarker<bool>{false};
        chain.equalizers[bankIndex].solo =
            dsp::StateMarker<bool>{false};
        for (std::size_t filterIndex = 0;
             filterIndex < settings.banks[bankIndex].bands.size();
             ++filterIndex)
        {
            chain.equalizers[bankIndex].filters[filterIndex] =
                MakeFilterState(settings.banks[bankIndex].bands[filterIndex]);
        }
    }

    for (std::size_t filterIndex = 0;
         filterIndex < settings.saturator.detector.bands.size();
         ++filterIndex)
    {
        chain.saturator.detector.filters[filterIndex] =
            MakeFilterState(settings.saturator.detector.bands[filterIndex]);
    }

    for (std::size_t filterIndex = 0;
         filterIndex < settings.compressor.detector.bands.size();
         ++filterIndex)
    {
        chain.compressor.detector.filters[filterIndex] =
            MakeFilterState(settings.compressor.detector.bands[filterIndex]);
    }

    return chain;
}

} // namespace consolidator::core
