#pragma once

#include "../DspChainBuilder.h"
#include "../Curve/Curve.h"
#include "../../Models/EqSnapshot.h"
#include "../../Models/FilterDefinition.h"
#include "../../Settings/FilterOptions.h"
#include "EqFilterFactory.h"

#include <map>
#include <memory>
#include <string>

namespace consolidator::dsp {

class EqRuntime final {
public:
    EqRuntime() : definitions(settings::FilterOptions::Definitions()) {}

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
        auto order = AddAllBanksSection(builder, sampleRate, 0, models::EqSection::Pre);
        AddAllBanksSection(builder, sampleRate, order, models::EqSection::Post);
        return builder;
    }

    long AddAllBanks(DspChainBuilder& builder, double sampleRate, long firstOrder) const {
        auto order = AddAllBanksSection(builder, sampleRate, firstOrder, models::EqSection::Pre);
        return AddAllBanksSection(builder, sampleRate, order, models::EqSection::Post);
    }

    long AddAllBanksSection(
        DspChainBuilder& builder,
        double sampleRate,
        long firstOrder,
        models::EqSection section
    ) const {
        auto order = firstOrder;
        for (const auto& bank : snapshot.banks) AddBankSection(builder, bank, sampleRate, order, section);
        return order;
    }

    DspChainBuilder BuildBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        if (const auto bank = snapshot.FindBank(bankId)) {
            auto order = 0L;
            AddBankSection(builder, *bank, sampleRate, order, models::EqSection::Pre);
            AddBankSection(builder, *bank, sampleRate, order, models::EqSection::Post);
        }
        return builder;
    }

    DspChainBuilder BuildThroughBank(long bankId, double sampleRate) const {
        DspChainBuilder builder;
        auto order = 0L;
        for (const auto& bank : snapshot.banks) {
            if (bank.bankId > bankId) break;
            AddBankSection(builder, bank, sampleRate, order, models::EqSection::Pre);
            AddBankSection(builder, bank, sampleRate, order, models::EqSection::Post);
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
            if (SectionBypassed(*bank, definition->second.section)) continue;
            EqFilterFactory factory{ definition->second, filter.values, sampleRate };
            const auto processor = factory.CreateFilter();
            if (!processor) continue;
            for (std::size_t index = 0; index < curve.Inputs().size(); ++index) {
                curve.AddValue(index, processor->GetMagnitudeDb(curve.Inputs()[index]));
            }
        }
        return curve;
    }

    Curve BuildThroughBankCurve(long bankId, double sampleRate) const {
        Curve curve;
        for (const auto& bank : snapshot.banks) {
            if (bank.bankId > bankId) break;
            AddBankCurve(curve, bank, sampleRate);
        }
        return curve;
    }

    Curve BuildAllBanksCurve(double sampleRate) const {
        Curve curve;
        for (const auto& bank : snapshot.banks) AddBankCurve(curve, bank, sampleRate);
        return curve;
    }

private:
    void AddBankCurve(Curve& curve, const models::EqBank& bank, double sampleRate) const {
        for (const auto& filter : bank.filters) {
            if (filter.bypass) continue;
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end()) continue;
            if (SectionBypassed(bank, definition->second.section)) continue;
            EqFilterFactory factory{ definition->second, filter.values, sampleRate };
            const auto processor = factory.CreateFilter();
            if (!processor) continue;
            for (std::size_t index = 0; index < curve.Inputs().size(); ++index) {
                curve.AddValue(index, processor->GetMagnitudeDb(curve.Inputs()[index]));
            }
        }
    }

    void AddBankSection(
        DspChainBuilder& builder,
        const models::EqBank& bank,
        double sampleRate,
        long& order,
        models::EqSection section
    ) const {
        for (const auto& filter : bank.filters) {
            const auto definition = definitions.find(filter.filterId);
            if (definition == definitions.end() || definition->second.section != section) continue;
            builder.UpsertDevice({
                std::to_string(bank.bankId) + ":" + std::to_string(filter.filterId),
                std::make_shared<EqFilterFactory>(definition->second, filter.values, sampleRate),
                SectionBypassed(bank, section) || filter.bypass,
                order++,
                settings::AudioOptions::ParameterSmoothingSamples(sampleRate)
            });
        }
    }

    static bool SectionBypassed(const models::EqBank& bank, models::EqSection section) {
        return section == models::EqSection::Pre ? bank.preBypass : bank.postBypass;
    }

    std::map<long, models::FilterDefinition> definitions;
    models::EqSnapshot snapshot;
};

} // namespace consolidator::dsp
