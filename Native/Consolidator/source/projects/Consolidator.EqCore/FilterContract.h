#pragma once

#include "c74_min.h"

#include "FilterSpec.h"
#include "ParameterRange.h"

#include <optional>
#include <string>
#include <vector>

struct FilterParameter {
    std::string name;
    ParameterRange range;
    double default_value = 0.0;
};

struct FilterContract {
    int slot = 0;
    FilterType type = FilterType::peak;
    std::vector<FilterParameter> parameters;
};

inline std::optional<FilterType> try_filter_type_from_symbol(const std::string& value) {
    if (value == "gain") {
        return FilterType::gain;
    }

    if (value == "tilt") {
        return FilterType::tilt;
    }

    if (value == "peak") {
        return FilterType::peak;
    }

    if (value == "lowshelf" || value == "low_shelf") {
        return FilterType::low_shelf;
    }

    if (value == "highshelf" || value == "high_shelf") {
        return FilterType::high_shelf;
    }

    return std::nullopt;
}

inline FilterContract make_default_contract(int slot, FilterType type) {
    FilterContract contract;
    contract.slot = slot;
    contract.type = type;

    switch (type) {
        case FilterType::gain:
            contract.parameters = {
                { "gain", { -18.0, 18.0, ParameterScale::linear }, 0.0 }
            };
            break;
        case FilterType::tilt:
            contract.parameters = {
                { "gain", { -18.0, 18.0, ParameterScale::linear }, 0.0 },
                { "pivot", { 200.0, 4000.0, ParameterScale::logarithmic }, 1000.0 }
            };
            break;
        case FilterType::peak:
            contract.parameters = {
                { "gain", { -18.0, 18.0, ParameterScale::linear }, 0.0 },
                { "freq", { 40.0, 18000.0, ParameterScale::logarithmic }, 1000.0 },
                { "q", { 0.2, 8.0, ParameterScale::logarithmic }, 0.707 }
            };
            break;
        case FilterType::low_shelf:
        case FilterType::high_shelf:
            contract.parameters = {
                { "gain", { -18.0, 18.0, ParameterScale::linear }, 0.0 },
                { "freq", { 30.0, 18000.0, ParameterScale::logarithmic }, 1000.0 },
                { "q", { 0.2, 2.0, ParameterScale::logarithmic }, 0.707 }
            };
            break;
    }

    return contract;
}

inline std::size_t contract_parameter_count(const FilterContract& contract) {
    return contract.parameters.size();
}

inline bool parse_definition_arguments(
    FilterContract& contract,
    const c74::min::atoms& args
) {
    if (args.size() < 2) {
        return false;
    }

    const auto type = try_filter_type_from_symbol(static_cast<std::string>(args[1]));
    if (!type) {
        return false;
    }

    contract.type = *type;
    contract = make_default_contract(contract.slot, contract.type);

    std::size_t position = 2;
    for (std::size_t i = 0; i < contract.parameters.size(); ++i) {
        if (position >= args.size()) {
            return false;
        }

        auto& parameter = contract.parameters[i];
        parameter.name = static_cast<std::string>(args[position++]);

        if (position < args.size() && static_cast<std::string>(args[position]) == "discrete") {
            ++position;
            if (position + 1 >= args.size()) {
                return false;
            }

            parameter.range.scale = ParameterScale::discrete;
            parameter.default_value = static_cast<double>(args[position++]);
            const auto value_count = static_cast<int>(args[position++]);
            if (value_count <= 0 || position + static_cast<std::size_t>(value_count) > args.size()) {
                return false;
            }

            parameter.range.discrete_values.clear();
            for (int value_index = 0; value_index < value_count; ++value_index) {
                parameter.range.discrete_values.push_back(static_cast<double>(args[position++]));
            }

            const auto [minimum, maximum] = std::minmax_element(
                parameter.range.discrete_values.begin(),
                parameter.range.discrete_values.end());
            parameter.range.min_value = *minimum;
            parameter.range.max_value = *maximum;

            const bool has_default = std::any_of(
                parameter.range.discrete_values.begin(),
                parameter.range.discrete_values.end(),
                [&parameter](double value) {
                    return value == parameter.default_value;
                });
            if (!has_default) {
                return false;
            }
        }
        else {
            if (position + 2 >= args.size()) {
                return false;
            }

            parameter.range.discrete_values.clear();
            parameter.range.min_value = static_cast<double>(args[position++]);
            parameter.range.max_value = static_cast<double>(args[position++]);
            parameter.default_value = static_cast<double>(args[position++]);
        }

        if (parameter.default_value < parameter.range.min_value ||
            parameter.default_value > parameter.range.max_value) {
            return false;
        }
    }

    return position == args.size();
}

inline std::vector<double> default_normalized_values(const FilterContract& contract) {
    std::vector<double> values;
    values.reserve(contract.parameters.size());

    for (const auto& parameter : contract.parameters) {
        values.push_back(normalize_parameter(parameter.range, parameter.default_value));
    }

    return values;
}

inline std::vector<double> DefaultAbsoluteValues(const FilterContract& contract) {
    std::vector<double> values;
    values.reserve(contract.parameters.size());

    for (const auto& parameter : contract.parameters) {
        values.push_back(parameter.default_value);
    }

    return values;
}

inline bool AbsoluteValuesMatchContract(
    const FilterContract& contract,
    const std::vector<double>& values
) {
    if (values.size() != contract.parameters.size()) {
        return false;
    }

    for (std::size_t index = 0; index < values.size(); ++index) {
        const auto& parameter = contract.parameters[index];
        const auto value = values[index];
        if (!std::isfinite(value) || value < parameter.range.min_value ||
            value > parameter.range.max_value) {
            return false;
        }
        if (parameter.range.scale == ParameterScale::discrete &&
            std::find(parameter.range.discrete_values.begin(),
                parameter.range.discrete_values.end(), value) ==
                parameter.range.discrete_values.end()) {
            return false;
        }
    }

    return true;
}

inline FilterSpec AbsoluteValuesToSpec(
    const FilterContract& contract,
    const std::vector<double>& values
) {
    FilterSpec spec;
    spec.type = contract.type;
    if (!AbsoluteValuesMatchContract(contract, values)) {
        return spec;
    }

    spec.gainDb = values[0];
    if (contract.type == FilterType::tilt) {
        spec.pivotHz = values[1];
    }
    else if (contract.type != FilterType::gain) {
        spec.freqHz = values[1];
        spec.q = values[2];
    }
    return spec;
}

inline std::vector<double> spec_to_normalized_values(
    const FilterContract& contract,
    const FilterSpec& spec
) {
    std::vector<double> values;
    values.reserve(contract.parameters.size());

    if (contract.type == FilterType::gain) {
        values.push_back(normalize_parameter(contract.parameters[0].range, spec.gainDb));
        return values;
    }

    if (contract.type == FilterType::tilt) {
        values.push_back(normalize_parameter(contract.parameters[0].range, spec.gainDb));
        values.push_back(normalize_parameter(contract.parameters[1].range, spec.pivotHz));
        return values;
    }

    values.push_back(normalize_parameter(contract.parameters[0].range, spec.gainDb));
    values.push_back(normalize_parameter(contract.parameters[1].range, spec.freqHz));
    values.push_back(normalize_parameter(contract.parameters[2].range, spec.q));
    return values;
}

inline FilterSpec contract_to_spec(
    const FilterContract& contract,
    const std::vector<double>& normalized_values
) {
    FilterSpec spec;
    spec.type = contract.type;

    if (normalized_values.size() != contract.parameters.size()) {
        return spec;
    }

    if (contract.type == FilterType::gain) {
        spec.gainDb = denormalize_parameter(contract.parameters[0].range, normalized_values[0]);
        return spec;
    }

    if (contract.type == FilterType::tilt) {
        spec.gainDb = denormalize_parameter(contract.parameters[0].range, normalized_values[0]);
        spec.pivotHz = denormalize_parameter(contract.parameters[1].range, normalized_values[1]);
        return spec;
    }

    spec.gainDb = denormalize_parameter(contract.parameters[0].range, normalized_values[0]);
    spec.freqHz = denormalize_parameter(contract.parameters[1].range, normalized_values[1]);
    spec.q = denormalize_parameter(contract.parameters[2].range, normalized_values[2]);
    return spec;
}

inline c74::min::atoms make_definition_atoms(const FilterContract& contract) {
    c74::min::atoms atoms;
    atoms.push_back(std::string("define"));
    atoms.push_back(contract.slot);
    atoms.push_back(std::string(filter_type_name(contract.type)));

    for (const auto& parameter : contract.parameters) {
        atoms.push_back(parameter.name);
        if (parameter.range.scale == ParameterScale::discrete) {
            atoms.push_back(std::string("discrete"));
            atoms.push_back(parameter.default_value);
            atoms.push_back(static_cast<int>(parameter.range.discrete_values.size()));
            for (double value : parameter.range.discrete_values) {
                atoms.push_back(value);
            }
        }
        else {
            atoms.push_back(parameter.range.min_value);
            atoms.push_back(parameter.range.max_value);
            atoms.push_back(parameter.default_value);
        }
    }

    return atoms;
}

inline c74::min::atoms make_add_filter_atoms(const FilterContract& contract) {
    auto atoms = make_definition_atoms(contract);
    atoms[0] = std::string("add_filter");
    return atoms;
}

inline c74::min::atoms make_parameter_atoms(
    const std::string& command,
    const FilterContract& contract,
    const FilterSpec& spec
) {
    c74::min::atoms atoms;
    atoms.push_back(command);
    atoms.push_back(contract.slot);

    const auto values = spec_to_normalized_values(contract, spec);
    for (double value : values) {
        atoms.push_back(value);
    }

    return atoms;
}

inline c74::min::atoms make_filter_atoms(
    const FilterContract& contract,
    const FilterSpec& spec
) {
    return make_parameter_atoms("filter", contract, spec);
}

inline c74::min::atoms make_init_atoms(
    const FilterContract& contract,
    const FilterSpec& spec
) {
    return make_parameter_atoms("init", contract, spec);
}

inline c74::min::atoms make_reset_atoms(
    const FilterContract& contract,
    const FilterSpec& spec
) {
    return make_parameter_atoms("reset", contract, spec);
}
