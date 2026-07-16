#pragma once

#include "c74_min.h"

#include "FilterContract.h"

#include <algorithm>
#include <string>
#include <vector>

using c74::min::atom;
using c74::min::dict;
using c74::min::number;

inline bool dictionary_atom(const atom& value) {
    return c74::max::atomisdictionary(const_cast<c74::max::t_atom*>(
        static_cast<const c74::max::t_atom*>(&value)));
}

inline bool read_dictionary_number(dict& source, const char* key, double& value) {
    try {
        value = static_cast<double>(static_cast<atom>(source.at(key)));
        return true;
    }
    catch (...) {
        return false;
    }
}

inline bool read_dictionary_symbol(dict& source, const char* key, std::string& value) {
    try {
        value = static_cast<std::string>(static_cast<atom>(source.at(key)));
        return true;
    }
    catch (...) {
        return false;
    }
}

inline bool parse_parameter_scale(const std::string& value, ParameterScale& scale) {
    if (value == "linear") {
        scale = ParameterScale::linear;
        return true;
    }
    if (value == "logarithmic") {
        scale = ParameterScale::logarithmic;
        return true;
    }
    if (value == "discrete") {
        scale = ParameterScale::discrete;
        return true;
    }
    return false;
}

inline bool parse_filter_contract_definition(
    FilterContract& contract,
    dict& source
) {
    try {
        double slot_value = 0.0;
        std::string type_name;
        if (!read_dictionary_number(source, "slot", slot_value) ||
            !read_dictionary_symbol(source, "type", type_name)) {
            return false;
        }

        const auto type = try_filter_type_from_symbol(type_name);
        if (!type) {
            return false;
        }

        contract = make_default_contract(static_cast<int>(slot_value), *type);
        dict parameters{ static_cast<atom>(source.at("parameters")) };

        for (auto& parameter : contract.parameters) {
            atom parameter_value = static_cast<atom>(parameters.at(parameter.name));

            if (dictionary_atom(parameter_value)) {
                dict definition{ parameter_value };
                std::string scale;
                double default_value = 0.0;
                if (!read_dictionary_symbol(definition, "scale", scale) ||
                    !read_dictionary_number(definition, "default", default_value)) {
                    return false;
                }

                ParameterScale parsed_scale = ParameterScale::linear;
                if (!parse_parameter_scale(scale, parsed_scale)) {
                    return false;
                }
                parameter.range.scale = parsed_scale;
                parameter.default_value = default_value;

                if (parsed_scale == ParameterScale::discrete) {
                    const auto values = static_cast<std::vector<number>>(definition.at("values"));
                    if (values.empty()) {
                        return false;
                    }

                    parameter.range.discrete_values.assign(values.begin(), values.end());
                    const auto [minimum, maximum] = std::minmax_element(
                        parameter.range.discrete_values.begin(),
                        parameter.range.discrete_values.end());
                    parameter.range.min_value = *minimum;
                    parameter.range.max_value = *maximum;

                    const bool valid_default = std::any_of(
                        parameter.range.discrete_values.begin(),
                        parameter.range.discrete_values.end(),
                        [default_value](double value) { return value == default_value; });
                    if (!valid_default) {
                        return false;
                    }
                }
                else {
                    if (!read_dictionary_number(definition, "min", parameter.range.min_value) ||
                        !read_dictionary_number(definition, "max", parameter.range.max_value)) {
                        return false;
                    }
                    parameter.range.discrete_values.clear();
                }
            }
            else {
                const auto values = static_cast<std::vector<number>>(parameter_value);
                if (values.size() != 3) {
                    return false;
                }

                parameter.range.discrete_values.clear();
                parameter.range.min_value = values[0];
                parameter.range.max_value = values[1];
                parameter.default_value = values[2];
            }

            if (parameter.default_value < parameter.range.min_value ||
                parameter.default_value > parameter.range.max_value) {
                return false;
            }
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

inline bool parse_filter_contract_dictionary(
    FilterContract& contract,
    const atom& dictionary_atom_value
) {
    if (!dictionary_atom(dictionary_atom_value)) {
        return false;
    }

    try {
        dict source{ dictionary_atom_value };
        double selected_slot = 0.0;

        if (read_dictionary_number(source, "selected", selected_slot)) {
            dict filters{ static_cast<atom>(source.at("filters")) };
            const auto selected_key = std::to_string(static_cast<int>(selected_slot));
            dict definition{ static_cast<atom>(filters.at(selected_key)) };
            return parse_filter_contract_definition(contract, definition);
        }

        return parse_filter_contract_definition(contract, source);
    }
    catch (...) {
        return false;
    }
}

inline bool parse_filter_contract_dictionary_for_slot(
    FilterContract& contract,
    const atom& dictionary_atom_value,
    const int preferred_slot
) {
    if (!dictionary_atom(dictionary_atom_value)) {
        return false;
    }

    try {
        dict source{ dictionary_atom_value };
        double selected_slot = 0.0;
        if (read_dictionary_number(source, "selected", selected_slot)) {
            return parse_filter_contract_dictionary(contract, dictionary_atom_value);
        }

        dict filters{ static_cast<atom>(source.at("filters")) };
        const auto selected_key = std::to_string(preferred_slot);
        dict definition{ static_cast<atom>(filters.at(selected_key)) };
        return parse_filter_contract_definition(contract, definition);
    }
    catch (...) {
        return false;
    }
}
