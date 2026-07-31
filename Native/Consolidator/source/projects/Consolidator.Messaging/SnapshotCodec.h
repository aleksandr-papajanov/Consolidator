#pragma once

#include "AtomWriter.h"
#include "AtomReader.h"
#include "States/States.h"
#include "Snapshots/Snapshots.h"

#include <cmath>
#include <array>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

namespace consolidator::messaging {

class SnapshotCodec final {
    static void AppendProcessorState(
        AtomList& atoms,
        const domain::ProcessorState& processor
    ) {
        atoms.emplace_back(processor.inputGain.gainDb);
        atoms.emplace_back(processor.compressor.bypass);
        atoms.emplace_back(processor.compressor.attackMs);
        atoms.emplace_back(processor.compressor.releaseMs);
        atoms.emplace_back(processor.compressor.thresholdDb);
        atoms.emplace_back(processor.compressor.outputDb);
        atoms.emplace_back(processor.compressor.mix);
        for (const auto& filter : processor.compressor.detectorFilters) {
            atoms.emplace_back(filter.bypass);
            atoms.emplace_back(filter.gainDb);
            atoms.emplace_back(filter.frequencyHz);
            atoms.emplace_back(filter.q);
        }
        atoms.emplace_back(static_cast<std::int64_t>(
            processor.compressor.detectorListen));
        atoms.emplace_back(processor.saturator.bypass);
        atoms.emplace_back(processor.saturator.saturation);
        atoms.emplace_back(processor.saturator.outputDb);
        for (const auto& filter : processor.saturator.detectorFilters) {
            atoms.emplace_back(filter.bypass);
            atoms.emplace_back(filter.gainDb);
            atoms.emplace_back(filter.frequencyHz);
            atoms.emplace_back(filter.q);
        }
        atoms.emplace_back(static_cast<std::int64_t>(
            processor.saturator.detectorListen));
        atoms.emplace_back(processor.outputGain.gainDb);
    }

    static std::optional<domain::ProcessorState> DecodeProcessorFields(
        const AtomList& atoms
    ) {
        AtomReader reader(atoms);
        const auto inputGain = reader.ReadDouble();
        const auto compressorBypass = reader.ReadBool();
        const auto attack = reader.ReadDouble();
        const auto release = reader.ReadDouble();
        const auto threshold = reader.ReadDouble();
        const auto output = reader.ReadDouble();
        const auto compressorMix = reader.ReadDouble();
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
        const auto saturation = reader.ReadDouble();
        const auto saturatorOutput = reader.ReadDouble();
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
        if (!inputGain || !compressorBypass || !attack || !release ||
            !threshold || !output || !compressorMix ||
            !compressorDetectorListen || *compressorDetectorListen < 0 ||
            *compressorDetectorListen > 3 || !saturatorBypass ||
            !saturation || !saturatorOutput || !saturatorDetectorListen ||
            *saturatorDetectorListen < 0 || *saturatorDetectorListen > 3 ||
            !outputGain || !reader.RequireEnd()) return std::nullopt;

        domain::ProcessorState result;
        result.inputGain = { *inputGain };
        result.compressor.attackMs = *attack;
        result.compressor.releaseMs = *release;
        result.compressor.thresholdDb = *threshold;
        result.compressor.outputDb = *output;
        result.compressor.mix = *compressorMix;
        result.compressor.detectorFilters = compressorFilters;
        result.compressor.detectorListen =
            static_cast<long>(*compressorDetectorListen);
        result.compressor.bypass = *compressorBypass;
        result.saturator.saturation = *saturation;
        result.saturator.outputDb = *saturatorOutput;
        result.saturator.detectorFilters = saturatorFilters;
        result.saturator.detectorListen =
            static_cast<long>(*saturatorDetectorListen);
        result.saturator.bypass = *saturatorBypass;
        result.outputGain = { *outputGain };
        return result;
    }

public:
    static AtomList EncodeDevice(std::string instanceId) {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "device" })
            .Write(static_cast<std::int64_t>(1)).Write(std::move(instanceId));
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
        AppendProcessorState(atoms, snapshot.processor);
        return atoms;
    }

    static AtomList EncodeProcessor(
        const domain::ProcessorState& processor,
        domain::StoreRevision revision
    ) {
        AtomWriter writer;
        writer.Write(std::string{ "snapshot" }).Write(static_cast<std::int64_t>(1))
            .Write(std::string{ "host" }).Write(std::string{ "processor" })
            .Write(static_cast<std::int64_t>(revision));
        auto atoms = std::move(writer).Finish();
        AppendProcessorState(atoms, processor);
        return atoms;
    }

    static std::optional<domain::ProcessorState> DecodeProcessor(
        const AtomList& atoms
    ) {
        constexpr std::size_t headerFieldCount = 5;
        if (atoms.size() != headerFieldCount + 29 ||
            !std::holds_alternative<std::string>(atoms[0]) ||
            std::get<std::string>(atoms[0]) != "snapshot" ||
            !std::holds_alternative<std::int64_t>(atoms[1]) ||
            std::get<std::int64_t>(atoms[1]) != 1 ||
            !std::holds_alternative<std::string>(atoms[2]) ||
            std::get<std::string>(atoms[2]) != "host" ||
            !std::holds_alternative<std::string>(atoms[3]) ||
            std::get<std::string>(atoms[3]) != "processor" ||
            !std::holds_alternative<std::int64_t>(atoms[4]) ||
            std::get<std::int64_t>(atoms[4]) < 0) return std::nullopt;
        return DecodeProcessorFields(AtomList(
            atoms.begin() + headerFieldCount, atoms.end()
        ));
    }

    static std::optional<domain::DspSnapshot> DecodeDsp(const AtomList& atoms) {
        constexpr std::size_t processorFieldCount = 29;
        if (atoms.size() <= processorFieldCount ||
            !std::holds_alternative<std::string>(atoms[3]) ||
            std::get<std::string>(atoms[3]) != "dsp") return std::nullopt;

        AtomList eqAtoms(atoms.begin(), atoms.end() - processorFieldCount);
        eqAtoms[3] = std::string{ "eq" };
        auto eq = DecodeEq(eqAtoms);
        if (!eq) return std::nullopt;

        auto processor = DecodeProcessorFields(AtomList(
            atoms.end() - processorFieldCount, atoms.end()
        ));
        if (!processor) return std::nullopt;

        domain::DspSnapshot result;
        result.eq = std::move(*eq);
        result.processor = std::move(*processor);
        if (const auto revision = std::get_if<std::int64_t>(&atoms[4])) result.revision = *revision;
        else return std::nullopt;
        return result;
    }
};

} // namespace consolidator::messaging
