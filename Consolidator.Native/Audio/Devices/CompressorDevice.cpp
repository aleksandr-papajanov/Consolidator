#include "CompressorDevice.h"

#include "../Primitives/Math.h"

namespace consolidator::audio::devices
{

void CompressorDevice::Prepare(const ProcessingContext& context)
{
    detector_.Prepare(context);
    cla76_.Prepare(context);
    cla3a_.Prepare(context);
    cla2a_.Prepare(context);
    ssl_.Prepare(context);
    transition_.Prepare(context);
}

void CompressorDevice::ResetModels() noexcept
{
    cla76_.Reset();
    cla3a_.Reset();
    cla2a_.Reset();
    ssl_.Reset();
    reductionDb_ = 0.0;
    resetPending_ = false;
}

void CompressorDevice::Reset() noexcept
{
    detector_.Reset();
    ResetModels();
    transition_.Reset();
}

void CompressorDevice::SetParameters(const CompressorParameters& parameters) noexcept
{
    resetPending_ = resetPending_ || algorithm_ != parameters.algorithm;
    algorithm_ = parameters.algorithm;
    bypass_ = parameters.bypass;
    detector_.SetParameters(parameters.detector);
    const auto output = primitives::DbToGain(parameters.outputDb);
    switch (algorithm_)
    {
        case CompressorAlgorithm::Ssl:
        {
            algorithms::SslChannelControls controls;
            controls.reduction = parameters.reduction;
            controls.outputGain = output;
            controls.attack = parameters.attack;
            controls.release = parameters.release;
            controls.ratio = parameters.ratio;
            ssl_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Cla76:
        {
            algorithms::Cla76Controls controls;
            controls.reduction = parameters.reduction;
            controls.outputGain = output;
            controls.attack = parameters.attack;
            controls.release = parameters.release;
            controls.ratio = parameters.ratio;
            cla76_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Cla3a:
        {
            algorithms::Cla3aControls controls;
            controls.reduction = parameters.reduction;
            controls.outputGain = output;
            cla3a_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Cla2a:
        {
            algorithms::Cla2aControls controls;
            controls.reduction = parameters.reduction;
            controls.outputGain = output;
            cla2a_.SetControls(controls);
            break;
        }
    }
    // Zero Reduction disables new gain reduction without switching the device to Bypass.
    transition_.SetTarget(parameters.bypass ? 0.0 : (parameters.parallel ? 0.5 : 1.0));
}

void CompressorDevice::ProcessWet(AudioBuffer audio, ChannelMode mode) noexcept
{
    if (resetPending_)
    {
        // Only the selected model runs; switching discards old histories, as in DeviceCompressor.
        ResetModels();
    }
    const auto detector = detector_.Capture(audio, mode);
    switch (algorithm_)
    {
        case CompressorAlgorithm::Ssl:
        {
            ssl_.Process(audio, detector, mode);
            reductionDb_ = ssl_.ReductionDb();
            break;
        }
        case CompressorAlgorithm::Cla76:
        {
            cla76_.Process(audio, detector, mode);
            reductionDb_ = cla76_.ReductionDb();
            break;
        }
        case CompressorAlgorithm::Cla3a:
        {
            cla3a_.Process(audio, detector, mode);
            reductionDb_ = cla3a_.ReductionDb();
            break;
        }
        case CompressorAlgorithm::Cla2a:
        {
            cla2a_.Process(audio, detector, mode);
            reductionDb_ = cla2a_.ReductionDb();
            break;
        }
    }
}

void CompressorDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    transition_.Process(audio, mode, [&](AudioBuffer wet)
    {
        ProcessWet(wet, mode);
    }, [&]()
    {
        detector_.Reset();
        ResetModels();
    });
}

double CompressorDevice::ReductionDb() const noexcept
{
    return bypass_ ? 0.0 : reductionDb_;
}

} // namespace consolidator::audio::devices
