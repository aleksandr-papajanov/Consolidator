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
            const c74::min::dict configuration{ c74::min::symbol(message.contractName.c_str()) };
            const auto atom = c74::min::atom{ static_cast<c74::max::t_object*>(configuration) };
            if (!parse_filter_contract_dictionary_for_slot(
                    contract, atom, static_cast<int>(message.filterId))) {
                return false;
            }
            color = ReadColor(atom, message.filterId);
        }
        else {
            if (!dictionary_atom(message.contract)) return false;
            c74::min::dict definition{ message.contract };
            if (!parse_filter_contract_definition(contract, definition)) return false;
            contract.slot = static_cast<int>(message.filterId);
        }

        filters_[message.filterId] = VisualFilter{ contract, color };
        return true;
    }

    bool SetSnapshot(const consolidator::protocol::EqStorageSnapshotMessage& message) {
        std::map<long, EqBank> nextBanks;
        const long nextSelectedBankId = message.selectedBankId;
        snapshotError_ = "unknown";
        c74::min::dict snapshot;
        try {
            snapshot = c74::min::dict{ c74::min::symbol(message.snapshotName.c_str()) };
            if (!snapshot.valid()) {
                snapshotError_ = "dictionary_not_found";
                return false;
            }
        }
        catch (...) {
            snapshotError_ = "dictionary_not_found";
            return false;
        }
        c74::min::dict sourceBanks;
        try {
            sourceBanks = c74::min::dict{ static_cast<c74::min::atom>(snapshot.at("banks")) };
            if (!sourceBanks.valid()) {
                snapshotError_ = "invalid_banks_dictionary";
                return false;
            }
        }
        catch (...) {
            snapshotError_ = "invalid_banks_dictionary";
            return false;
        }

        try {
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
            snapshotError_ = "invalid_bank_contents";
            return false;
        }

        if (nextBanks.find(nextSelectedBankId) == nextBanks.end()) {
            snapshotError_ = "selected_bank_not_found";
            return false;
        }
        selectedBankId_ = nextSelectedBankId;
        banks_ = std::move(nextBanks);
        snapshotError_.clear();
        return true;
    }

    const std::string& SnapshotError() const {
        return snapshotError_;
    }

    void PublishSelected(c74::min::outlet<>& outlet) const {
        const auto selected = banks_.find(selectedBankId_);
        for (const auto& [filterId, visual] : filters_) {
            const StoredFilter* state = nullptr;
            if (selected != banks_.end()) {
                const auto stored = selected->second.filters.find(filterId);
                if (stored != selected->second.filters.end()) state = &stored->second;
            }
            PublishFilter(filterId, visual, state, outlet);
        }
    }

    void PublishTotal(c74::min::outlet<>& outlet) const {
        SendCurve(SumBanks(false), outlet);
    }

    void PublishSelectedBank(c74::min::outlet<>& outlet) const {
        SendCurve(SumSelectedBank(), outlet);
    }

    std::vector<double> SelectedPrefixCurve() const {
        return SumBanks(true);
    }

private:
    struct VisualFilter {
        FilterContract contract;
        std::array<double, 4> color{ 1.0, 1.0, 1.0, 1.0 };
    };

    struct StoredFilter {
        std::vector<double> values;
        bool bypassed = false;
    };

    struct EqBank {
        std::map<long, StoredFilter> filters;
    };

    void PublishFilter(
        long filterId,
        const VisualFilter& visual,
        const StoredFilter* state,
        c74::min::outlet<>& outlet
    ) const {
        const bool active = state && !state->bypassed &&
            AbsoluteValuesMatchContract(visual.contract, state->values);
        const auto values = state && AbsoluteValuesMatchContract(visual.contract, state->values)
            ? state->values
            : DefaultAbsoluteValues(visual.contract);
        const auto spec = AbsoluteValuesToSpec(visual.contract, values);
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> curve(frequencies.size(), 0.0);
        if (active) {
            for (std::size_t index = 0; index < frequencies.size(); ++index) {
                curve[index] = filter_response_db(spec, frequencies[index], sampleRate_);
            }
        }

        double q = 0.0;
        double qMin = 0.0;
        double qMax = 0.0;
        for (const auto& parameter : visual.contract.parameters) {
            if (parameter.name == "q") {
                q = spec.q;
                qMin = parameter.range.min_value;
                qMax = parameter.range.max_value;
            }
        }

        c74::min::atoms output;
        output.reserve(13 + curve.size());
        output.push_back("filter_curve");
        output.push_back(filterId);
        output.push_back(active ? 1 : 0);
        for (const auto component : visual.color) output.push_back(component);
        output.push_back(visual.contract.type == FilterType::tilt ? spec.pivotHz : spec.freqHz);
        output.push_back(spec.gainDb);
        output.push_back(filter_type_name(visual.contract.type));
        output.push_back(q);
        output.push_back(qMin);
        output.push_back(qMax);
        for (const auto value : curve) output.push_back(value);
        outlet.send(output);
    }

    std::vector<double> SumBanks(bool stopAtSelected) const {
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> result(frequencies.size(), 0.0);
        for (const auto& [bankId, bank] : banks_) {
            if (stopAtSelected && bankId > selectedBankId_) break;
            AddBank(bank, frequencies, result);
        }
        return result;
    }

    std::vector<double> SumSelectedBank() const {
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> result(frequencies.size(), 0.0);
        const auto selected = banks_.find(selectedBankId_);
        if (selected != banks_.end()) AddBank(selected->second, frequencies, result);
        return result;
    }

    void AddBank(
        const EqBank& bank,
        const std::vector<double>& frequencies,
        std::vector<double>& result
    ) const {
        for (const auto& [filterId, state] : bank.filters) {
            const auto visual = filters_.find(filterId);
            if (visual == filters_.end() || state.bypassed ||
                !AbsoluteValuesMatchContract(visual->second.contract, state.values)) {
                continue;
            }
            const auto spec = AbsoluteValuesToSpec(visual->second.contract, state.values);
            for (std::size_t index = 0; index < frequencies.size(); ++index) {
                result[index] += filter_response_db(spec, frequencies[index], sampleRate_);
            }
        }
    }

    static void SendCurve(const std::vector<double>& curve, c74::min::outlet<>& outlet) {
        c74::min::atoms output;
        output.reserve(curve.size());
        for (const auto value : curve) output.push_back(value);
        outlet.send(output);
    }

    std::array<double, 4> ReadColor(const c74::min::atom& configuration, long filterId) const {
        try {
            c74::min::dict root{ configuration };
            c74::min::dict filters{ static_cast<c74::min::atom>(root.at("filters")) };
            c74::min::dict filter{ static_cast<c74::min::atom>(filters.at(std::to_string(filterId))) };
            return ParseColor(static_cast<c74::min::atom>(filter.at("color")));
        }
        catch (...) {
            return { 1.0, 1.0, 1.0, 1.0 };
        }
    }

    static std::array<double, 4> ParseColor(const c74::min::atom& value) {
        std::string text;
        try { text = static_cast<std::string>(value); }
        catch (...) { return { 1.0, 1.0, 1.0, 1.0 }; }
        if (!text.empty() && text.front() == '#') text.erase(text.begin());
        if (text.size() != 6 && text.size() != 8) return { 1.0, 1.0, 1.0, 1.0 };
        for (const auto character : text) {
            if (!std::isxdigit(static_cast<unsigned char>(character))) {
                return { 1.0, 1.0, 1.0, 1.0 };
            }
        }
        const auto component = [&text](std::size_t offset) {
            return static_cast<double>(std::stoul(text.substr(offset, 2), nullptr, 16)) / 255.0;
        };
        return { component(0), component(2), component(4),
            text.size() == 8 ? component(6) : 1.0 };
    }

    double sampleRate_ = EqCurveGrid::default_sample_rate;
    long selectedBankId_ = 0;
    std::string snapshotError_;
    std::map<long, EqBank> banks_;
    std::map<long, VisualFilter> filters_;
};
