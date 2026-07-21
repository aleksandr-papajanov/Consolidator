#pragma once

#include "../IDspDeviceFactory.h"
#include "../../Models/FilterDefinition.h"
#include "Filters/BiquadBellFilter.h"
#include "Filters/BiquadHighShelfFilter.h"
#include "Filters/BiquadLowShelfFilter.h"
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

    bool CanUpdate(const IDspDevice& device) const override {
        switch (definition.type) {
            case models::FilterType::Tilt:
                return dynamic_cast<const TiltFilter*>(&device) != nullptr;
            case models::FilterType::LowShelf:
                return dynamic_cast<const BiquadLowShelfFilter*>(&device) != nullptr;
            case models::FilterType::HighShelf:
                return dynamic_cast<const BiquadHighShelfFilter*>(&device) != nullptr;
            case models::FilterType::Peak:
                return dynamic_cast<const BiquadBellFilter*>(&device) != nullptr;
        }
        return false;
    }

    void Update(IDspDevice& device) const override {
        const double gain = definition.Value(values, "gain", 0.0);
        switch (definition.type) {
            case models::FilterType::Tilt:
                if (auto* filter = dynamic_cast<TiltFilter*>(&device)) {
                    filter->UpdateSettings({
                        definition.Value(values, "pivot", settings::EqOptions::DefaultFrequencyHz),
                        definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
                }
                return;
            case models::FilterType::LowShelf:
                if (auto* filter = dynamic_cast<BiquadLowShelfFilter*>(&device)) {
                    filter->UpdateSettings({
                        definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                        definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
                }
                return;
            case models::FilterType::HighShelf:
                if (auto* filter = dynamic_cast<BiquadHighShelfFilter*>(&device)) {
                    filter->UpdateSettings({
                        definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                        definition.Value(values, "q", settings::EqOptions::DefaultFilterQ), gain, sampleRate });
                }
                return;
            case models::FilterType::Peak:
                if (auto* filter = dynamic_cast<BiquadBellFilter*>(&device)) {
                    filter->UpdateSettings({
                        definition.Value(values, "freq", settings::EqOptions::DefaultFrequencyHz),
                        definition.Value(values, "q", 1.0), gain, sampleRate });
                }
                return;
        }
    }

    std::unique_ptr<IEqFilter> CreateFilter() const {
        const double gain = definition.Value(values, "gain", 0.0);
        switch (definition.type) {
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
