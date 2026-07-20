#pragma once

#include "Messaging/MessagePayload.h"
#include "Definitions/BankNameGenerator.h"
#include "Models/EqSnapshot.h"
#include "Settings/FilterOptions.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace consolidator::persistence {

struct PersistedDeviceState {
    long schemaVersion = 2;
    models::EqSnapshot eq;
};

class PersistenceCodec final {
public:
    static constexpr long SchemaVersion = 2;

    static PersistedDeviceState Defaults() {
        PersistedDeviceState result{ SchemaVersion, {} };
        result.eq.selectedBankId = 1;
        result.eq.banks.push_back({ 1, domain::BankNameGenerator::Generate(1), {} });
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
            { "bank_ids", std::move(bankIds) }
        };
        for (const auto& bank : state.eq.banks) {
            const auto prefix = "bank." + std::to_string(bank.bankId) + ".";
            result[prefix + "name"] = bank.name;
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
        for (const auto& bankIdValue : *bankIds) {
            const auto decodedBankId = bankIdValue.As<std::int64_t>();
            if (!decodedBankId || *decodedBankId < 1 ||
                *decodedBankId > std::numeric_limits<long>::max()) return std::nullopt;
            const auto bankId = static_cast<long>(*decodedBankId);
            const auto prefix = "bank." + std::to_string(bankId) + ".";
            const auto name = root.ReadString(prefix + "name");
            if (!name) return std::nullopt;
            models::EqBank bank{ bankId, *name, {} };
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
