#pragma once

#include "../IDspDeviceFactory.h"
#include "../../Models/FilterDefinition.h"
#include "Filters/BiquadBellFilter.h"
#include "Filters/BiquadHighShelfFilter.h"
#include "Filters/BiquadLowShelfFilter.h"
#include "Filters/GainFilter.h"
#include "Filters/TiltFilter.h"
#include "../../Settings/EqOptions.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class EqFilterFactory final : public IDspDeviceFactory {
public:
    EqFilterFactory(
        models::FilterDefinition definition,
        std::vector<double> values,
        double sampleRate
    ) : definition(std::move(definition)), values(std::move(values)), sampleRate(sampleRate) {}

    std::unique_ptr<IDspDevice> Create() const override {
        return CreateFilter();
    }

    std::unique_ptr<IEqFilter> CreateFilter() const {
        const double gain = definition.Value(values, "gain", 0.0);
        switch (definition.type) {
            case models::FilterType::Gain:
                return std::make_unique<GainFilter>(GainFilterSettings{ gain });
            case models::FilterType::Tilt:
                return std::make_unique<TiltFilter>(TiltFilterSettings{
                    definition.Value(values, "pivot", settings::EqOptions::DefaultFrequencyHz),
                    definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
            case models::FilterType::LowShelf:
                return std::make_unique<BiquadLowShelfFilter>(LowShelfFilterSettings{
                    definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                    definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
            case models::FilterType::HighShelf:
                return std::make_unique<BiquadHighShelfFilter>(HighShelfFilterSettings{
                    definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                    definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
            case models::FilterType::Peak:
                return std::make_unique<BiquadBellFilter>(BellFilterSettings{
                    definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                    definition.Value(values, "q", 1.0), gain, sampleRate });
        }
        return nullptr;
    }

private:
    models::FilterDefinition definition;
    std::vector<double> values;
    double sampleRate;
};

} // namespace consolidator::dsp
