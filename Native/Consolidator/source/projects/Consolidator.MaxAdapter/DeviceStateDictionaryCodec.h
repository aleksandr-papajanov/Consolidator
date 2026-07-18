#pragma once

#include "DictionaryCodec.h"
#include "Models/DeviceState.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
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
        const auto filters = root.ReadObject("filters");
        if (!revision || !generation || !selectedBankId || !bankCount || !filters) {
            return std::nullopt;
        }

        auto filterIds = ReadLongs(root, "filter_order");
        std::sort(filterIds.begin(), filterIds.end());

        models::DeviceState state;
        state.revision = *revision;
        state.generation = *generation;
        state.snapshot.selectedBankId = *selectedBankId;
        const messaging::MessagePayload filterObjects{ *filters };
        for (const auto filterId : filterIds) {
            const auto source = filterObjects.ReadObject(std::to_string(filterId));
            if (!source) return std::nullopt;
            const auto definition = ReadDefinition(
                filterId,
                root.ReadBool("filter_" + std::to_string(filterId) + "_default_bypass").value_or(false),
                *source
            );
            if (!definition) return std::nullopt;
            state.filterDefinitions.push_back(*definition);
        }

        for (long bankId = 1; bankId <= *bankCount; ++bankId) {
            const auto prefix = BankPrefix(bankId);
            const auto name = root.ReadString(prefix + "name");
            if (!name) return std::nullopt;
            models::EqBank bank;
            bank.bankId = bankId;
            bank.name = *name;
            for (const auto filterId : filterIds) {
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
        messaging::MessageArray filterOrder;
        messaging::MessageObject filters;
        for (const auto& definition : state.filterDefinitions) {
            filterOrder.emplace_back(static_cast<std::int64_t>(definition.filterId));
            filters[std::to_string(definition.filterId)] = WriteDefinition(definition);
            root["filter_" + std::to_string(definition.filterId) + "_default_bypass"] =
                definition.defaultBypass;
        }
        root["filter_order"] = std::move(filterOrder);
        root["filters"] = std::move(filters);

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
    static std::optional<models::FilterDefinition> ReadDefinition(
        long filterId,
        bool defaultBypass,
        const messaging::MessageObject& object
    ) {
        const messaging::MessagePayload source{ object };
        const auto typeName = source.ReadString("type");
        const auto color = source.ReadString("color");
        const auto parameterObjects = source.ReadObject("parameters");
        if (!typeName || !color || !parameterObjects) return std::nullopt;
        const auto type = ParseType(*typeName);
        if (!type) return std::nullopt;

        models::FilterDefinition definition;
        definition.filterId = filterId;
        definition.type = *type;
        definition.defaultBypass = defaultBypass;
        definition.color = ParseColor(*color);
        const messaging::MessagePayload parameters{ *parameterObjects };
        for (const auto& name : ParameterNames(*type)) {
            const auto parameterObject = parameters.ReadObject(name);
            if (!parameterObject) return std::nullopt;
            const messaging::MessagePayload parameter{ *parameterObject };
            const auto scale = parameter.ReadString("scale");
            const auto minimum = parameter.ReadDouble("min");
            const auto maximum = parameter.ReadDouble("max");
            const auto defaultValue = parameter.ReadDouble("default");
            if (!scale || !minimum || !maximum || !defaultValue) return std::nullopt;

            models::FilterParameterDefinition result;
            result.name = name;
            result.defaultValue = *defaultValue;
            if (*scale == "linear") result.range.scale = models::ParameterScale::Linear;
            else if (*scale == "logarithmic") result.range.scale = models::ParameterScale::Logarithmic;
            else return std::nullopt;
            result.range.minimum = *minimum;
            result.range.maximum = *maximum;
            definition.parameters.push_back(std::move(result));
        }
        return definition;
    }

    static messaging::MessageObject WriteDefinition(const models::FilterDefinition& definition) {
        messaging::MessageObject parameters;
        for (const auto& parameter : definition.parameters) {
            parameters[parameter.name] = messaging::MessageObject{
                { "scale", parameter.range.scale == models::ParameterScale::Logarithmic
                    ? "logarithmic" : "linear" },
                { "min", parameter.range.minimum },
                { "max", parameter.range.maximum },
                { "default", parameter.defaultValue }
            };
        }
        return {
            { "slot", static_cast<std::int64_t>(definition.filterId) },
            { "type", TypeName(definition.type) },
            { "color", ColorName(definition.color) },
            { "parameters", std::move(parameters) }
        };
    }

    static std::vector<long> ReadLongs(
        const messaging::MessagePayload& object,
        const std::string& key
    ) {
        if (const auto single = object.ReadLong(key)) return { *single };
        const auto array = object.ReadArray(key);
        if (!array) return {};
        std::vector<long> values;
        values.reserve(array->size());
        for (const auto& value : *array) {
            const auto integer = value.As<std::int64_t>();
            if (!integer) return {};
            values.push_back(static_cast<long>(*integer));
        }
        return values;
    }

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
            else if (const auto integer = value.As<std::int64_t>()) values.push_back(static_cast<double>(*integer));
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

    static std::optional<models::FilterType> ParseType(const std::string& type) {
        if (type == "gain") return models::FilterType::Gain;
        if (type == "tilt") return models::FilterType::Tilt;
        if (type == "peak") return models::FilterType::Peak;
        if (type == "lowshelf") return models::FilterType::LowShelf;
        if (type == "highshelf") return models::FilterType::HighShelf;
        return std::nullopt;
    }

    static const char* TypeName(models::FilterType type) {
        if (type == models::FilterType::Gain) return "gain";
        if (type == models::FilterType::Tilt) return "tilt";
        if (type == models::FilterType::LowShelf) return "lowshelf";
        if (type == models::FilterType::HighShelf) return "highshelf";
        return "peak";
    }

    static std::vector<std::string> ParameterNames(models::FilterType type) {
        if (type == models::FilterType::Gain) return { "gain" };
        if (type == models::FilterType::Tilt) return { "gain", "pivot" };
        return { "gain", "freq", "q" };
    }

    static std::array<double, 4> ParseColor(std::string text) {
        if (!text.empty() && text.front() == '#') text.erase(text.begin());
        if (text.size() != 6 && text.size() != 8) return { 1.0, 1.0, 1.0, 1.0 };
        for (const auto character : text) {
            if (!std::isxdigit(static_cast<unsigned char>(character))) {
                return { 1.0, 1.0, 1.0, 1.0 };
            }
        }
        const auto component = [&text](std::size_t offset) {
            return static_cast<double>(std::stoul(text.substr(offset, 2), nullptr, 16)) / 255.0;
        };
        return { component(0), component(2), component(4),
            text.size() == 8 ? component(6) : 1.0 };
    }

    static std::string ColorName(const std::array<double, 4>& color) {
        std::ostringstream result;
        result << '#';
        for (const auto component : color) {
            result << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                << std::clamp(static_cast<int>(std::lround(component * 255.0)), 0, 255);
        }
        return result.str();
    }

    static std::string BankPrefix(long bankId) {
        return "bank_" + std::to_string(bankId) + "_";
    }
};

} // namespace consolidator::maxadapter
