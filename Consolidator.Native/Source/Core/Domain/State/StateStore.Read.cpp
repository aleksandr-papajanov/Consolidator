#include "Core/Domain/State/StateStore.Internal.h"

namespace consolidator::core
{

namespace
{

void AppendFilterState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    StatePath basePath,
    const dsp::FilterState& state)
{
    detail::AppendParameter(
        query,
        snapshot,
        basePath.WithParameter(dsp::ParameterId::Frequency),
        state.frequencyHz);
    detail::AppendParameter(
        query,
        snapshot,
        basePath.WithParameter(dsp::ParameterId::Q),
        state.q);
    detail::AppendParameter(
        query,
        snapshot,
        basePath.WithParameter(dsp::ParameterId::Gain),
        state.gainDb);
    detail::AppendParameter(
        query,
        snapshot,
        basePath.WithParameter(dsp::ParameterId::Bypass),
        state.bypass);
}

void AppendGainState(
    const StatePath& query,
    StateResponseEntries& snapshot,
    dsp::DeviceId deviceId,
    const dsp::GainState& state)
{
    detail::AppendParameter(
        query,
        snapshot,
        StatePath{deviceId, dsp::ParameterId::Gain},
        state.gainDb);
    detail::AppendParameter(
        query,
        snapshot,
        StatePath{deviceId, dsp::ParameterId::Bypass},
        state.bypass);
}

} // namespace

void StateStore::ReadState(
    const StatePath& path,
    StateResponseEntries& snapshot) const
{
    topology_.ReadState(path, snapshot);
    AppendGainState(path, snapshot, dsp::DeviceId::MainInputGain, chain_.inputGain);
    AppendGainState(path, snapshot, dsp::DeviceId::MainOutputGain, chain_.outputGain);

    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Drive}, chain_.saturator.drive);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Gain}, chain_.saturator.outputDb);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Mix}, chain_.saturator.mix);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Type}, chain_.saturator.detectorAmount);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Bypass}, chain_.saturator.bypass);

    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Threshold}, chain_.compressor.thresholdDb);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Ratio}, chain_.compressor.ratio);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Attack}, chain_.compressor.attackMs);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Release}, chain_.compressor.releaseMs);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Gain}, chain_.compressor.outputDb);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Mix}, chain_.compressor.mix);
    detail::AppendParameter(path, snapshot, StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Bypass}, chain_.compressor.bypass);

    for (std::size_t bankIndex = 0; bankIndex < chain_.equalizers.size(); ++bankIndex)
    {
        const auto bankNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) + bankIndex);
        detail::AppendParameter(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Equalizer, dsp::ParameterId::Bypass, bankNode},
            chain_.equalizers[bankIndex].bypass);

        for (std::size_t filterIndex = 0;
             filterIndex < chain_.equalizerFilters[bankIndex].size();
             ++filterIndex)
        {
            const auto filterNode = static_cast<dsp::RouteNodeId>(
                static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
            AppendFilterState(
                path,
                snapshot,
                StatePath{dsp::DeviceId::Equalizer, dsp::ParameterId::Gain, bankNode, filterNode},
                chain_.equalizerFilters[bankIndex][filterIndex]);
        }
    }

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.saturatorDetectorFilters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Saturator, dsp::ParameterId::Gain, dsp::RouteNodeId::Detector, filterNode},
            chain_.saturatorDetectorFilters[filterIndex]);
    }

    for (std::size_t filterIndex = 0;
         filterIndex < chain_.compressorDetectorFilters.size();
         ++filterIndex)
    {
        const auto filterNode = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
        AppendFilterState(
            path,
            snapshot,
            StatePath{dsp::DeviceId::Compressor, dsp::ParameterId::Gain, dsp::RouteNodeId::Detector, filterNode},
            chain_.compressorDetectorFilters[filterIndex]);
    }
}

} // namespace consolidator::core
