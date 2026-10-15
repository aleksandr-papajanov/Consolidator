#include "DeviceCompressorProcessor.h"

namespace consolidator::max::dsp::processors
{

void DeviceCompressorProcessor::Prepare(const DspProcessingContext& context)
{
    adaptiveGain_.Prepare(context);
    cla76_.Prepare(context);
    cla3a_.Prepare(context);
    cla2a_.Prepare(context);
    ssl_.Prepare(context);
    ApplyActiveControls();
    Reset();
}

void DeviceCompressorProcessor::Reset() noexcept
{
    cla76_.Reset();
    cla3a_.Reset();
    cla2a_.Reset();
    ssl_.Reset();
    adaptiveGain_.Reset(controls_.outputGain);
    resetPending_ = false;
}

void DeviceCompressorProcessor::SetControls(const DeviceCompressorControls& controls) noexcept
{
    resetPending_ = resetPending_ || controls.algorithm != controls_.algorithm;
    controls_ = controls;
    adaptiveGain_.SetControls(controls.autoGain, controls.outputGain);
    ApplyActiveControls();
}

double DeviceCompressorProcessor::OutputDb() const noexcept
{
    return adaptiveGain_.OutputDb();
}

void DeviceCompressorProcessor::ApplyActiveControls() noexcept
{
    switch (controls_.algorithm)
    {
        case CompressorAlgorithm::Cla76:
        {
            Cla76Controls controls;
            controls.reduction = controls_.reduction;
            controls.outputGain = 1.0;
            controls.attack = controls_.attack;
            controls.release = controls_.release;
            controls.ratio = controls_.ratio;
            cla76_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Cla3a:
        {
            Cla3aControls controls;
            controls.reduction = controls_.reduction;
            controls.outputGain = 1.0;
            cla3a_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Cla2a:
        {
            Cla2aControls controls;
            controls.reduction = controls_.reduction;
            controls.outputGain = 1.0;
            cla2a_.SetControls(controls);
            break;
        }
        case CompressorAlgorithm::Ssl:
        {
            SslChannelControls controls;
            controls.reduction = controls_.reduction;
            controls.outputGain = 1.0;
            controls.attack = controls_.attack < 0.5 ? 0.0 : 1.0;
            controls.release = controls_.release;
            controls.ratio = controls_.ratio;
            ssl_.SetControls(controls);
            break;
        }
    }
}

void DeviceCompressorProcessor::ResetActiveProcessor() noexcept
{
    switch (controls_.algorithm)
    {
        case CompressorAlgorithm::Cla76:
        {
            cla76_.Reset();
            break;
        }
        case CompressorAlgorithm::Cla3a:
        {
            cla3a_.Reset();
            break;
        }
        case CompressorAlgorithm::Cla2a:
        {
            cla2a_.Reset();
            break;
        }
        case CompressorAlgorithm::Ssl:
        {
            ssl_.Reset();
            break;
        }
    }
}

void DeviceCompressorProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    if (resetPending_)
    {
        // A newly selected device starts with current controls and an empty history.
        ResetActiveProcessor();
        adaptiveGain_.ResetMeasurement();
        resetPending_ = false;
    }
    adaptiveGain_.CaptureReference(audio, mode);
    switch (controls_.algorithm)
    {
        case CompressorAlgorithm::Cla76:
        {
            cla76_.Process(audio, mode);
            break;
        }
        case CompressorAlgorithm::Cla3a:
        {
            cla3a_.Process(audio, mode);
            break;
        }
        case CompressorAlgorithm::Cla2a:
        {
            cla2a_.Process(audio, mode);
            break;
        }
        case CompressorAlgorithm::Ssl:
        {
            ssl_.Process(audio, mode);
            break;
        }
    }
    adaptiveGain_.Process(audio, mode);
}

} // namespace consolidator::max::dsp::processors
