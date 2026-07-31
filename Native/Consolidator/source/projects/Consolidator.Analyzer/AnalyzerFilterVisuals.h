#pragma once

#include "DSP/Curve/Curve.h"
#include "DSP/Eq/EqFilterFactory.h"
#include "DSP/Eq/EqRuntime.h"
#include "Models/EqSnapshot.h"
#include "Models/FilterDefinition.h"
#include "Settings/AudioOptions.h"
#include "Settings/EqOptions.h"

#include "c74_min.h"

#include <string>
#include <vector>

class AnalyzerFilterVisuals {
public:
    void SetSampleRate(double sampleRate) {
        this->sampleRate = sampleRate;
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

    bool UpdateParameter(
        long bankId,
        long filterId,
        const std::string& parameter,
        double value
    ) {
        return eqRuntime.UpdateParameter(bankId, filterId, parameter, value);
    }

    void RefreshCurves() {
        const auto* selectedBank = eqRuntime.Snapshot().SelectedBank();
        banksThroughSelectedCurve = selectedBank
            ? eqRuntime.BuildThroughBankCurve(selectedBank->bankId, sampleRate)
            : consolidator::dsp::Curve{};
        totalCurve = SumBanks();
    }

    const std::string& SnapshotError() const {
        return snapshotError;
    }

    const consolidator::models::EqSnapshot& Snapshot() const {
        return eqRuntime.Snapshot();
    }

    const consolidator::models::FilterDefinition* Definition(long filterId) const {
        const auto definition = eqRuntime.Definitions().find(filterId);
        return definition == eqRuntime.Definitions().end() ? nullptr : &definition->second;
    }

    void PublishSelected(c74::min::outlet<>& outlet) const {
        const auto& snapshot = eqRuntime.Snapshot();
        const auto selected = snapshot.SelectedBank();
        for (const auto& [filterId, definition] : eqRuntime.Definitions()) {
            const auto filter = selected ? selected->FindFilter(filterId) : nullptr;
            PublishFilter(definition, filter, selected && !snapshot.IsBypassed(), outlet);
        }
    }

    void PublishTotal(c74::min::outlet<>& outlet) const {
        SendCurve(totalCurve, outlet);
    }

    const consolidator::dsp::Curve& BanksThroughSelectedCurve() const {
        return banksThroughSelectedCurve;
    }

private:
    using EqBank = consolidator::models::EqBank;
    using FilterState = consolidator::models::FilterState;
    using FilterDefinition = consolidator::models::FilterDefinition;

    void PublishFilter(
        const FilterDefinition& definition,
        const FilterState* state,
        bool bankActive,
        c74::min::outlet<>& outlet
    ) const {
        const bool active = bankActive && state && !state->bypass;
        const auto values = state ? state->values : definition.DefaultValues();
        const auto curve = active
            ? BuildFilterCurve(definition, values, true)
            : consolidator::dsp::Curve{};

        const auto frequencyName = definition.type == consolidator::models::FilterType::Tilt
            ? "pivot" : "freq";
        const double frequency = definition.Value(values, frequencyName,
            consolidator::settings::EqOptions::DefaultFrequencyHz);
        const double gain = definition.Value(values, "gain", 0.0);
        const auto qParameter = definition.FindParameter("q");
        const double q = definition.Value(values, "q", 0.0);
        const double qMinimum = qParameter ? qParameter->range.minimum : 0.0;
        const double qMaximum = qParameter ? qParameter->range.maximum : 0.0;
        const auto frequencyParameter = definition.FindParameter(frequencyName);
        const auto gainParameter = definition.FindParameter("gain");
        const double frequencyMinimum = frequencyParameter ? frequencyParameter->range.minimum : frequency;
        const double frequencyMaximum = frequencyParameter ? frequencyParameter->range.maximum : frequency;
        const double gainMinimum = gainParameter ? gainParameter->range.minimum : gain;
        const double gainMaximum = gainParameter ? gainParameter->range.maximum : gain;

        c74::min::atoms output;
        output.reserve(13 + (active ? curve.Values().size() : 0));
        output.push_back("filter_curve");
        output.push_back(definition.filterId);
        output.push_back(active ? 1 : 0);
        output.push_back(frequency);
        output.push_back(gain);
        output.push_back(std::string{ consolidator::models::FilterTypeName(definition.type) });
        output.push_back(q);
        output.push_back(qMinimum);
        output.push_back(qMaximum);
        output.push_back(frequencyMinimum);
        output.push_back(frequencyMaximum);
        output.push_back(gainMinimum);
        output.push_back(gainMaximum);
        if (active) {
            for (const auto value : curve.Values()) output.push_back(value);
        }
        outlet.send(output);
    }

    consolidator::dsp::Curve SumBanks() const {
        return eqRuntime.BuildAllBanksCurve(sampleRate);
    }

    void AddBank(
        const EqBank& bank,
        consolidator::dsp::Curve& result
    ) const {
        for (const auto& filter : bank.filters) {
            if (filter.bypass) continue;
            const auto definition = eqRuntime.Definitions().find(filter.filterId);
            if (definition == eqRuntime.Definitions().end()) continue;
            if (eqRuntime.Snapshot().IsBypassed()) continue;
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

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dsp::EqRuntime eqRuntime;
    consolidator::dsp::Curve banksThroughSelectedCurve;
    consolidator::dsp::Curve totalCurve;
    std::string snapshotError;
};
