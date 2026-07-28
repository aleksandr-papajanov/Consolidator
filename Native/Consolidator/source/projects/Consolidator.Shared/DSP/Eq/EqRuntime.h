#pragma once

#include "../DspChainBuilder.h"
#include "../Curve/Curve.h"
#include "../../Models/EqSnapshot.h"
#include "../../Models/FilterDefinition.h"
#include "../../Settings/FilterOptions.h"
#include "EqFilterFactory.h"

#include <map>
#include <cmath>
#include <memory>
#include <string>

namespace consolidator::dsp {

class EqRuntime final {
public:
    EqRuntime() : definitions(settings::FilterOptions::EqDefinitions()) {}

    void SetSnapshot(models::EqSnapshot snapshot) {
        this->snapshot = std::move(snapshot);
    }

    const std::map<long, models::FilterDefinition>& Definitions() const {
        return definitions;
    }

    const models::EqSnapshot& Snapshot() const {
        return snapshot;
    }

    bool UpdateParameter(
        long bankId,
        long filterId,
        const std::string& parameter,
        double value
    ) {
        auto* bank = snapshot.FindBank(bankId);
        auto* filter = bank ? bank->FindFilter(filterId) : nullptr;
        const auto definition = definitions.find(filterId);
        if (!filter || definition == definitions.end()) return false;
        const auto parameterIndex = definition->second.ParameterIndex(parameter);
        if (!parameterIndex || *parameterIndex >= filter->values.size()) return false;
        filter->values[*parameterIndex] = value;
        return true;
    }

    DspChainBuilder BuildAllBanks(double sampleRate) const {
        DspChainBuilder builder;
        AddAllBanks(builder, sampleRate, 0);
        return builder;
    }

    long AddAllBanks(DspChainBuilder& builder, double sampleRate, long firstOrder) const {
        auto order = firstOrder;
        for (const auto& bank : snapshot.banks) {
            if (ShouldIncludeBank(bank)) {
                AddBank(builder, bank, sampleRate, order);
            }
        }
        return order;
    }

    DspChainBuilder BuildBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        if (const auto bank = snapshot.FindBank(bankId)) {
            auto order = 0L;
            AddBank(builder, *bank, sampleRate, order);
        }
        return builder;
    }

    DspChainBuilder BuildThroughBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        auto order = 0L;
        for (const auto& bank : snapshot.banks) {
            if (bank.bankId <= bankId && ShouldIncludeBank(bank)) {
                AddBank(builder, bank, sampleRate, order);
            }
        }
        return builder;
    }

    Curve BuildBankCurve(long bankId, double sampleRate) const {
        Curve curve;
        const auto bank = snapshot.FindBank(bankId);
        if (!bank) return curve;

        AddBankCurve(curve, *bank, sampleRate);
        return curve;
    }

    Curve BuildThroughBankCurve(long bankId, double sampleRate) const {
        Curve curve;
        for (const auto& bank : snapshot.banks) {
            if (bank.bankId <= bankId && ShouldIncludeBank(bank)) {
                AddBankCurve(curve, bank, sampleRate);
            }
        }
        return curve;
    }

    Curve BuildAllBanksCurve(double sampleRate) const {
        Curve curve;
        for (const auto& bank : snapshot.banks) {
            if (ShouldIncludeBank(bank)) {
                AddBankCurve(curve, bank, sampleRate);
            }
        }
        return curve;
    }

private:
    bool ShouldIncludeBank(const models::EqBank& bank) const {
        return !snapshot.solo || bank.bankId == snapshot.selectedBankId;
    }

    void AddBankCurve(Curve& curve, const models::EqBank& bank, double sampleRate) const {
        if (snapshot.IsBypassed()) return;
        for (const auto& filter : bank.filters) {
            if (filter.bypass) continue;
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) continue;
            EqFilterFactory factory{ definition->second, filter.values, sampleRate };
            const auto processor = factory.CreateFilter();
            if (!processor) continue;
            for (std::size_t index = 0; index < curve.Inputs().size(); ++index) {
                curve.AddValue(index, processor->GetMagnitudeDb(curve.Inputs()[index]));
            }
        }
    }

    void AddBank(
        DspChainBuilder& builder,
        const models::EqBank& bank,
        double sampleRate,
        long& order
    ) const {
        for (std::size_t filterIndex = 0; filterIndex < bank.filters.size(); ++filterIndex) {
            const auto& filter = bank.filters[filterIndex];
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) continue;
            builder.UpsertDevice({
                std::to_string(bank.bankId) + ":" + std::to_string(filterIndex),
                std::make_shared<EqFilterFactory>(definition->second, filter.values, sampleRate),
                snapshot.IsBypassed() || filter.bypass || IsNeutral(definition->second, filter),
                order++,
                settings::AudioOptions::ParameterSmoothingSamples(sampleRate)
            });
        }
    }

    static bool IsNeutral(
        const models::FilterDefinition& definition,
        const models::FilterState& filter
    ) {
        return std::abs(definition.Value(filter.values, "gain", 0.0)) < 1.0e-12;
    }

    std::map<long, models::FilterDefinition> definitions;
    models::EqSnapshot snapshot;
};

} // namespace consolidator::dsp
