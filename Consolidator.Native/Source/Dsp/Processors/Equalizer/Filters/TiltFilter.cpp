#include "Dsp/Processors/Equalizer/Filters/TiltFilter.h"

namespace consolidator::dsp
{

TiltFilter::TiltFilter(
    FilterId FilterId,
    double pivotHz)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(FilterId)),
      lowShelf_(FilterId, pivotHz),
      highShelf_(FilterId, pivotHz)
{
    InitializeParameters(
        pivotHz,
        core::settings::FilterDefaults::kDefaultQ,
        core::settings::FilterDefaults::kDefaultGainDb);

    RecalculateCoefficients();
}

void TiltFilter::Prepare(double sampleRate, std::size_t channelCount)
{
    Filter::Prepare(sampleRate, channelCount);
    lowShelf_.Prepare(sampleRate, channelCount);
    highShelf_.Prepare(sampleRate, channelCount);

    ApplyInternalParameters();
    Reset();
}

void TiltFilter::Reset() noexcept
{
    Filter::Reset();
    lowShelf_.Reset();
    highShelf_.Reset();
}

double TiltFilter::ProcessSample(
    double input,
    std::size_t channel) noexcept
{
    const double lowShelfOutput = lowShelf_.ProcessSample(input, channel);

    return highShelf_.ProcessSample(
        lowShelfOutput,
        channel);
}

void TiltFilter::RecalculateCoefficients()
{
    ApplyInternalParameters();
}

bool TiltFilter::CalculateIsNeutral() const noexcept
{
    return runtimeState_.gainDb == 0.0;
}

void TiltFilter::ApplyInternalParameters()
{
    const auto filterNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Filter1) +
        detail::ToIndex(GetFilterId()));

    const auto applyParameter = [filterNode](
                                    Filter& filter,
                                    ParameterId parameterId,
                                    float value)
    {
        const core::StatePath route{
            DeviceId::Equalizer,
            parameterId,
            filterNode};

        filter.ApplyParameter(route, ParameterVariant{value}, 1);
    };

    const float frequencyHz = runtimeState_.frequencyHz;
    const float q = runtimeState_.q;
    const float halfGainDb = runtimeState_.gainDb * 0.5f;

    applyParameter(lowShelf_, ParameterId::Frequency, frequencyHz);
    applyParameter(lowShelf_, ParameterId::Q, q);
    applyParameter(lowShelf_, ParameterId::Gain, -halfGainDb);

    applyParameter(highShelf_, ParameterId::Frequency, frequencyHz);
    applyParameter(highShelf_, ParameterId::Q, q);
    applyParameter(highShelf_, ParameterId::Gain, halfGainDb);
}

} // namespace consolidator::dsp
