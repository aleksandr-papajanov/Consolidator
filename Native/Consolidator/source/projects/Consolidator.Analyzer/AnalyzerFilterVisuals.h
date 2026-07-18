#pragma once

#include "DSP/Curve/Curve.h"
#include "DSP/Eq/EqFilterFactory.h"
#include "DSP/Eq/EqRuntime.h"
#include "Models/EqSnapshot.h"
#include "Models/FilterDefinition.h"
#include "Settings/GlobalSettings.h"

#include "c74_min.h"

#include <string>
#include <vector>

class AnalyzerFilterVisuals {
public:
    void SetSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
    }

    void Define(consolidator::models::FilterDefinition definition) {
        eqRuntime.Define(std::move(definition));
    }

    void ClearDefinitions() {
        eqRuntime.ClearDefinitions();
    }

    bool SetSnapshot(consolidator::models::EqSnapshot snapshot) {
        if (!snapshot.SelectedBank()) {
            snapshotError = "selected_bank_not_found";
            return false;
        }
        eqRuntime.SetSnapshot(std::move(snapshot));
        snapshotError.clear();
        return true;
    }

    const std::string& SnapshotError() const {
        return snapshotError;
    }

    void PublishSelected(c74::min::outlet<>& outlet) const {
        const auto& snapshot = eqRuntime.Snapshot();
        const auto selected = snapshot.SelectedBank();
        for (const auto& [filterId, definition] : eqRuntime.Definitions()) {
            const auto filter = selected ? selected->FindFilter(filterId) : nullptr;
            PublishFilter(definition, filter, outlet);
        }
    }

    void PublishTotal(c74::min::outlet<>& outlet) const {
        SendCurve(SumBanks(false), outlet);
    }

    consolidator::dsp::Curve SelectedPrefixCurve() const {
        return SumBanks(true);
    }

private:
    using EqBank = consolidator::models::EqBank;
    using FilterState = consolidator::models::FilterState;
    using FilterDefinition = consolidator::models::FilterDefinition;

    void PublishFilter(
        const FilterDefinition& definition,
        const FilterState* state,
        c74::min::outlet<>& outlet
    ) const {
        const bool active = state && !state->bypass;
        const auto values = state ? state->values : definition.DefaultValues();
        auto curve = BuildFilterCurve(definition, values, active);

        const auto frequencyName = definition.type == consolidator::models::FilterType::Tilt
            ? "pivot" : "freq";
        const double frequency = definition.Value(values, frequencyName,
            consolidator::settings::GlobalSettings::DefaultFrequencyHz);
        const double gain = definition.Value(values, "gain", 0.0);
        const auto qParameter = definition.FindParameter("q");
        const double q = definition.Value(values, "q", 0.0);
        const double qMinimum = qParameter ? qParameter->range.minimum : 0.0;
        const double qMaximum = qParameter ? qParameter->range.maximum : 0.0;

        c74::min::atoms output;
        output.reserve(13 + curve.Values().size());
        output.push_back("filter_curve");
        output.push_back(definition.filterId);
        output.push_back(active ? 1 : 0);
        for (const auto component : definition.color) output.push_back(component);
        output.push_back(frequency);
        output.push_back(gain);
        output.push_back(std::string{ consolidator::models::FilterTypeName(definition.type) });
        output.push_back(q);
        output.push_back(qMinimum);
        output.push_back(qMaximum);
        for (const auto value : curve.Values()) output.push_back(value);
        outlet.send(output);
    }

    consolidator::dsp::Curve SumBanks(bool stopAtSelected) const {
        consolidator::dsp::Curve result;
        for (const auto& bank : eqRuntime.Snapshot().banks) {
            if (stopAtSelected && bank.bankId > eqRuntime.Snapshot().selectedBankId) break;
            AddBank(bank, result);
        }
        return result;
    }

    void AddBank(
        const EqBank& bank,
        consolidator::dsp::Curve& result
    ) const {
        for (const auto& filter : bank.filters) {
            if (filter.bypass) continue;
            const auto definition = eqRuntime.Definitions().find(filter.filterId);
            if (definition == eqRuntime.Definitions().end()) continue;
            consolidator::dsp::EqFilterFactory factory{
                definition->second, filter.values, sampleRate };
            const auto processor = factory.CreateFilter();
            if (!processor) continue;
            for (std::size_t index = 0; index < result.Inputs().size(); ++index) {
                result.AddValue(index, processor->GetMagnitudeDb(result.Inputs()[index]));
            }
        }
    }

    consolidator::dsp::Curve BuildFilterCurve(
        const FilterDefinition& definition,
        const std::vector<double>& values,
        bool active
    ) const {
        consolidator::dsp::Curve result;
        if (!active) return result;
        consolidator::dsp::EqFilterFactory factory{ definition, values, sampleRate };
        const auto processor = factory.CreateFilter();
        if (!processor) return result;
        for (std::size_t index = 0; index < result.Inputs().size(); ++index) {
            result.SetValue(index, processor->GetMagnitudeDb(result.Inputs()[index]));
        }
        return result;
    }

    static void SendCurve(const consolidator::dsp::Curve& curve, c74::min::outlet<>& outlet) {
        c74::min::atoms output;
        output.reserve(curve.Values().size());
        for (const auto value : curve.Values()) output.push_back(value);
        outlet.send(output);
    }

    double sampleRate = consolidator::settings::GlobalSettings::DefaultSampleRateHz;
    consolidator::dsp::EqRuntime eqRuntime;
    std::string snapshotError;
};
