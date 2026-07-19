#pragma once

#include "DictionaryCodec.h"
#include "Models/DeviceState.h"
#include "Settings/FilterOptions.h"

#include <cstdint>
#include <string>
#include <vector>

namespace consolidator::maxadapter {

template <>
struct DictionaryCodec<models::DeviceState> final {
    static std::optional<models::DeviceState> Deserialize(
        const messaging::MessageObject& object
    ) {
        const messaging::MessagePayload root{ object };
        const auto revision = root.ReadLong("revision");
        const auto generation = root.ReadLong("generation");
        const auto selectedBankId = root.ReadLong("selected_row");
        const auto bankCount = root.ReadLong("bank_count");
        if (!revision || !generation || !selectedBankId || !bankCount) {
            return std::nullopt;
        }

        models::DeviceState state;
        state.revision = *revision;
        state.generation = *generation;
        state.snapshot.selectedBankId = *selectedBankId;
        const auto& definitions = settings::FilterOptions::Definitions();

        for (long bankId = 1; bankId <= *bankCount; ++bankId) {
            const auto prefix = BankPrefix(bankId);
            const auto name = root.ReadString(prefix + "name");
            if (!name) return std::nullopt;
            models::EqBank bank;
            bank.bankId = bankId;
            bank.name = *name;
            for (const auto& [filterId, definition] : definitions) {
                const auto values = ReadDoubles(root, prefix + "filter_" + std::to_string(filterId));
                if (values.empty()) continue;
                models::FilterState filter;
                filter.filterId = filterId;
                filter.bankIndex = bankId;
                filter.values = values;
                filter.bypass = root.ReadBool(
                    prefix + "bypass_" + std::to_string(filterId)).value_or(false);
                bank.filters.push_back(std::move(filter));
            }
            state.snapshot.banks.push_back(std::move(bank));
        }
        return state;
    }

    static messaging::MessageObject Serialize(const models::DeviceState& state) {
        messaging::MessageObject root{
            { "revision", static_cast<std::int64_t>(state.revision) },
            { "generation", static_cast<std::int64_t>(state.generation) },
            { "selected_row", static_cast<std::int64_t>(state.snapshot.selectedBankId) },
            { "bank_count", static_cast<std::int64_t>(state.snapshot.banks.size()) }
        };

        for (const auto& bank : state.snapshot.banks) {
            const auto prefix = BankPrefix(bank.bankId);
            root[prefix + "name"] = bank.name;
            for (const auto& filter : bank.filters) {
                root[prefix + "filter_" + std::to_string(filter.filterId)] =
                    WriteDoubles(filter.values);
                root[prefix + "bypass_" + std::to_string(filter.filterId)] = filter.bypass;
            }
        }
        return root;
    }

private:
    static std::vector<double> ReadDoubles(
        const messaging::MessagePayload& object,
        const std::string& key
    ) {
        if (const auto single = object.ReadDouble(key)) return { *single };
        const auto array = object.ReadArray(key);
        if (!array) return {};
        std::vector<double> values;
        values.reserve(array->size());
        for (const auto& value : *array) {
            if (const auto number = value.As<double>()) values.push_back(*number);
            else if (const auto integer = value.As<std::int64_t>()) {
                values.push_back(static_cast<double>(*integer));
            }
            else return {};
        }
        return values;
    }

    static messaging::MessageArray WriteDoubles(const std::vector<double>& values) {
        messaging::MessageArray result;
        result.reserve(values.size());
        for (const auto value : values) result.emplace_back(value);
        return result;
    }

    static std::string BankPrefix(long bankId) {
        return "bank_" + std::to_string(bankId) + "_";
    }
};

} // namespace consolidator::maxadapter
