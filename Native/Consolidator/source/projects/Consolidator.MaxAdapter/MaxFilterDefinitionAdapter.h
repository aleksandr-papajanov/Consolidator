#pragma once

#include "c74_min.h"
#include "Models/FilterDefinition.h"

#include <array>
#include <cctype>
#include <optional>
#include <string>
#include <vector>

namespace consolidator::maxadapter {

class MaxFilterDefinitionAdapter final {
public:
    static std::optional<models::FilterDefinition> Read(
        const std::string& dictionaryName,
        long filterId,
        bool defaultBypass = false
    ) {
        try {
            c74::min::dict root{ c74::min::symbol{ dictionaryName.c_str() } };
            c74::min::dict filters{ static_cast<c74::min::atom>(root.at("filters")) };
            c74::min::dict source{ static_cast<c74::min::atom>(filters.at(std::to_string(filterId))) };
            const auto type = ParseType(ReadString(source, "type"));
            if (!type) return std::nullopt;

            models::FilterDefinition definition;
            definition.filterId = filterId;
            definition.type = *type;
            definition.defaultBypass = defaultBypass;
            definition.color = ParseColor(ReadString(source, "color"));
            c74::min::dict parameters{ static_cast<c74::min::atom>(source.at("parameters")) };
            for (const auto& name : ParameterNames(*type)) {
                const auto parameter = ReadParameter(parameters, name);
                if (!parameter) return std::nullopt;
                definition.parameters.push_back(*parameter);
            }
            return definition;
        }
        catch (...) {
            return std::nullopt;
        }
    }

private:
    static std::string ReadString(c74::min::dict& dictionary, const char* key) {
        return static_cast<std::string>(static_cast<c74::min::atom>(dictionary.at(key)));
    }

    static double ReadNumber(c74::min::dict& dictionary, const char* key) {
        return static_cast<double>(static_cast<c74::min::atom>(dictionary.at(key)));
    }

    static std::optional<models::FilterType> ParseType(const std::string& type) {
        if (type == "gain") return models::FilterType::Gain;
        if (type == "tilt") return models::FilterType::Tilt;
        if (type == "peak") return models::FilterType::Peak;
        if (type == "lowshelf") return models::FilterType::LowShelf;
        if (type == "highshelf") return models::FilterType::HighShelf;
        return std::nullopt;
    }

    static std::vector<std::string> ParameterNames(models::FilterType type) {
        if (type == models::FilterType::Gain) return { "gain" };
        if (type == models::FilterType::Tilt) return { "gain", "pivot" };
        return { "gain", "freq", "q" };
    }

    static std::optional<models::FilterParameterDefinition> ReadParameter(
        c74::min::dict& parameters,
        const std::string& name
    ) {
        try {
            c74::min::dict source{ static_cast<c74::min::atom>(parameters.at(name)) };
            models::FilterParameterDefinition parameter;
            parameter.name = name;
            parameter.defaultValue = ReadNumber(source, "default");
            const auto scale = ReadString(source, "scale");
            if (scale == "linear") parameter.range.scale = models::ParameterScale::Linear;
            else if (scale == "logarithmic") parameter.range.scale = models::ParameterScale::Logarithmic;
            else return std::nullopt;

            parameter.range.minimum = ReadNumber(source, "min");
            parameter.range.maximum = ReadNumber(source, "max");
            return parameter;
        }
        catch (...) {
            return std::nullopt;
        }
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
};

} // namespace consolidator::maxadapter
