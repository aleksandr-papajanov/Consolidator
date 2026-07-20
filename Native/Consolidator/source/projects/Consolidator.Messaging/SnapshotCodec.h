#pragma once

#include "AtomWriter.h"
#include "AtomReader.h"
#include "Definitions/Definitions.h"
#include "States/States.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace consolidator::messaging {

class SnapshotCodec final {
public:
    static AtomList EncodeDefinitions(const domain::FilterDefinitionCatalog& definitions) {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "definitions" })
            .Write(static_cast<std::int64_t>(1))
            .Write(static_cast<std::int64_t>(definitions.size()));
        for (const auto& [filterId, definition] : definitions) {
            writer.Write(static_cast<std::int64_t>(filterId))
                .Write(std::string{ models::FilterTypeName(definition.type) })
                .Write(definition.defaultBypass)
                .Write(static_cast<std::int64_t>(definition.parameters.size()));
            for (const auto& parameter : definition.parameters) {
                writer.Write(parameter.name)
                    .Write(parameter.range.minimum)
                    .Write(parameter.range.maximum)
                    .Write(static_cast<std::int64_t>(parameter.range.scale))
                    .Write(parameter.defaultValue);
            }
        }
        return std::move(writer).Finish();
    }

    static AtomList EncodeEq(const domain::EqState& state, domain::StoreRevision revision) {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "eq" })
            .Write(static_cast<std::int64_t>(revision))
            .Write(static_cast<std::int64_t>(state.selectedBankId))
            .Write(static_cast<std::int64_t>(state.banks.size()));
        for (const auto& bank : state.banks) {
            writer.Write(static_cast<std::int64_t>(bank.bankId)).Write(bank.name)
                .Write(static_cast<std::int64_t>(bank.filters.size()));
            for (const auto& filter : bank.filters) {
                writer.Write(static_cast<std::int64_t>(filter.filterId))
                    .Write(filter.bypass)
                    .Write(static_cast<std::int64_t>(filter.values.size()));
                for (const auto value : filter.values) writer.Write(value);
            }
        }
        return std::move(writer).Finish();
    }

    static std::optional<domain::EqState> DecodeEq(const AtomList& atoms) {
        AtomReader reader(atoms);
        const auto category = reader.ReadString();
        const auto version = reader.ReadInt();
        const auto source = reader.ReadString();
        const auto store = reader.ReadString();
        const auto revision = reader.ReadInt();
        const auto selectedBank = reader.ReadInt();
        const auto bankCount = reader.ReadInt();
        if (!category || !version || !source || !store || !revision || !selectedBank || !bankCount ||
            *category != "snapshot" || *version != 1 || *source != "host" || *store != "eq" ||
            *revision < 0 || *selectedBank < 1 || *selectedBank > std::numeric_limits<long>::max() ||
            *bankCount < 1) {
            return std::nullopt;
        }

        domain::EqState result;
        result.selectedBankId = static_cast<long>(*selectedBank);
        std::int64_t previousBankId = 0;
        for (long bankIndex = 0; bankIndex < *bankCount; ++bankIndex) {
            const auto bankId = reader.ReadInt();
            const auto name = reader.ReadString();
            const auto filterCount = reader.ReadInt();
            if (!bankId || !name || !filterCount || *bankId <= previousBankId ||
                *bankId > std::numeric_limits<long>::max() ||
                name->empty() || *filterCount < 0) return std::nullopt;
            previousBankId = *bankId;
            models::EqBank bank;
            bank.bankId = static_cast<long>(*bankId);
            bank.name = *name;
            std::int64_t previousFilterId = 0;
            for (long filterIndex = 0; filterIndex < *filterCount; ++filterIndex) {
                const auto filterId = reader.ReadInt();
                const auto bypass = reader.ReadBool();
                const auto valueCount = reader.ReadInt();
                if (!filterId || !bypass || !valueCount || *filterId <= previousFilterId ||
                    *filterId > std::numeric_limits<long>::max() ||
                    *valueCount < 0) return std::nullopt;
                previousFilterId = *filterId;
                models::FilterState filter;
                filter.filterId = static_cast<long>(*filterId);
                filter.bypass = *bypass;
                for (long valueIndex = 0; valueIndex < *valueCount; ++valueIndex) {
                    const auto value = reader.ReadDouble();
                    if (!value || !std::isfinite(*value)) return std::nullopt;
                    filter.values.push_back(*value);
                }
                bank.filters.push_back(std::move(filter));
            }
            result.banks.push_back(std::move(bank));
        }
        return reader.RequireEnd() && result.FindBank(result.selectedBankId)
            ? std::optional<domain::EqState>{ std::move(result) }
            : std::nullopt;
    }
};

} // namespace consolidator::messaging
