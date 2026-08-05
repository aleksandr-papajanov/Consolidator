#include "Dsp/DspChainBuilder.h"

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"
#include "Dsp/Processors/Equalizer/Filters/HighShelfFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"
#include "Dsp/Processors/Equalizer/Filters/TiltFilter.h"
#include "Dsp/Processors/Compressor/Compressor.h"
#include "Dsp/Processors/DspChain.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Processors/Gain/Gain.h"
#include "Dsp/Processors/Saturator/Saturator.h"

namespace consolidator::dsp
{

namespace
{

using namespace consolidator::core::settings;

std::unique_ptr<Filter> CreateFilter(const FilterSettings& settings)
{
    switch (settings.kind)
    {
    case FilterKind::Bell:
        return std::make_unique<BellFilter>(settings.elementId, settings.frequencyHz.defaultValue);
    case FilterKind::LowShelf:
        return std::make_unique<LowShelfFilter>(settings.elementId, settings.frequencyHz.defaultValue);
    case FilterKind::HighShelf:
        return std::make_unique<HighShelfFilter>(settings.elementId, settings.frequencyHz.defaultValue);
    case FilterKind::Tilt:
        return std::make_unique<TiltFilter>(settings.elementId, settings.frequencyHz.defaultValue);
    case FilterKind::GainFilter:
        return std::make_unique<GainFilter>(settings.elementId);
    }
    return nullptr;
}

void BuildEqualizerFromSettings(Equalizer& equalizer, const EqualizerSettings& eqSettings)
{
    for (const auto& band : eqSettings.bands)
    {
        auto filter = CreateFilter(band);
        if (filter)
        {
            equalizer.AddFilter(std::move(filter));
        }
    }
}

} // namespace

std::unique_ptr<DspChain> DspChainBuilder::BuildFromSettings(const DspSettings& settings) const
{
    auto chain = std::make_unique<DspChain>();
    auto equalizer = std::make_unique<Equalizer>();
    BuildEqualizerFromSettings(*equalizer, settings.equalizer);

    // Input Gain
    chain->AddDevice(std::make_unique<Gain>(settings.inputGain.elementId));
    // Saturator (with built-in detector)
    chain->AddDevice(std::make_unique<Saturator>());
    // Compressor (with built-in detector)
    chain->AddDevice(std::make_unique<Compressor>());
    // Equalizer (7 bands)
    chain->AddDevice(std::move(equalizer));
    // Output Gain
    chain->AddDevice(std::make_unique<Gain>(settings.outputGain.elementId));

    return chain;
}

std::unique_ptr<DspChain> DspChainBuilder::BuildStandardChain() const
{
    return BuildFromSettings(DspSettings{});
}

} // namespace consolidator::dsp