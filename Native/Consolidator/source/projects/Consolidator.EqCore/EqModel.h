#pragma once

#include "FilterChain.h"
#include "EqParams.h"
#include "FilterRegistry.h"

#include <vector>

class EqModel {
public:
    static std::vector<double> buildCurve(
        const std::vector<double>& freqs,
        const FilterRegistry& registry,
        const std::vector<double>& normalized_values,
        double sampleRate = EqCurveGrid::default_sample_rate
    ) {
        FilterChain chain;
        chain.set_sample_rate(sampleRate);

        std::size_t value_offset = 0;
        for (std::size_t slot = 0; slot < FilterRegistry::max_filters; ++slot) {
            const auto& contract = registry.at(slot);
            if (!contract) {
                continue;
            }

            const auto parameter_count = contract->parameters.size();
            if (value_offset + parameter_count > normalized_values.size()) {
                break;
            }

            const std::vector<double> values(
                normalized_values.begin() + value_offset,
                normalized_values.begin() + value_offset + parameter_count);
            chain.set_filter(slot, contract_to_spec(*contract, values));
            value_offset += parameter_count;
        }

        return chain.response_curve(freqs);
    }

    static std::vector<double> buildCurve(
        const std::vector<double>& freqs,
        const EqParams& p,
        const FilterRegistry& registry,
        double sampleRate = EqCurveGrid::default_sample_rate
    ) {
        FilterChain chain;
        chain.set_sample_rate(sampleRate);

        int bell_index = 0;
        for (std::size_t slot = 0; slot < FilterRegistry::max_filters; ++slot) {
            const auto& contract = registry.at(slot);
            if (!contract) {
                continue;
            }

            switch (contract->type) {
                case FilterType::gain:
                    chain.set_filter(slot, FilterSpec{ FilterType::gain, p.gainDb });
                    break;
                case FilterType::tilt:
                    chain.set_filter(slot, FilterSpec{ FilterType::tilt, p.tiltDb, 0.0, 1.0, p.tiltPivotHz });
                    break;
                case FilterType::low_shelf:
                    chain.set_filter(slot, FilterSpec{ FilterType::low_shelf, p.lowShelf.gainDb, p.lowShelf.freqHz, p.lowShelf.q });
                    break;
                case FilterType::high_shelf:
                    chain.set_filter(slot, FilterSpec{ FilterType::high_shelf, p.highShelf.gainDb, p.highShelf.freqHz, p.highShelf.q });
                    break;
                case FilterType::peak:
                    if (bell_index < static_cast<int>(p.bells.size())) {
                        const auto& bell = p.bells[bell_index++];
                        chain.set_filter(slot, FilterSpec{ FilterType::peak, bell.gainDb, bell.freqHz, bell.q });
                    }
                    break;
            }
        }

        return chain.response_curve(freqs);
    }
};
