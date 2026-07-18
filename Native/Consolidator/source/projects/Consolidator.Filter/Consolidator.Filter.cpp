#include "c74_min.h"

#include "FilterContract.h"
#include "FilterContractDictionary.h"
#include "EqFrequencyGrid.h"
#include "FilterSpec.h"
#include "MessageEnvelope.h"
#include "MessageFactory.h"
#include "TypedMessages.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace c74::min;

class ConsolidatorFilter : public object<ConsolidatorFilter> {
public:
    MIN_DESCRIPTION{ "Consolidator single-filter curve generator." };
    MIN_TAGS{ "audio, eq, filter" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) commands: message <dictionary type=filter.define|filter.control.update|filter.update|filter.bypass|filter.edit|filter.reset>"
    };

    outlet<> command_out{
        this,
        "(message) commands: message <dictionary type=filter.define|filter.update(filterId,values,bankIndex?)|filter.bypass>"
    };

    outlet<> status_out{
        this,
        "(anything) status: status initializing|ready|values <normalized...> <bypass>, error <code>"
    };

    outlet<> curve_out{
        this,
        "(list) filter response curve in dB"
    };

    outlet<> debug_out{
        this,
        "(anything) diagnostics: error <code>"
    };

    outlet<> handle_out{
        this,
        "(anything) messages: filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz> <gainDb> <type> <q> <qMin> <qMax> <curve...>, handle <filterId> <frequencyHz> <gainDb> <type> <active> <q> <qMin> <qMax>"
    };

    attribute<int> slot_attr{
        this,
        "slot",
        0,
        range { 0, 15 },
        description { "Filter slot index used by the chain." }
    };

    message<> dspsetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sample_rate_ = static_cast<double>(args[0]);
                publish();
            }
            return {};
        }
    };

    message<> envelope_message{
        this,
        "message",
        "Apply a structured control envelope",
        MIN_FUNCTION {
            if (args.size() != 1) {
                debug_out.send("error", "invalid_message_envelope");
                return {};
            }

            auto message = consolidator::protocol::MessageFactory::from_atom(args[0]);
            if (!message || !message->is_addressed_to("filter")) return {};

            const auto result = dispatch_command(*message);
            if (result == consolidator::protocol::MessageDispatchResult::invalid) {
                debug_out.send("error", "invalid_message_envelope");
            }
            return {};
        }
    };

private:
    consolidator::protocol::MessageDispatchResult dispatch_command(
        const consolidator::protocol::MessageEnvelope& message) {
        return consolidator::protocol::dispatch<
            consolidator::protocol::FilterDefineMessage,
            consolidator::protocol::FilterControlUpdateMessage,
            consolidator::protocol::FilterResetMessage,
            consolidator::protocol::FilterUpdateMessage,
            consolidator::protocol::FilterBypassMessage,
            consolidator::protocol::FilterEditMessage>(message, [this](const auto& command) {
                handle_command(command);
            });
    }

    bool accepts_target(const long target) const {
        return target == static_cast<long>(slot_attr);
    }

    void handle_command(const consolidator::protocol::FilterDefineMessage& command) {
        if (!accepts_target(command.filterId)) return;

        if (!command.contractName.empty()) {
            const dict configuration{ symbol(command.contractName.c_str()) };
            define_from_configuration(
                atom{ static_cast<c74::max::t_object*>(configuration) });
            return;
        }

        define_from_configuration(command.contract);
    }

    void handle_command(const consolidator::protocol::FilterControlUpdateMessage& command) {
        if (!accepts_target(command.filterId) || !defined_) return;

        if (command.control == "bypass") {
            if (command.value != 0.0 && command.value != 1.0) {
                debug_out.send("error", "bypass_must_be_0_or_1");
            }
            else {
                set_bypass(command.value == 1.0);
            }
            return;
        }

        if (!defined_) {
            debug_out.send("error", "filter_not_defined");
            return;
        }

        if (command.value < 0.0 || command.value > 1.0) {
            debug_out.send("error", "control_value_out_of_range", command.control, command.value);
            return;
        }

        if (!update_parameter(command.control, command.value)) {
            debug_out.send("error", "unsupported_filter_control", command.control);
            return;
        }

        publish();
        publish_values_status();
    }

    void handle_command(const consolidator::protocol::FilterResetMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_) {
            debug_out.send("error", "filter_not_defined");
            return;
        }
        reset_filter_state();
    }

    void handle_command(const consolidator::protocol::FilterUpdateMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_ ||
            !apply_normalized_values(atoms(command.values.begin(), command.values.end()))) {
            debug_out.send("error", "invalid_global_filter_values");
            return;
        }
        publish(command.bankIndex);
        publish_values_status();
    }

    void handle_command(const consolidator::protocol::FilterBypassMessage& command) {
        if (accepts_target(command.filterId)) set_bypass(command.bypassed);
    }

    void handle_command(const consolidator::protocol::FilterEditMessage& command) {
        if (!accepts_target(command.filterId)) return;

        if (!defined_ ||
            (command.q && !update_parameter("q", *command.q)) ||
            (command.frequency &&
             !apply_graph_edit(*command.frequency, *command.gain, std::nullopt))) {
            debug_out.send("error", "invalid_filter_edit");
            return;
        }

        publish();
        publish_values_status();
    }

    void define_from_configuration(const atom& configuration) {
        FilterContract contract;
        if (!parse_filter_contract_dictionary_for_slot(
                contract,
                configuration,
                static_cast<int>(slot_attr))) {
            debug_out.send("error", "invalid_filter_configuration_dictionary");
            return;
        }

        contract_ = contract;
        slot_attr = contract_.slot;

        const dict source{ configuration };
        definition_dictionary_name_ = "consolidator.filter.definition." +
            std::to_string(reinterpret_cast<std::uintptr_t>(this));
        definition_dictionary_ = std::make_unique<dict>(
            symbol(definition_dictionary_name_.c_str()));
        *definition_dictionary_ = source;
        read_filter_color(configuration);

        defined_ = true;
        bypassed_ = false;
        normalized_values_ = default_normalized_values(contract_);
        spec_ = contract_to_spec(contract_, normalized_values_);

        publish_definition();
        publish_curve();
        publish_handle(true);
        publish_filter_curve(true);
        publish();
        publish_values_status();
        status_out.send("status", "ready");
    }

    void read_filter_color(const atom& configuration) {
        color_ = { 1.0, 1.0, 1.0, 1.0 };
        try {
            dict root{ configuration };
            dict filters{ static_cast<atom>(root.at("filters")) };
            const auto selected = static_cast<int>(
                static_cast<double>(static_cast<atom>(root.at("selected"))));
            dict filter{ static_cast<atom>(filters.at(std::to_string(selected))) };
            if (const auto color = parse_hex_color(static_cast<atom>(filter.at("color")))) {
                color_ = *color;
            }
        }
        catch (...) {
        }
    }

    bool update_parameter(const std::string& control, const double value) {
        if (!defined_ || value < 0.0 || value > 1.0) return false;

        const auto parameter = std::find_if(
            contract_.parameters.begin(),
            contract_.parameters.end(),
            [&control](const auto& candidate) {
                return candidate.name == control ||
                    (control == "frequency" &&
                     (candidate.name == "freq" || candidate.name == "pivot"));
            });
        if (parameter == contract_.parameters.end()) return false;

        const auto index = static_cast<std::size_t>(
            std::distance(contract_.parameters.begin(), parameter));
        if (index >= normalized_values_.size()) return false;

        normalized_values_[index] = value;
        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    void set_bypass(const bool should_bypass) {
        if (!defined_ || bypassed_ == should_bypass) return;

        bypassed_ = should_bypass;
        publish_curve();
        publish_handle(!bypassed_);
        publish_filter_curve(!bypassed_);
        publish_bypass_message(bypassed_ ? 1.0 : 0.0);
        publish_values_status();
    }

    void reset_filter_state() {
        bypassed_ = false;
        normalized_values_ = default_normalized_values(contract_);
        spec_ = contract_to_spec(contract_, normalized_values_);
        publish();
        publish_values_status();
    }

    std::optional<std::array<double, 4>> parse_hex_color(const atom& value) const {
        std::string text;
        try {
            text = static_cast<std::string>(value);
        }
        catch (...) {
            return std::nullopt;
        }

        if (!text.empty() && text.front() == '#') text.erase(text.begin());
        if (text.size() != 6 && text.size() != 8) return std::nullopt;

        for (const auto character : text) {
            if (!std::isxdigit(static_cast<unsigned char>(character))) {
                return std::nullopt;
            }
        }

        auto component = [&text](const std::size_t offset) {
            return static_cast<double>(
                std::stoul(text.substr(offset, 2), nullptr, 16)) / 255.0;
        };

        return std::array<double, 4>{
            component(0),
            component(2),
            component(4),
            text.size() == 8 ? component(6) : 1.0
        };
    }

    bool apply_graph_edit(
        const double frequency_hz,
        const double gain_db,
        const std::optional<double>& q_value) {
        bool changed = false;
        for (std::size_t index = 0; index < contract_.parameters.size(); ++index) {
            const auto& parameter = contract_.parameters[index];
            if (parameter.name == "gain") {
                normalized_values_[index] =
                    normalize_parameter(parameter.range, gain_db);
                changed = true;
            }
            else if (parameter.name == "freq" || parameter.name == "pivot") {
                normalized_values_[index] =
                    normalize_parameter(parameter.range, frequency_hz);
                changed = true;
            }
            else if (parameter.name == "q" && q_value) {
                normalized_values_[index] =
                    normalize_parameter(parameter.range, *q_value);
                changed = true;
            }
        }

        if (!changed) return false;
        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    bool apply_normalized_values(const atoms& args) {
        if (args.size() != contract_parameter_count(contract_)) return false;

        normalized_values_.clear();
        normalized_values_.reserve(args.size());
        for (const auto& value : args) {
            const auto number = static_cast<double>(value);
            if (number < 0.0 || number > 1.0) return false;
            normalized_values_.push_back(number);
        }

        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    void publish(std::optional<long> bankIndex = std::nullopt) {
        publish_curve();
        const bool active = defined_ && !bypassed_;
        publish_handle(active);
        publish_filter_curve(active);
        if (!defined_) return;

        const consolidator::protocol::FilterUpdateMessage typed_message{
            contract_.slot, normalized_values_, bankIndex
        };
        const auto message = typed_message.to_envelope();
        command_out.send("message", message.transport_atom());
    }

    void publish_values_status() {
        atoms message;
        message.reserve(normalized_values_.size() + 3);
        message.push_back("status");
        message.push_back("values");
        for (const auto value : normalized_values_) message.push_back(value);
        message.push_back(bypassed_ ? 1 : 0);
        status_out.send(message);
    }

    void publish_bypass_message(const double value) {
        const consolidator::protocol::FilterBypassMessage typed_message{
            contract_.slot, value == 1.0
        };
        const auto message = typed_message.to_envelope();
        command_out.send("message", message.transport_atom());
    }

    void publish_handle(const bool active) {
        const double handle_frequency =
            contract_.type == FilterType::tilt ? spec_.pivotHz : spec_.freqHz;

        double q = 0.0;
        double q_min = 0.0;
        double q_max = 0.0;
        for (const auto& parameter : contract_.parameters) {
            if (parameter.name == "q") {
                q = spec_.q;
                q_min = parameter.range.min_value;
                q_max = parameter.range.max_value;
                break;
            }
        }

        handle_out.send(
            "handle",
            contract_.slot,
            handle_frequency,
            spec_.gainDb,
            filter_type_name(contract_.type),
            active ? 1 : 0,
            q,
            q_min,
            q_max
        );
    }

    void publish_filter_curve(const bool active) {
        const auto curve = active ? response_curve() :
            std::vector<double>(make_eq_curve_frequency_grid().size(), 0.0);

        atoms message;
        message.reserve(13 + curve.size());
        message.push_back("filter_curve");
        message.push_back(contract_.slot);
        message.push_back(active ? 1 : 0);
        message.push_back(color_[0]);
        message.push_back(color_[1]);
        message.push_back(color_[2]);
        message.push_back(color_[3]);
        message.push_back(
            contract_.type == FilterType::tilt ? spec_.pivotHz : spec_.freqHz);
        message.push_back(spec_.gainDb);
        message.push_back(filter_type_name(contract_.type));

        double q = 0.0;
        double q_min = 0.0;
        double q_max = 0.0;
        for (const auto& parameter : contract_.parameters) {
            if (parameter.name == "q") {
                q = spec_.q;
                q_min = parameter.range.min_value;
                q_max = parameter.range.max_value;
                break;
            }
        }

        message.push_back(q);
        message.push_back(q_min);
        message.push_back(q_max);
        for (const double value : curve) message.push_back(value);
        handle_out.send(message);
    }

    void publish_curve() {
        if (!defined_ || bypassed_) {
            const auto frequencies = make_eq_curve_frequency_grid();
            curve_out.send(atoms(frequencies.size(), 0.0));
            return;
        }

        const auto curve = response_curve();
        atoms curve_atoms;
        curve_atoms.reserve(curve.size());
        for (const double value : curve) curve_atoms.push_back(value);
        curve_out.send(curve_atoms);
    }

    void publish_definition() {
        if (!definition_dictionary_) {
            debug_out.send("error", "filter_definition_requires_dictionary");
            return;
        }

        const consolidator::protocol::FilterDefineMessage typed_message{
            contract_.slot, {}, definition_dictionary_name_
        };
        const auto message = typed_message.to_envelope();
        command_out.send("message", message.transport_atom());
    }

    std::vector<double> response_curve() const {
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> result;
        result.reserve(frequencies.size());
        for (const double frequency_hz : frequencies) {
            result.push_back(filter_response_db(spec_, frequency_hz, sample_rate_));
        }
        return result;
    }

    double sample_rate_ = EqCurveGrid::default_sample_rate;
    FilterSpec spec_{};
    FilterContract contract_ = make_default_contract(0, FilterType::peak);
    std::unique_ptr<dict> definition_dictionary_;
    std::string definition_dictionary_name_;
    std::vector<double> normalized_values_;
    std::array<double, 4> color_{ 1.0, 1.0, 1.0, 1.0 };
    bool defined_ = false;
    bool bypassed_ = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorFilter, consolidator.filter);
