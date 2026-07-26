#pragma once

#include "AtomWriter.h"
#include "AtomReader.h"
#include "Definitions/Definitions.h"
#include "States/States.h"
#include "Snapshots/Snapshots.h"
#include "Settings/CompressorOptions.h"
#include "Settings/DetectorFilterOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"

#include <cmath>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace consolidator::messaging {

class SnapshotCodec final {
public:
    static AtomList EncodeDevice(std::string instanceId) {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "device" })
            .Write(static_cast<std::int64_t>(1)).Write(std::move(instanceId));
        return std::move(writer).Finish();
    }

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

    static void WriteDetectorDefinitions(AtomWriter& writer) {
        const auto& definition = settings::DetectorFilterOptions::Definition();
        for (long filterId = 1; filterId <= 2; ++filterId) {
            for (const auto& parameter : definition.parameters) {
                writer.Write(std::string{ "detector." } + std::to_string(filterId) + "." + parameter.name)
                    .Write(parameter.range.minimum)
                    .Write(parameter.range.maximum)
                    .Write(static_cast<std::int64_t>(parameter.range.scale))
                    .Write(parameter.defaultValue);
            }
            writer.Write(std::string{ "detector." } + std::to_string(filterId) + ".bypass")
                .Write(0.0).Write(1.0)
                .Write(static_cast<std::int64_t>(0)).Write(0.0);
        }
    }

    static AtomList EncodeProcessorDefinitions() {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "processor_definitions" })
            .Write(static_cast<std::int64_t>(1))
            .Write(static_cast<std::int64_t>(4))
            .Write(std::string{ "input_gain" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "gain" }).Write(settings::GainOptions::MinimumGainDb).Write(settings::GainOptions::MaximumGainDb).Write(static_cast<std::int64_t>(0)).Write(settings::GainOptions::DefaultGainDb)
            .Write(std::string{ "compressor" }).Write(static_cast<std::int64_t>(14))
            .Write(std::string{ "attack" }).Write(settings::CompressorOptions::MinimumAttackMs).Write(settings::CompressorOptions::MaximumAttackMs).Write(static_cast<std::int64_t>(1)).Write(settings::CompressorOptions::DefaultAttackMs)
            .Write(std::string{ "release" }).Write(settings::CompressorOptions::MinimumReleaseMs).Write(settings::CompressorOptions::MaximumReleaseMs).Write(static_cast<std::int64_t>(1)).Write(settings::CompressorOptions::DefaultReleaseMs)
            .Write(std::string{ "input" }).Write(settings::CompressorOptions::MinimumInputDb).Write(settings::CompressorOptions::MaximumInputDb).Write(static_cast<std::int64_t>(0)).Write(settings::CompressorOptions::DefaultInputDb)
            .Write(std::string{ "output" }).Write(settings::CompressorOptions::MinimumOutputDb).Write(settings::CompressorOptions::MaximumOutputDb).Write(static_cast<std::int64_t>(0)).Write(settings::CompressorOptions::DefaultOutputDb)
            .Write(std::string{ "mix" }).Write(settings::CompressorOptions::MinimumMix).Write(settings::CompressorOptions::MaximumMix).Write(static_cast<std::int64_t>(0)).Write(settings::CompressorOptions::DefaultMix)
            .Write(std::string{ "mode" }).Write(static_cast<double>(0)).Write(static_cast<double>(settings::CompressorOptions::ModeCount - 1)).Write(static_cast<std::int64_t>(0)).Write(static_cast<double>(settings::CompressorOptions::DefaultMode))
            ;
        WriteDetectorDefinitions(writer);
        writer
            .Write(std::string{ "saturator" }).Write(static_cast<std::int64_t>(12))
            .Write(std::string{ "input" }).Write(settings::SaturatorOptions::MinimumInputDb).Write(settings::SaturatorOptions::MaximumInputDb).Write(static_cast<std::int64_t>(0)).Write(settings::SaturatorOptions::DefaultInputDb)
            .Write(std::string{ "output" }).Write(settings::SaturatorOptions::MinimumOutputDb).Write(settings::SaturatorOptions::MaximumOutputDb).Write(static_cast<std::int64_t>(0)).Write(settings::SaturatorOptions::DefaultOutputDb)
            .Write(std::string{ "mix" }).Write(settings::SaturatorOptions::MinimumMix).Write(settings::SaturatorOptions::MaximumMix).Write(static_cast<std::int64_t>(0)).Write(settings::SaturatorOptions::DefaultMix)
            .Write(std::string{ "mode" }).Write(static_cast<double>(0)).Write(static_cast<double>(settings::SaturatorOptions::ModeCount - 1)).Write(static_cast<std::int64_t>(0)).Write(static_cast<double>(settings::SaturatorOptions::DefaultMode))
            ;
        WriteDetectorDefinitions(writer);
        writer
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
            .Write(state.bypass)
            .Write(state.solo)
            .Write(static_cast<std::int64_t>(state.banks.size()));
        for (const auto& bank : state.banks) {
            writer.Write(static_cast<std::int64_t>(bank.bankId)).Write(bank.linkId)
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
        const auto bypass = reader.ReadBool();
        const auto solo = reader.ReadBool();
        const auto bankCount = reader.ReadInt();
        if (!category || !version || !source || !store || !revision || !selectedBank || !bypass || !solo || !bankCount ||
            *category != "snapshot" || *version != 1 || *source != "host" || *store != "eq" ||
            *revision < 0 || !models::EqSnapshot::IsUserBankId(static_cast<long>(*selectedBank)) ||
            *bankCount != models::EqSnapshot::BankCount) {
            return std::nullopt;
        }

        domain::EqState result;
        result.selectedBankId = static_cast<long>(*selectedBank);
        result.bypass = *bypass;
        result.solo = *solo;
        for (long bankIndex = 0; bankIndex < *bankCount; ++bankIndex) {
            const auto bankId = reader.ReadInt();
            const auto linkId = reader.ReadString();
            const auto filterCount = reader.ReadInt();
            if (!bankId || !linkId || !filterCount || *bankId != bankIndex || *filterCount < 0) return std::nullopt;
            models::EqBank bank;
            bank.bankId = static_cast<long>(*bankId);
            bank.linkId = *linkId;
            for (long filterIndex = 0; filterIndex < *filterCount; ++filterIndex) {
                const auto filterId = reader.ReadInt();
                const auto bypass = reader.ReadBool();
                const auto valueCount = reader.ReadInt();
                if (!filterId || !bypass || !valueCount || *filterId < 1 ||
                    *filterId > std::numeric_limits<long>::max() ||
                    *valueCount < 0) return std::nullopt;
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
        atoms.emplace_back(snapshot.processor.compressor.inputDb);
        atoms.emplace_back(snapshot.processor.compressor.outputDb);
        atoms.emplace_back(snapshot.processor.compressor.mix);
        atoms.emplace_back(static_cast<std::int64_t>(snapshot.processor.compressor.mode));
        for (const auto& filter : snapshot.processor.compressor.detectorFilters) {
            atoms.emplace_back(filter.bypass);
            atoms.emplace_back(filter.gainDb);
            atoms.emplace_back(filter.frequencyHz);
            atoms.emplace_back(filter.q);
        }
        atoms.emplace_back(static_cast<std::int64_t>(snapshot.processor.compressor.detectorListen));
        atoms.emplace_back(snapshot.processor.saturator.bypass);
        atoms.emplace_back(snapshot.processor.saturator.inputDb);
        atoms.emplace_back(snapshot.processor.saturator.outputDb);
        atoms.emplace_back(snapshot.processor.saturator.mix);
        atoms.emplace_back(static_cast<std::int64_t>(snapshot.processor.saturator.mode));
        for (const auto& filter : snapshot.processor.saturator.detectorFilters) {
            atoms.emplace_back(filter.bypass);
            atoms.emplace_back(filter.gainDb);
            atoms.emplace_back(filter.frequencyHz);
            atoms.emplace_back(filter.q);
        }
        atoms.emplace_back(static_cast<std::int64_t>(snapshot.processor.saturator.detectorListen));
        atoms.emplace_back(snapshot.processor.outputGain.gainDb);
        return atoms;
    }

    static std::optional<domain::DspSnapshot> DecodeDsp(const AtomList& atoms) {
        constexpr std::size_t processorFieldCount = 32;
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
        const auto input = reader.ReadDouble();
        const auto output = reader.ReadDouble();
        const auto compressorMix = reader.ReadDouble();
        const auto compressorMode = reader.ReadInt();
        std::array<models::DetectorFilterState, 2> compressorFilters{};
        for (std::size_t index = 0; index < compressorFilters.size(); ++index) {
            compressorFilters[index].filterId = static_cast<long>(index + 1);
            const auto bypass = reader.ReadBool();
            const auto gain = reader.ReadDouble();
            const auto frequency = reader.ReadDouble();
            const auto q = reader.ReadDouble();
            if (!bypass || !gain || !frequency || !q) return std::nullopt;
            compressorFilters[index].bypass = *bypass;
            compressorFilters[index].gainDb = *gain;
            compressorFilters[index].frequencyHz = *frequency;
            compressorFilters[index].q = *q;
        }
        const auto compressorDetectorListen = reader.ReadInt();
        const auto saturatorBypass = reader.ReadBool();
        const auto saturatorInput = reader.ReadDouble();
        const auto saturatorOutput = reader.ReadDouble();
        const auto saturatorMix = reader.ReadDouble();
        const auto saturatorMode = reader.ReadInt();
        std::array<models::DetectorFilterState, 2> saturatorFilters{};
        for (std::size_t index = 0; index < saturatorFilters.size(); ++index) {
            saturatorFilters[index].filterId = static_cast<long>(index + 1);
            const auto bypass = reader.ReadBool();
            const auto gain = reader.ReadDouble();
            const auto frequency = reader.ReadDouble();
            const auto q = reader.ReadDouble();
            if (!bypass || !gain || !frequency || !q) return std::nullopt;
            saturatorFilters[index].bypass = *bypass;
            saturatorFilters[index].gainDb = *gain;
            saturatorFilters[index].frequencyHz = *frequency;
            saturatorFilters[index].q = *q;
        }
        const auto saturatorDetectorListen = reader.ReadInt();
        const auto outputGain = reader.ReadDouble();
        if (!inputGain || !compressorBypass || !attack || !release || !input || !output || !compressorMix || !compressorMode ||
            !compressorDetectorListen || *compressorDetectorListen < 0 || *compressorDetectorListen > 2 ||
            !saturatorBypass || !saturatorInput || !saturatorOutput || !saturatorMix ||
            !saturatorMode || !saturatorDetectorListen ||
            *saturatorDetectorListen < 0 || *saturatorDetectorListen > 2 || !outputGain || !reader.RequireEnd()) return std::nullopt;

        domain::DspSnapshot result;
        result.eq = std::move(*eq);
        result.processor.inputGain = { *inputGain };
        result.processor.compressor.attackMs = *attack;
        result.processor.compressor.releaseMs = *release;
        result.processor.compressor.inputDb = *input;
        result.processor.compressor.outputDb = *output;
        result.processor.compressor.mix = *compressorMix;
        result.processor.compressor.mode = static_cast<long>(*compressorMode);
        result.processor.compressor.detectorFilters = compressorFilters;
        result.processor.compressor.detectorListen = static_cast<long>(*compressorDetectorListen);
        result.processor.compressor.bypass = *compressorBypass;
        result.processor.saturator.inputDb = *saturatorInput;
        result.processor.saturator.outputDb = *saturatorOutput;
        result.processor.saturator.mix = *saturatorMix;
        result.processor.saturator.mode = static_cast<long>(*saturatorMode);
        result.processor.saturator.detectorFilters = saturatorFilters;
        result.processor.saturator.detectorListen = static_cast<long>(*saturatorDetectorListen);
        result.processor.saturator.bypass = *saturatorBypass;
        result.processor.outputGain = { *outputGain };
        if (const auto revision = std::get_if<std::int64_t>(&atoms[4])) result.revision = *revision;
        else return std::nullopt;
        return result;
    }
};

} // namespace consolidator::messaging
