#include "SnapshotBinding.h"

#include <cstddef>

namespace consolidator::audio::runtime
{

namespace
{

FilterParameters BindFilter(const transport::FilterSnapshot& filter) noexcept
{
    return { filter.active, filter.type, filter.frequencyHz, filter.gainDb, filter.q, filter.fixedQ };
}

} // namespace

ChainParameters BindSnapshot(const transport::DspSnapshot& snapshot) noexcept
{
    ChainParameters parameters{};
    parameters.input.width = snapshot.inputWidth;
    parameters.input.bypass = snapshot.inputGainBypass != 0;
    parameters.input.gain.levelDb = snapshot.inputLevel;
    parameters.input.gain.targetDb = snapshot.inputTarget;
    parameters.input.gain.leveler = snapshot.inputLeveler != 0;

    parameters.output.bypass = snapshot.outputGainBypass != 0;
    parameters.output.gain.levelDb = snapshot.outputLevel;
    parameters.output.gain.targetDb = snapshot.outputTarget;
    parameters.output.gain.leveler = snapshot.outputLeveler != 0;
    parameters.output.ceilingDb = snapshot.outputLimit;

    parameters.saturator.drive = snapshot.saturatorDrive;
    parameters.saturator.curve = snapshot.saturatorCurve;
    parameters.saturator.outputDb = snapshot.saturatorOutputDb;
    parameters.saturator.split = snapshot.saturatorSplit != 0;
    parameters.saturator.bypass = snapshot.saturatorBypass != 0;

    parameters.shaper.attack = snapshot.compressorAttack;
    parameters.shaper.sustain = snapshot.compressorSustain;
    parameters.shaper.outputDb = snapshot.compressorOutputDb;
    parameters.shaper.bypass = snapshot.dynamicsShaperBypass != 0;

    parameters.compressor.algorithm = static_cast<devices::CompressorAlgorithm>(snapshot.compressorCharacter);
    parameters.compressor.reduction = snapshot.compressorCompression;
    parameters.compressor.outputDb = snapshot.dynamicsCompressionOutputDb;
    parameters.compressor.attack = snapshot.dynamicsCompressionAttack;
    parameters.compressor.release = snapshot.dynamicsCompressionRelease;
    parameters.compressor.ratio = snapshot.dynamicsCompressionRatio;
    parameters.compressor.parallel = snapshot.compressorParallel != 0;
    parameters.compressor.bypass = snapshot.dynamicsCompressionBypass != 0;

    for (std::size_t index = 0; index < 2; ++index)
    {
        parameters.input.gain.detector[index] = BindFilter(snapshot.detectorFilters[index]);
        parameters.saturator.detector[index] = BindFilter(snapshot.detectorFilters[2 + index]);
        parameters.compressor.detector[index] = BindFilter(snapshot.detectorFilters[4 + index]);
        parameters.output.gain.detector[index] = BindFilter(snapshot.outputDetectorFilters[index]);
    }
    parameters.equalizer.bypass = snapshot.equalizerBypass != 0;
    for (std::size_t index = 0; index < parameters.equalizer.banks.size(); ++index)
    {
        parameters.equalizer.banks[index] = snapshot.equalizerBanksActive[index] != 0;
    }
    for (std::size_t index = 0; index < parameters.equalizer.filters.size(); ++index)
    {
        parameters.equalizer.filters[index] = BindFilter(snapshot.equalizerFilters[index]);
    }
    parameters.polish.thick = snapshot.polishThick;
    parameters.polish.air = snapshot.polishAir;
    parameters.polish.bypass = snapshot.polishBypass != 0;
    return parameters;
}

} // namespace consolidator::audio::runtime
