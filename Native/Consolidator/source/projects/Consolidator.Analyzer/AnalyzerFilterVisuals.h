#pragma once

#include "FilterContractDictionary.h"
#include "FilterSpec.h"
#include "TypedMessages.h"

#include <array>
#include <cctype>
#include <map>
#include <string>
#include <vector>

class AnalyzerFilterVisuals {
public:
    void SetSampleRate(double sampleRate) {
        sampleRate_ = sampleRate;
    }

    bool Define(const consolidator::protocol::FilterDefineMessage& message) {
        FilterContract contract;
        std::array<double, 4> color{ 1.0, 1.0, 1.0, 1.0 };

        if (!message.contractName.empty()) {
            const c74::min::dict configuration{
                c74::min::symbol(message.contractName.c_str()) };
            if (!parse_filter_contract_dictionary_for_slot(
                    contract,
                    c74::min::atom{ static_cast<c74::max::t_object*>(configuration) },
                    static_cast<int>(message.filterId))) {
                return false;
            }
            color = ReadColor(
                c74::min::atom{ static_cast<c74::max::t_object*>(configuration) },
                message.filterId);
        }
        else {
            if (!dictionary_atom(message.contract)) return false;
            c74::min::dict definition{ message.contract };
            if (!parse_filter_contract_definition(contract, definition)) return false;
            contract.slot = static_cast<int>(message.filterId);
        }

        VisualFilter visual;
        visual.contract = contract;
        visual.spec = contract_to_spec(contract, default_normalized_values(contract));
        visual.color = color;
        filters_[message.filterId] = visual;
        return true;
    }

    bool Update(const consolidator::protocol::FilterUpdateMessage& message) {
        const auto found = filters_.find(message.filterId);
        if (found == filters_.end() ||
            message.values.size() != contract_parameter_count(found->second.contract)) {
            return false;
        }
        for (const auto value : message.values) {
            if (value < 0.0 || value > 1.0) return false;
        }

        found->second.spec = contract_to_spec(found->second.contract, message.values);
        return true;
    }

    bool SetBypass(const consolidator::protocol::FilterBypassMessage& message) {
        const auto found = filters_.find(message.filterId);
        if (found == filters_.end()) return false;
        found->second.bypassed = message.bypassed;
        return true;
    }

    bool SetSnapshot(const consolidator::protocol::EqStorageSnapshotMessage& message) {
        std::map<long, EqBank> nextBanks;
        try {
            c74::min::dict snapshot{ c74::min::symbol(message.snapshotName.c_str()) };
            c74::min::dict sourceBanks{ static_cast<c74::min::atom>(snapshot.at("banks")) };
            for (const auto& bankSymbol : sourceBanks.keys()) {
                const auto bankId = std::stol(static_cast<const char* const>(bankSymbol));
                c74::min::dict sourceBank{ static_cast<c74::min::atom>(sourceBanks.at(bankSymbol)) };
                c74::min::dict sourceFilters{ static_cast<c74::min::atom>(sourceBank.at("filters")) };
                auto& bank = nextBanks[bankId];
                for (const auto& filterSymbol : sourceFilters.keys()) {
                    const auto filterId = std::stol(static_cast<const char* const>(filterSymbol));
                    c74::min::dict sourceFilter{
                        static_cast<c74::min::atom>(sourceFilters.at(filterSymbol)) };
                    const auto values = static_cast<std::vector<c74::min::number>>(
                        sourceFilter.at("values"));
                    bool bypassed = false;
                    try {
                        bypassed = static_cast<double>(
                            static_cast<c74::min::atom>(sourceFilter.at("bypass"))) != 0.0;
                    }
                    catch (...) {
                    }
                    bank.filters[filterId] = StoredFilter{
                        std::vector<double>(values.begin(), values.end()), bypassed };
                }
            }
        }
        catch (...) {
            return false;
        }

        banks_ = std::move(nextBanks);
        return true;
    }

    void Publish(long filterId, c74::min::outlet<>& outlet) const {
        const auto found = filters_.find(filterId);
        if (found == filters_.end()) return;

        const auto& filter = found->second;
        const bool active = !filter.bypassed;
        const auto frequencies = make_eq_curve_frequency_grid();
        const auto curve = active
            ? ResponseCurve(filter.spec, frequencies)
            : std::vector<double>(frequencies.size(), 0.0);

        double q = 0.0;
        double qMin = 0.0;
        double qMax = 0.0;
        for (const auto& parameter : filter.contract.parameters) {
            if (parameter.name == "q") {
                q = filter.spec.q;
                qMin = parameter.range.min_value;
                qMax = parameter.range.max_value;
                break;
            }
        }

        c74::min::atoms output;
        output.reserve(13 + curve.size());
        output.push_back("filter_curve");
        output.push_back(filterId);
        output.push_back(active ? 1 : 0);
        output.push_back(filter.color[0]);
        output.push_back(filter.color[1]);
        output.push_back(filter.color[2]);
        output.push_back(filter.color[3]);
        output.push_back(filter.contract.type == FilterType::tilt
            ? filter.spec.pivotHz : filter.spec.freqHz);
        output.push_back(filter.spec.gainDb);
        output.push_back(filter_type_name(filter.contract.type));
        output.push_back(q);
        output.push_back(qMin);
        output.push_back(qMax);
        for (const auto value : curve) output.push_back(value);
        outlet.send(output);
    }

    void PublishAll(c74::min::outlet<>& outlet) const {
        for (const auto& item : filters_) {
            Publish(item.first, outlet);
        }
    }

    void PublishTotal(c74::min::outlet<>& outlet) const {
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> total(frequencies.size(), 0.0);

        for (const auto& [bankId, bank] : banks_) {
            for (const auto& [filterId, filter] : bank.filters) {
                const auto definition = filters_.find(filterId);
                if (definition == filters_.end() || filter.bypassed ||
                    filter.values.size() != contract_parameter_count(definition->second.contract)) {
                    continue;
                }

                const auto spec = contract_to_spec(definition->second.contract, filter.values);
                for (std::size_t index = 0; index < frequencies.size(); ++index) {
                    total[index] += filter_response_db(spec, frequencies[index], sampleRate_);
                }
            }
        }

        c74::min::atoms output;
        output.reserve(total.size());
        for (const auto value : total) output.push_back(value);
        outlet.send(output);
    }

private:
    struct VisualFilter {
        FilterContract contract;
        FilterSpec spec;
        std::array<double, 4> color{ 1.0, 1.0, 1.0, 1.0 };
        bool bypassed = false;
    };

    struct StoredFilter {
        std::vector<double> values;
        bool bypassed = false;
    };

    struct EqBank {
        std::map<long, StoredFilter> filters;
    };

    std::vector<double> ResponseCurve(
        const FilterSpec& spec,
        const std::vector<double>& frequencies) const {
        std::vector<double> result;
        result.reserve(frequencies.size());
        for (const auto frequency : frequencies) {
            result.push_back(filter_response_db(spec, frequency, sampleRate_));
        }
        return result;
    }

    std::array<double, 4> ReadColor(
        const c74::min::atom& configuration,
        long filterId) const {
        try {
            c74::min::dict root{ configuration };
            c74::min::dict filters{ static_cast<c74::min::atom>(root.at("filters")) };
            c74::min::dict filter{
                static_cast<c74::min::atom>(filters.at(std::to_string(filterId))) };
            return ParseColor(static_cast<c74::min::atom>(filter.at("color")));
        }
        catch (...) {
            return { 1.0, 1.0, 1.0, 1.0 };
        }
    }

    std::array<double, 4> ParseColor(const c74::min::atom& value) const {
        std::string text;
        try {
            text = static_cast<std::string>(value);
        }
        catch (...) {
            return { 1.0, 1.0, 1.0, 1.0 };
        }

        if (!text.empty() && text.front() == '#') text.erase(text.begin());
        if (text.size() != 6 && text.size() != 8) return { 1.0, 1.0, 1.0, 1.0 };
        for (const auto character : text) {
            if (!std::isxdigit(static_cast<unsigned char>(character))) {
                return { 1.0, 1.0, 1.0, 1.0 };
            }
        }

        auto component = [&text](std::size_t offset) {
            return static_cast<double>(std::stoul(text.substr(offset, 2), nullptr, 16)) / 255.0;
        };
        return {
            component(0), component(2), component(4),
            text.size() == 8 ? component(6) : 1.0
        };
    }

    double sampleRate_ = EqCurveGrid::default_sample_rate;
    std::map<long, EqBank> banks_;
    std::map<long, VisualFilter> filters_;
};
