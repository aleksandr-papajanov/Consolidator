#include "Dsp/Processors/Equalizer/Filters/TiltFilter.h"

#include "Dsp/Parameters/ParameterChange.h"

namespace consolidator::dsp
{

TiltFilter::TiltFilter(
    EqFilterId filterId,
    double pivotHz)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(filterId)),
      lowShelf_(filterId, pivotHz),
      highShelf_(filterId, pivotHz)
{
    InitializeParameters(
        pivotHz,
        core::settings::FilterDefaults::kDefaultQ,
        core::settings::FilterDefaults::kDefaultGainDb);

    RecalculateCoefficients();
}

void TiltFilter::Prepare(
    double sampleRate,
    std::size_t channelCount)
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
    if (GetParameters().bypass)
    {
        return input;
    }

    const double lowShelfOutput =
        lowShelf_.ProcessSample(input, channel);

    return highShelf_.ProcessSample(
        lowShelfOutput,
        channel);
}

void TiltFilter::RecalculateCoefficients()
{
    ApplyInternalParameters();
}

void TiltFilter::ApplyInternalParameters()
{
    const auto& parameters = GetParameters();
    const auto filterId = GetEqFilterId();

    lowShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterFrequency(filterId),
            ParameterValue{
                static_cast<float>(parameters.frequencyHz)}});

    lowShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterQ(filterId),
            ParameterValue{
                static_cast<float>(parameters.q)}});

    lowShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterGain(filterId),
            ParameterValue{
                static_cast<float>(-parameters.gainDb * 0.5)}});

    highShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterFrequency(filterId),
            ParameterValue{
                static_cast<float>(parameters.frequencyHz)}});

    highShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterQ(filterId),
            ParameterValue{
                static_cast<float>(parameters.q)}});

    highShelf_.ApplyParameterChange(
        ParameterChange{
            ParameterAddress::EqFilterGain(filterId),
            ParameterValue{
                static_cast<float>(parameters.gainDb * 0.5)}});
}

} // namespace consolidator::dsp