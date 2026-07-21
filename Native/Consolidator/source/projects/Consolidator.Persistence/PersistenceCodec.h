#pragma once

#include "Messaging/MessagePayload.h"
#include "Definitions/BankNameGenerator.h"
#include "Models/EqSnapshot.h"
#include "Models/ProcessorState.h"
#include "Settings/FilterOptions.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace consolidator::persistence {

struct PersistedDeviceState {
    long schemaVersion = 6;
    models::EqSnapshot eq;
    models::ProcessorState processor;
};

class PersistenceCodec final {
public:
    static constexpr long SchemaVersion = 6;

    static PersistedDeviceState Defaults() {
        PersistedDeviceState result{ SchemaVersion, {}, {} };
        result.eq.selectedBankId = 1;
        result.eq.banks.push_back({ 1, domain::BankNameGenerator::Generate(1), false, false, {} });
        for (const auto& [filterId, definition] : settings::FilterOptions::Definitions()) {
            result.eq.banks.front().filters.push_back({
                filterId, definition.DefaultValues(), definition.defaultBypass
            });
        }
        return result;
    }

    static messaging::MessageObject Serialize(const PersistedDeviceState& state) {
        messaging::MessageArray bankIds;
        for (const auto& bank : state.eq.banks) {
            bankIds.emplace_back(static_cast<std::int64_t>(bank.bankId));
        }
        messaging::MessageObject result{
            { "schema_version", static_cast<std::int64_t>(state.schemaVersion) },
            { "selected_bank", static_cast<std::int64_t>(state.eq.selectedBankId) },
            { "bank_ids", std::move(bankIds) },
            { "input_gain.gain", state.processor.inputGain.gainDb },
            { "compressor.attack", state.processor.compressor.attackMs },
            { "compressor.release", state.processor.compressor.releaseMs },
            { "compressor.threshold", state.processor.compressor.thresholdDb },
            { "compressor.bypass", state.processor.compressor.bypass },
            { "saturator.saturation", state.processor.saturator.saturation },
            { "saturator.bypass", state.processor.saturator.bypass },
            { "output_gain.gain", state.processor.outputGain.gainDb }
        };
        for (const auto& bank : state.eq.banks) {
            const auto prefix = "bank." + std::to_string(bank.bankId) + ".";
            result[prefix + "name"] = bank.name;
            result[prefix + "pre_bypass"] = bank.preBypass;
            result[prefix + "post_bypass"] = bank.postBypass;
            messaging::MessageArray filterIds;
            for (const auto& filter : bank.filters) {
                filterIds.emplace_back(static_cast<std::int64_t>(filter.filterId));
                const auto filterPrefix = prefix + "filter." + std::to_string(filter.filterId) + ".";
                result[filterPrefix + "bypass"] = filter.bypass;
                messaging::MessageArray values;
                for (const auto value : filter.values) values.emplace_back(value);
                result[filterPrefix + "values"] = std::move(values);
            }
            result[prefix + "filter_ids"] = std::move(filterIds);
        }
        return result;
    }

    static std::optional<PersistedDeviceState> Deserialize(const messaging::MessageObject& object) {
        const messaging::MessagePayload root{ object };
        const auto schema = root.ReadLong("schema_version");
        const auto selectedBank = root.ReadLong("selected_bank");
        const auto bankIds = root.ReadArray("bank_ids");
        if (!schema || !selectedBank || !bankIds || *schema != SchemaVersion || bankIds->empty()) {
            return std::nullopt;
        }

        PersistedDeviceState result;
        result.schemaVersion = *schema;
        result.eq.selectedBankId = *selectedBank;
        const auto inputGain = root.ReadDouble("input_gain.gain");
        const auto attack = root.ReadDouble("compressor.attack");
        const auto release = root.ReadDouble("compressor.release");
        const auto threshold = root.ReadDouble("compressor.threshold");
        const auto compressorBypass = root.ReadBool("compressor.bypass");
        const auto saturation = root.ReadDouble("saturator.saturation");
        const auto saturatorBypass = root.ReadBool("saturator.bypass");
        const auto outputGain = root.ReadDouble("output_gain.gain");
        if (!inputGain || !attack || !release || !threshold || !compressorBypass ||
            !saturation || !saturatorBypass || !outputGain) {
            return std::nullopt;
        }
        result.processor.inputGain = { *inputGain };
        result.processor.compressor = { *attack, *release, *threshold, *compressorBypass };
        result.processor.saturator = { *saturation, *saturatorBypass };
        result.processor.outputGain = { *outputGain };
        for (const auto& bankIdValue : *bankIds) {
            const auto decodedBankId = bankIdValue.As<std::int64_t>();
            if (!decodedBankId || *decodedBankId < 1 ||
                *decodedBankId > std::numeric_limits<long>::max()) return std::nullopt;
            const auto bankId = static_cast<long>(*decodedBankId);
            const auto prefix = "bank." + std::to_string(bankId) + ".";
            const auto name = root.ReadString(prefix + "name");
            const auto preBypass = root.ReadBool(prefix + "pre_bypass");
            const auto postBypass = root.ReadBool(prefix + "post_bypass");
            if (!name || !preBypass || !postBypass) return std::nullopt;
            models::EqBank bank{ bankId, *name, *preBypass, *postBypass, {} };
            const auto filterIds = root.ReadArray(prefix + "filter_ids");
            if (!filterIds) return std::nullopt;
            for (const auto& filterIdValue : *filterIds) {
                const auto filterId = filterIdValue.As<std::int64_t>();
                if (!filterId || *filterId < 1 ||
                    *filterId > std::numeric_limits<long>::max()) return std::nullopt;
                const auto filterPrefix = prefix + "filter." + std::to_string(*filterId) + ".";
                const auto values = root.ReadArray(filterPrefix + "values");
                if (!values) return std::nullopt;
                models::FilterState filter;
                filter.filterId = static_cast<long>(*filterId);
                filter.bypass = root.ReadBool(filterPrefix + "bypass").value_or(false);
                for (const auto& value : *values) {
                    if (const auto number = value.As<double>()) filter.values.push_back(*number);
                    else if (const auto integer = value.As<std::int64_t>()) filter.values.push_back(static_cast<double>(*integer));
                    else return std::nullopt;
                }
                bank.filters.push_back(std::move(filter));
            }
            result.eq.banks.push_back(std::move(bank));
        }
        if (!result.eq.FindBank(result.eq.selectedBankId)) return std::nullopt;
        return result;
    }
};

} // namespace consolidator::persistence
