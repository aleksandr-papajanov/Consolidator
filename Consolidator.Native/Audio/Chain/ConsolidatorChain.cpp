#include "ConsolidatorChain.h"

namespace consolidator::audio
{

void ConsolidatorChain::Prepare(const ProcessingContext& context)
{
    input_.Prepare(context);
    saturator_.Prepare(context);
    shaper_.Prepare(context);
    compressor_.Prepare(context);
    equalizer_.Prepare(context);
    polish_.Prepare(context);
    output_.Prepare(context);
}

void ConsolidatorChain::Reset() noexcept
{
    input_.Reset();
    saturator_.Reset();
    shaper_.Reset();
    compressor_.Reset();
    equalizer_.Reset();
    polish_.Reset();
    output_.Reset();
}

void ConsolidatorChain::SetParameters(const ChainParameters& parameters) noexcept
{
    input_.SetParameters(parameters.input);
    saturator_.SetParameters(parameters.saturator);
    shaper_.SetParameters(parameters.shaper);
    compressor_.SetParameters(parameters.compressor);
    equalizer_.SetParameters(parameters.equalizer);
    polish_.SetParameters(parameters.polish);
    output_.SetParameters(parameters.output);
}

void ConsolidatorChain::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    input_.Process(audio, mode);
    saturator_.Process(audio, mode);
    shaper_.Process(audio, mode);
    compressor_.Process(audio, mode);
    equalizer_.Process(audio, mode);
    polish_.Process(audio, mode);
    output_.Process(audio, mode);
}

ChainTelemetry ConsolidatorChain::Telemetry() const noexcept
{
    return { input_.Telemetry(), output_.Telemetry(), compressor_.ReductionDb(), output_.LimiterReductionDb() };
}

} // namespace consolidator::audio
