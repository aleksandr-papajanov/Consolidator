#pragma once

#include "AtomWriter.h"
#include "AtomReader.h"
#include "Definitions/Definitions.h"
#include "States/States.h"
#include "Snapshots/Snapshots.h"
#include "Settings/CompressorOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"

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
                .Write(std::string{ models::EqSectionName(definition.section) })
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

    static AtomList EncodeProcessorDefinitions() {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "processor_definitions" })
            .Write(static_cast<std::int64_t>(1))
            .Write(static_cast<std::int64_t>(4))
            .Write(std::string{ "input_gain" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "gain" }).Write(settings::GainOptions::MinimumGainDb).Write(settings::GainOptions::MaximumGainDb).Write(static_cast<std::int64_t>(0)).Write(settings::GainOptions::DefaultGainDb)
            .Write(std::string{ "compressor" }).Write(static_cast<std::int64_t>(3))
            .Write(std::string{ "attack" }).Write(settings::CompressorOptions::MinimumAttackMs).Write(settings::CompressorOptions::MaximumAttackMs).Write(static_cast<std::int64_t>(1)).Write(settings::CompressorOptions::DefaultAttackMs)
            .Write(std::string{ "release" }).Write(settings::CompressorOptions::MinimumReleaseMs).Write(settings::CompressorOptions::MaximumReleaseMs).Write(static_cast<std::int64_t>(1)).Write(settings::CompressorOptions::DefaultReleaseMs)
            .Write(std::string{ "threshold" }).Write(settings::CompressorOptions::MinimumThresholdDb).Write(settings::CompressorOptions::MaximumThresholdDb).Write(static_cast<std::int64_t>(0)).Write(settings::CompressorOptions::DefaultThresholdDb)
            .Write(std::string{ "saturator" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "saturation" }).Write(settings::SaturatorOptions::MinimumSaturation).Write(settings::SaturatorOptions::MaximumSaturation).Write(static_cast<std::int64_t>(0)).Write(settings::SaturatorOptions::DefaultSaturation)
            .Write(std::string{ "output_gain" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "gain" }).Write(settings::GainOptions::MinimumGainDb).Write(settings::GainOptions::MaximumGainDb).Write(static_cast<std::int64_t>(0)).Write(settings::GainOptions::DefaultGainDb);
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
                .Write(bank.preBypass).Write(bank.postBypass)
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
            const auto preBypass = reader.ReadBool();
            const auto postBypass = reader.ReadBool();
            const auto filterCount = reader.ReadInt();
            if (!bankId || !name || !preBypass || !postBypass || !filterCount || *bankId <= previousBankId ||
                *bankId > std::numeric_limits<long>::max() ||
                name->empty() || *filterCount < 0) return std::nullopt;
            previousBankId = *bankId;
            models::EqBank bank;
            bank.bankId = static_cast<long>(*bankId);
            bank.name = *name;
            bank.preBypass = *preBypass;
            bank.postBypass = *postBypass;
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

    static AtomList EncodeDsp(const domain::DspSnapshot& snapshot) {
        auto atoms = EncodeEq(snapshot.eq, snapshot.revision);
        atoms[3] = std::string{ "dsp" };
        atoms.emplace_back(snapshot.processor.inputGain.gainDb);
        atoms.emplace_back(snapshot.processor.compressor.bypass);
        atoms.emplace_back(snapshot.processor.compressor.attackMs);
        atoms.emplace_back(snapshot.processor.compressor.releaseMs);
        atoms.emplace_back(snapshot.processor.compressor.thresholdDb);
        atoms.emplace_back(snapshot.processor.saturator.bypass);
        atoms.emplace_back(snapshot.processor.saturator.saturation);
        atoms.emplace_back(snapshot.processor.outputGain.gainDb);
        return atoms;
    }

    static std::optional<domain::DspSnapshot> DecodeDsp(const AtomList& atoms) {
        constexpr std::size_t processorFieldCount = 8;
        if (atoms.size() <= processorFieldCount ||
            !std::holds_alternative<std::string>(atoms[3]) ||
            std::get<std::string>(atoms[3]) != "dsp") return std::nullopt;

        AtomList eqAtoms(atoms.begin(), atoms.end() - processorFieldCount);
        eqAtoms[3] = std::string{ "eq" };
        auto eq = DecodeEq(eqAtoms);
        if (!eq) return std::nullopt;

        AtomList processorAtoms(atoms.end() - processorFieldCount, atoms.end());
        AtomReader reader(processorAtoms);
        const auto inputGain = reader.ReadDouble();
        const auto compressorBypass = reader.ReadBool();
        const auto attack = reader.ReadDouble();
        const auto release = reader.ReadDouble();
        const auto threshold = reader.ReadDouble();
        const auto saturatorBypass = reader.ReadBool();
        const auto saturation = reader.ReadDouble();
        const auto outputGain = reader.ReadDouble();
        if (!inputGain || !compressorBypass || !attack || !release || !threshold ||
            !saturatorBypass || !saturation || !outputGain || !reader.RequireEnd()) return std::nullopt;

        domain::DspSnapshot result;
        result.eq = std::move(*eq);
        result.processor.inputGain = { *inputGain };
        result.processor.compressor = { *attack, *release, *threshold, *compressorBypass };
        result.processor.saturator = { *saturation, *saturatorBypass };
        result.processor.outputGain = { *outputGain };
        if (const auto revision = std::get_if<std::int64_t>(&atoms[4])) result.revision = *revision;
        else return std::nullopt;
        return result;
    }
};

} // namespace consolidator::messaging
