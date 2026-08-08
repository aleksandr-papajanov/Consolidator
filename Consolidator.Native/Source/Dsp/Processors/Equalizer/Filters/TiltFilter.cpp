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
    if (GetState().bypass)
    {
        return input;
    }

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
    return state_.bypass || state_.gainDb == 0.0;
}

void TiltFilter::ApplyInternalParameters()
{
    const auto& parameters = GetState();
    const auto filterNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Filter1) +
        detail::ToIndex(GetFilterId()));

    const auto applyParameter = [filterNode](
                                    Filter& filter,
                                    ParameterId parameterId,
                                    float value)
    {
        const ParameterRoute route{
            DeviceId::Equalizer,
            parameterId,
            filterNode};

        filter.ApplyParameter(route, ParameterValue{value}, 1);
    };

    const float frequencyHz = static_cast<float>(parameters.frequencyHz);
    const float q = static_cast<float>(parameters.q);
    const float halfGainDb = static_cast<float>(parameters.gainDb * 0.5);

    applyParameter(lowShelf_, ParameterId::Frequency, frequencyHz);
    applyParameter(lowShelf_, ParameterId::Q, q);
    applyParameter(lowShelf_, ParameterId::Gain, -halfGainDb);

    applyParameter(highShelf_, ParameterId::Frequency, frequencyHz);
    applyParameter(highShelf_, ParameterId::Q, q);
    applyParameter(highShelf_, ParameterId::Gain, halfGainDb);
}

} // namespace consolidator::dsp
