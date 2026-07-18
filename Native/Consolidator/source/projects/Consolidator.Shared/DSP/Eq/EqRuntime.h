#pragma once

#include "../DspChainBuilder.h"
#include "../Curve/Curve.h"
#include "../../Models/EqSnapshot.h"
#include "../../Models/FilterDefinition.h"
#include "EqFilterFactory.h"

#include <map>
#include <memory>
#include <string>

namespace consolidator::dsp {

class EqRuntime final {
public:
    void Define(models::FilterDefinition definition) {
        definitions[definition.filterId] = std::move(definition);
    }

    void ClearDefinitions() {
        definitions.clear();
    }

    void SetSnapshot(models::EqSnapshot snapshot) {
        this->snapshot = std::move(snapshot);
    }

    const std::map<long, models::FilterDefinition>& Definitions() const {
        return definitions;
    }

    const models::EqSnapshot& Snapshot() const {
        return snapshot;
    }

    DspChainBuilder BuildAllBanks(double sampleRate) const {
        DspChainBuilder builder;
        for (const auto& bank : snapshot.banks) AddBank(builder, bank, sampleRate);
        return builder;
    }

    DspChainBuilder BuildBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        if (const auto bank = snapshot.FindBank(bankId)) {
            AddBank(builder, *bank, sampleRate);
        }
        return builder;
    }

    DspChainBuilder BuildThroughBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        for (const auto& bank : snapshot.banks) {
            if (bank.bankId > bankId) break;
            AddBank(builder, bank, sampleRate);
        }
        return builder;
    }

    Curve BuildBankCurve(long bankId, double sampleRate) const {
        Curve curve;
        const auto bank = snapshot.FindBank(bankId);
        if (!bank) return curve;

        for (const auto& filter : bank->filters) {
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
        return curve;
    }

private:
    void AddBank(DspChainBuilder& builder, const models::EqBank& bank, double sampleRate) const {
        for (const auto& filter : bank.filters) {
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) continue;
            builder.UpsertDevice({
                std::to_string(bank.bankId) + ":" + std::to_string(filter.filterId),
                std::make_shared<EqFilterFactory>(definition->second, filter.values, sampleRate),
                filter.bypass
            });
        }
    }

    std::map<long, models::FilterDefinition> definitions;
    models::EqSnapshot snapshot;
};

} // namespace consolidator::dsp
