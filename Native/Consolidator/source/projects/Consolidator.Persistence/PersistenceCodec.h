#pragma once

#include "Messaging/MessagePayload.h"
#include "Definitions/BankNameGenerator.h"
#include "Models/EqSnapshot.h"
#include "Models/ProcessorState.h"
#include "Settings/FilterOptions.h"

#include <cstdint>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>

namespace consolidator::persistence {

struct PersistedDeviceState {
    long schemaVersion = 11;
    models::EqSnapshot eq;
    models::ProcessorState processor;
};

class PersistenceCodec final {
public:
    static constexpr long SchemaVersion = 11;

    static PersistedDeviceState Defaults() {
        PersistedDeviceState result{ SchemaVersion, {}, {} };
        result.eq.selectedBankId = 1;
        result.eq.banks.push_back({ 1, domain::BankNameGenerator::Generate(1), false, {} });
        for (const auto& [filterId, definition] : settings::FilterOptions::EqDefinitions()) {
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
            { "compressor.input", state.processor.compressor.inputDb },
            { "compressor.output", state.processor.compressor.outputDb },
            { "compressor.mix", state.processor.compressor.mix },
            { "compressor.mode", static_cast<std::int64_t>(state.processor.compressor.mode) },
            { "compressor.bypass", state.processor.compressor.bypass },
            { "saturator.input", state.processor.saturator.inputDb },
            { "saturator.output", state.processor.saturator.outputDb },
            { "saturator.mix", state.processor.saturator.mix },
            { "saturator.mode", static_cast<std::int64_t>(state.processor.saturator.mode) },
            { "saturator.bypass", state.processor.saturator.bypass },
            { "output_gain.gain", state.processor.outputGain.gainDb }
        };
        for (std::size_t index = 0; index < 2; ++index) {
            const auto& compressorFilter = state.processor.compressor.detectorFilters[index];
            const auto compressorPrefix = "compressor.detector." + std::to_string(index + 1) + ".";
            result[compressorPrefix + "bypass"] = compressorFilter.bypass;
            result[compressorPrefix + "gain"] = compressorFilter.gainDb;
            result[compressorPrefix + "frequency"] = compressorFilter.frequencyHz;
            result[compressorPrefix + "q"] = compressorFilter.q;
            const auto& saturatorFilter = state.processor.saturator.detectorFilters[index];
            const auto saturatorPrefix = "saturator.detector." + std::to_string(index + 1) + ".";
            result[saturatorPrefix + "bypass"] = saturatorFilter.bypass;
            result[saturatorPrefix + "gain"] = saturatorFilter.gainDb;
            result[saturatorPrefix + "frequency"] = saturatorFilter.frequencyHz;
            result[saturatorPrefix + "q"] = saturatorFilter.q;
        }
        for (const auto& bank : state.eq.banks) {
            const auto prefix = "bank." + std::to_string(bank.bankId) + ".";
            result[prefix + "name"] = bank.name;
            result[prefix + "bypass"] = bank.bypass;
            result[prefix + "solo"] = bank.solo;
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
        const auto input = root.ReadDouble("compressor.input");
        const auto output = root.ReadDouble("compressor.output");
        const auto compressorMix = root.ReadDouble("compressor.mix");
        const auto compressorMode = root.ReadLong("compressor.mode");
        const auto compressorBypass = root.ReadBool("compressor.bypass");
        const auto saturatorInput = root.ReadDouble("saturator.input");
        const auto saturatorOutput = root.ReadDouble("saturator.output");
        const auto saturatorMix = root.ReadDouble("saturator.mix");
        const auto saturatorMode = root.ReadLong("saturator.mode");
        const auto saturatorBypass = root.ReadBool("saturator.bypass");
        const auto outputGain = root.ReadDouble("output_gain.gain");
        if (!inputGain || !attack || !release || !input || !output || !compressorMix || !compressorMode || !compressorBypass ||
            !saturatorInput || !saturatorOutput || !saturatorMix ||
            !saturatorMode || !saturatorBypass || !outputGain) {
            return std::nullopt;
        }
        result.processor.inputGain = { *inputGain };
        result.processor.compressor.attackMs = *attack;
        result.processor.compressor.releaseMs = *release;
        result.processor.compressor.inputDb = *input;
        result.processor.compressor.outputDb = *output;
        result.processor.compressor.mix = *compressorMix;
        result.processor.compressor.mode = *compressorMode;
        result.processor.compressor.bypass = *compressorBypass;
        result.processor.saturator.inputDb = *saturatorInput;
        result.processor.saturator.outputDb = *saturatorOutput;
        result.processor.saturator.mix = *saturatorMix;
        result.processor.saturator.mode = *saturatorMode;
        result.processor.saturator.bypass = *saturatorBypass;
        result.processor.outputGain = { *outputGain };
        for (std::size_t index = 0; index < 2; ++index) {
            auto& compressorFilter = result.processor.compressor.detectorFilters[index];
            auto& saturatorFilter = result.processor.saturator.detectorFilters[index];
            compressorFilter.filterId = static_cast<long>(index + 1);
            saturatorFilter.filterId = static_cast<long>(index + 1);
            const auto compressorPrefix = "compressor.detector." + std::to_string(index + 1) + ".";
            const auto saturatorPrefix = "saturator.detector." + std::to_string(index + 1) + ".";
            const auto compressorBypassValue = root.ReadBool(compressorPrefix + "bypass");
            const auto compressorGain = root.ReadDouble(compressorPrefix + "gain");
            const auto compressorFrequency = root.ReadDouble(compressorPrefix + "frequency");
            const auto compressorQ = root.ReadDouble(compressorPrefix + "q");
            const auto saturatorBypassValue = root.ReadBool(saturatorPrefix + "bypass");
            const auto saturatorGain = root.ReadDouble(saturatorPrefix + "gain");
            const auto saturatorFrequency = root.ReadDouble(saturatorPrefix + "frequency");
            const auto saturatorQ = root.ReadDouble(saturatorPrefix + "q");
            if (!compressorBypassValue || !compressorGain || !compressorFrequency || !compressorQ ||
                !saturatorBypassValue || !saturatorGain || !saturatorFrequency || !saturatorQ) return std::nullopt;
            compressorFilter.bypass = *compressorBypassValue;
            compressorFilter.gainDb = *compressorGain;
            compressorFilter.frequencyHz = *compressorFrequency;
            compressorFilter.q = *compressorQ;
            saturatorFilter.bypass = *saturatorBypassValue;
            saturatorFilter.gainDb = *saturatorGain;
            saturatorFilter.frequencyHz = *saturatorFrequency;
            saturatorFilter.q = *saturatorQ;
        }
        for (const auto& bankIdValue : *bankIds) {
            const auto decodedBankId = bankIdValue.As<std::int64_t>();
            if (!decodedBankId || *decodedBankId < 1 ||
                *decodedBankId > std::numeric_limits<long>::max()) return std::nullopt;
            const auto bankId = static_cast<long>(*decodedBankId);
            const auto prefix = "bank." + std::to_string(bankId) + ".";
            const auto name = root.ReadString(prefix + "name");
            const auto bypass = root.ReadBool(prefix + "bypass");
            const auto solo = root.ReadBool(prefix + "solo");
            if (!name || !bypass || !solo) return std::nullopt;
            models::EqBank bank{ bankId, *name, *bypass, *solo, {} };
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
