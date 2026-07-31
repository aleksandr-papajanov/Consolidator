#pragma once

#include "AtomReader.h"
#include "Models/FilterDefinition.h"

#include <cmath>
#include <map>
#include <optional>
#include <string>

namespace consolidator::approximator {

class ApproximatorDefinitionCodec final {
public:
    using Definitions = std::map<long, models::FilterDefinition>;

    static std::optional<Definitions> Decode(const messaging::AtomList& atoms) {
        messaging::AtomReader reader(atoms);
        const auto count = reader.ReadInt();
        if (!count || *count < 1 || *count > 64) return std::nullopt;

        Definitions definitions;
        for (long index = 0; index < *count; ++index) {
            auto definition = ReadDefinition(reader);
            if (!definition || !definitions.emplace(definition->filterId, std::move(*definition)).second) {
                return std::nullopt;
            }
        }
        return reader.RequireEnd() ? std::optional<Definitions>{ std::move(definitions) } : std::nullopt;
    }

private:
    static std::optional<models::FilterDefinition> ReadDefinition(messaging::AtomReader& reader) {
        const auto filterId = reader.ReadInt();
        const auto typeName = reader.ReadString();
        const auto defaultBypass = reader.ReadBool();
        const auto parameterCount = reader.ReadInt();
        const auto type = typeName ? ParseType(*typeName) : std::nullopt;
        if (!filterId || *filterId < 1 || !type || !defaultBypass || !parameterCount ||
            *parameterCount < 1 || *parameterCount > 8) {
            return std::nullopt;
        }

        models::FilterDefinition definition;
        definition.filterId = static_cast<long>(*filterId);
        definition.type = *type;
        definition.defaultBypass = *defaultBypass;
        definition.parameters.reserve(static_cast<std::size_t>(*parameterCount));
        for (long parameterIndex = 0; parameterIndex < *parameterCount; ++parameterIndex) {
            const auto parameter = ReadParameter(reader);
            if (!parameter) return std::nullopt;
            definition.parameters.push_back(*parameter);
        }
        return definition;
    }

    static std::optional<models::FilterParameterDefinition> ReadParameter(
        messaging::AtomReader& reader
    ) {
        const auto name = reader.ReadString();
        const auto minimum = reader.ReadDouble();
        const auto maximum = reader.ReadDouble();
        const auto logarithmic = reader.ReadBool();
        const auto defaultValue = reader.ReadDouble();
        if (!name || name->empty() || !minimum || !maximum || !logarithmic || !defaultValue ||
            !std::isfinite(*minimum) || !std::isfinite(*maximum) || !std::isfinite(*defaultValue) ||
            *minimum >= *maximum || *defaultValue < *minimum || *defaultValue > *maximum ||
            (*logarithmic && *minimum <= 0.0)) {
            return std::nullopt;
        }
        return models::FilterParameterDefinition{
            *name,
            { *minimum, *maximum, *logarithmic ? models::ParameterScale::Logarithmic : models::ParameterScale::Linear },
            *defaultValue
        };
    }

    static std::optional<models::FilterType> ParseType(const std::string& value) {
        using models::FilterType;
        if (value == "gain") return FilterType::Gain;
        if (value == "tilt") return FilterType::Tilt;
        if (value == "peak") return FilterType::Peak;
        if (value == "lowshelf") return FilterType::LowShelf;
        if (value == "highshelf") return FilterType::HighShelf;
        return std::nullopt;
    }
};

} // namespace consolidator::approximator
