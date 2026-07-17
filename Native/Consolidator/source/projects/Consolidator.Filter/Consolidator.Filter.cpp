#include "c74_min.h"

#include "FilterContract.h"
#include "FilterContractDictionary.h"
#include "Control.h"
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
#include <utility>
#include <vector>

using namespace c74::min;

class ConsolidatorFilter : public object<ConsolidatorFilter> {
public:
    MIN_DESCRIPTION{ "Consolidator single-filter curve generator." };
    MIN_TAGS{ "audio, eq, filter" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commands{
        this,
        "(message) commands: message <dictionary type=filter.define|filter.instance.state|filter.control.update|filter.update|filter.bypass|filter.edit|filter.reset>"
    };

    outlet<> command_out{
        this,
        "(message) commands: message <dictionary type=filter.define|filter.update(filterId,values,bankIndex?)|filter.bypass>"
    };

    outlet<> status_out{
        this,
        "(anything) status: initializing, ready, error <code>"
    };

    outlet<> curve_out{
        this,
        "(list) filter response curve in dB"
    };

    outlet<> local_command_out{
        this,
        "(message) commands: message <dictionary type=filter.control>"
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
            if (!message) return {};
            if (!message->is_addressed_to("filter")) return {};
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
            consolidator::protocol::FilterInstanceStateMessage,
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
              define_from_configuration(atom{ static_cast<c74::max::t_object*>(configuration) });
              return;
          }
          define_from_configuration(command.contract);
    }

    void handle_command(const consolidator::protocol::FilterInstanceStateMessage& command) {
        if (!accepts_target(command.filterId)) return;
        pending_instance_recovered_ = command.recovered;
        if (defined_) {
            apply_instance_state(*pending_instance_recovered_);
            pending_instance_recovered_.reset();
        }
    }

    void handle_command(const consolidator::protocol::FilterControlUpdateMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_) return;
        if (command.control == "bypass") {
            if (command.value != 0.0 && command.value != 1.0) debug_out.send("error", "bypass_must_be_0_or_1");
            else set_bypass(command.value == 1.0);
            return;
        }
        if (!update_parameter(command.control, command.value)) debug_out.send("error", "invalid_update_control");
        else publish();
    }

    void handle_command(const consolidator::protocol::FilterResetMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_) debug_out.send("error", "filter_not_defined");
        else reset_filter_state();
    }

    void handle_command(const consolidator::protocol::FilterUpdateMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_ || !apply_normalized_values(atoms(command.values.begin(), command.values.end()))) {
            debug_out.send("error", "invalid_global_filter_values");
            return;
        }
        publish(command.bankIndex);
    }

    void handle_command(const consolidator::protocol::FilterBypassMessage& command) {
        if (accepts_target(command.filterId)) set_bypass(command.bypassed);
    }

    void handle_command(const consolidator::protocol::FilterEditMessage& command) {
        if (!accepts_target(command.filterId)) return;
        if (!defined_ || (command.q && !update_parameter("q", *command.q)) ||
            (command.frequency && !apply_graph_edit(*command.frequency, *command.gain, std::nullopt))) {
            debug_out.send("error", "invalid_filter_edit");
            return;
        }
        publish_parameter_values(normalized_values_);
        publish();
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
          read_controls(configuration);
        defined_ = true;
        bypassed_ = false;
        normalized_values_ = default_normalized_values(contract_);
        spec_ = contract_to_spec(contract_, normalized_values_);
        publish_definition();
        publish_initialization();
        publish_curve();
        publish_handle(true);
        publish_filter_curve(true);
        publish();
        status_out.send("status", "ready");
    }

    bool update_parameter(const std::string& control, const double value) {
        if (!defined_ || value < 0.0 || value > 1.0) {
            return false;
        }

        const std::string control_id = control == "freq" ? "frequency" : control;
        const auto parameter = std::find_if(
            parameter_control_ids_.begin(),
            parameter_control_ids_.end(),
            [&control_id](const std::string& id) { return id == control_id; });
        if (parameter == parameter_control_ids_.end()) {
            return false;
        }

        const auto index = static_cast<std::size_t>(
            std::distance(parameter_control_ids_.begin(), parameter));
        if (index >= normalized_values_.size()) {
            return false;
        }

        normalized_values_[index] = value;
        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    void set_bypass(const bool should_bypass) {
        if (!defined_ || bypassed_ == should_bypass) {
            return;
        }

        bypassed_ = should_bypass;
        if (bypassed_) {
            set_controls_enabled(false);
            publish_curve();
            publish_handle(false);
            publish_filter_curve(false);
            publish_bypass_message(1.0);
        }
        else {
            set_controls_enabled(true);
            publish_curve();
            publish_handle(true);
            publish_filter_curve(true);
            publish_bypass_message(0.0);
        }
    }

    void publish_initialization() {
        publish_control_state();

        if (pending_instance_recovered_) {
            if (*pending_instance_recovered_) {
                output_control_values();
            }
            else {
                reset_filter_state();
            }
            pending_instance_recovered_.reset();
        }

    }

    void publish_bypass_message(const double value) {
        const consolidator::protocol::FilterBypassMessage typed_message{
            contract_.slot, value == 1.0
        };
        const auto message = typed_message.to_envelope();
        command_out.send("message", message.transport_atom());
    }

    void apply_instance_state(const bool is_recovered_instance) {
        publish_control_state();

        if (is_recovered_instance) {
            output_control_values();
        }
        else {
            reset_filter_state();
        }

    }

    void reset_filter_state() {
        bypassed_ = false;
        const auto defaults = default_normalized_values(contract_);
        normalized_values_ = defaults;
        spec_ = contract_to_spec(contract_, normalized_values_);
        set_controls_enabled(true);
        publish_parameter_values(defaults);
        publish();
    }

    void read_controls(const atom& configuration) {
        controls_.clear();
        parameter_control_ids_.clear();

        try {
            dict root{ configuration };
            dict selected_filter{ static_cast<atom>(root.at("filters")) };
            const auto selected_slot = static_cast<int>(static_cast<double>(
                static_cast<atom>(root.at("selected"))));
            selected_filter = dict{ static_cast<atom>(selected_filter.at(std::to_string(selected_slot))) };
            dict common_controls{ static_cast<atom>(root.at("controls")) };
            dict layouts{ static_cast<atom>(root.at("layouts")) };
            dict parameters{ static_cast<atom>(selected_filter.at("parameters")) };
            dict layout{ static_cast<atom>(layouts.at(filter_type_name(contract_.type))) };

            color_ = { 1.0, 1.0, 1.0, 1.0 };
            try {
                if (const auto configured_color = parse_hex_color(static_cast<atom>(selected_filter.at("color")))) {
                    color_ = *configured_color;
                }
            }
            catch (...) {
            }

            for (const auto& parameter : contract_.parameters) {
                try {
                    dict parameter_definition{
                        static_cast<atom>(parameters.at(parameter.name))
                    };
                    std::string parameter_control;
                    if (read_dictionary_symbol(parameter_definition, "control", parameter_control)) {
                        parameter_control_ids_.push_back(parameter_control);
                    }
                }
                catch (...) {
                    parameter_control_ids_.push_back({});
                }
            }

            for (const auto& control_symbol : common_controls.keys()) {
                const auto control_id = std::string(
                    static_cast<const char* const>(control_symbol));
                dict control_definition{ static_cast<atom>(common_controls.at(control_id)) };
                const auto position = read_position(control_definition, "defaultPosition");
                if (!position) {
                    continue;
                }

                bool output_value = false;
                double output_value_number = 0.0;
                if (read_dictionary_number(control_definition, "outputValue", output_value_number)) {
                    output_value = output_value_number != 0.0;
                }
                bool always_enabled = false;
                double default_enabled = 0.0;
                if (read_dictionary_number(control_definition, "defaultEnabled", default_enabled)) {
                    always_enabled = default_enabled != 0.0;
                }

                ControlState state{
                    FilterControl{ control_id, *position },
                    true,
                    true,
                    output_value,
                    false,
                    always_enabled
                };
                for (const auto& parameter : contract_.parameters) {
                    try {
                        dict parameter_definition{
                            static_cast<atom>(parameters.at(parameter.name))
                        };
                        std::string parameter_control;
                        if (read_dictionary_symbol(parameter_definition, "control", parameter_control) &&
                            parameter_control == control_id) {
                            state.active = true;
                            break;
                        }
                    }
                    catch (...) {
                    }
                }

                try {
                    dict override{ static_cast<atom>(layout.at(control_id)) };
                    if (const auto override_position = read_position(override, "position")) {
                        state.control.set_position(*override_position);
                    }
                    double visible = state.visible ? 1.0 : 0.0;
                    double enabled = state.enabled ? 1.0 : 0.0;
                    read_dictionary_number(override, "visible", visible);
                    read_dictionary_number(override, "enabled", enabled);
                    state.visible = visible != 0.0;
                    state.enabled = enabled != 0.0;
                }
                catch (...) {
                }

                if (!state.active && !state.always_enabled) {
                    state.enabled = false;
                }

                controls_.push_back(std::move(state));
            }
        }
        catch (...) {
            controls_.clear();
        }
    }

    std::optional<std::array<double, 4>> read_position(dict& source, const char* key) const {
        try {
            const auto values = static_cast<std::vector<number>>(source.at(key));
            if (values.size() != 4) {
                return std::nullopt;
            }
            return std::array<double, 4>{ values[0], values[1], values[2], values[3] };
        }
        catch (...) {
            return std::nullopt;
        }
    }

    void send_control_message(const consolidator::protocol::MessageEnvelope& message) {
        local_command_out.send("message", message.transport_atom());
    }

    void publish_control_state() {
        for (const auto& state : controls_) {
            send_control_message(state.control.control_update("move", {
                state.control.position()[0], state.control.position()[1],
                state.control.position()[2], state.control.position()[3]
            }));
            send_control_message(state.control.control_update(state.visible ? "show" : "hide"));
            send_control_message(state.control.control_update(state.enabled ? "enable" : "disable"));
        }
        publish_control_colors_from_config();
    }

    void publish_control_colors_from_config() {
        for (const auto& state : controls_) {
            if (state.control.id() == "gain" ||
                state.control.id() == "frequency" ||
                state.control.id() == "q" ||
                state.control.id() == "bypass" ||
                state.control.id() == "reset") {
                send_control_message(state.control.control_update("color", {
                    color_[0], color_[1], color_[2], color_[3]
                }));
            }
        }
    }

    void set_controls_enabled(bool enabled) {
        for (const auto& state : controls_) {
            const bool control_enabled = state.always_enabled || (enabled && state.enabled);
            send_control_message(state.control.control_update(control_enabled ? "enable" : "disable"));
        }
    }

    void output_control_values() {
        for (const auto& state : controls_) {
            if (state.active) {
                send_control_message(state.control.control_update("outputvalue"));
            }
        }
    }

    void publish_parameter_values(const std::vector<double>& values) {
        for (std::size_t i = 0; i < values.size() && i < parameter_control_ids_.size(); ++i) {
            const auto control = std::find_if(
                controls_.begin(),
                controls_.end(),
                [this, i](const ControlState& state) {
                    return state.control.id() == parameter_control_ids_[i];
                });
            if (control != controls_.end()) {
                send_control_message(control->control.control_update("set", { values[i] }));
            }
        }
    }

    std::optional<std::array<double, 4>> parse_hex_color(const atom& value) const {
        std::string text;
        try {
            text = static_cast<std::string>(value);
        }
        catch (...) {
            return std::nullopt;
        }

        if (!text.empty() && text.front() == '#') {
            text.erase(text.begin());
        }
        if (text.size() != 6 && text.size() != 8) {
            return std::nullopt;
        }
        for (const auto character : text) {
            if (!std::isxdigit(static_cast<unsigned char>(character))) {
                return std::nullopt;
            }
        }

        auto component = [&text](const std::size_t offset) {
            return static_cast<double>(std::stoul(text.substr(offset, 2), nullptr, 16)) / 255.0;
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
        const std::optional<double>& q_value
    ) {
        bool changed = false;
        for (std::size_t index = 0; index < contract_.parameters.size(); ++index) {
            const auto& parameter = contract_.parameters[index];
            if (parameter.name == "gain") {
                normalized_values_[index] = normalize_parameter(parameter.range, gain_db);
                changed = true;
            }
            else if (parameter.name == "freq" || parameter.name == "pivot") {
                normalized_values_[index] = normalize_parameter(parameter.range, frequency_hz);
                changed = true;
            }
            else if (parameter.name == "q" && q_value) {
                normalized_values_[index] = normalize_parameter(parameter.range, *q_value);
                changed = true;
            }
        }

        if (!changed) {
            return false;
        }

        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    bool apply_normalized_values(const atoms& args) {
        if (args.size() != contract_parameter_count(contract_)) {
            return false;
        }

        std::vector<double> normalized;
        normalized.reserve(args.size());
        for (const auto& value : args) {
            normalized.push_back(static_cast<double>(value));
        }

        normalized_values_ = normalized;
        spec_ = contract_to_spec(contract_, normalized_values_);
        return true;
    }

    void publish(std::optional<long> bankIndex = std::nullopt) {
        publish_curve();
        const bool active = defined_ && !bypassed_;
        publish_handle(active);
        publish_filter_curve(active);
        if (!defined_) {
            return;
        }

        const consolidator::protocol::FilterUpdateMessage typed_message{
            contract_.slot, normalized_values_, bankIndex
        };
        const auto message = typed_message.to_envelope();
        command_out.send("message", message.transport_atom());
    }

    void publish_handle(const bool active) {
        const double handle_frequency = contract_.type == FilterType::tilt
            ? spec_.pivotHz
            : spec_.freqHz;
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
        const auto curve = active ? response_curve() : std::vector<double>(
            make_eq_curve_frequency_grid().size(), 0.0);

        atoms message;
        message.reserve(7 + curve.size());
        message.push_back("filter_curve");
        message.push_back(contract_.slot);
        message.push_back(active ? 1 : 0);
        message.push_back(color_[0]);
        message.push_back(color_[1]);
        message.push_back(color_[2]);
        message.push_back(color_[3]);
        message.push_back(contract_.type == FilterType::tilt ? spec_.pivotHz : spec_.freqHz);
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
        for (const double value : curve) {
            message.push_back(value);
        }
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
        for (double value : curve) {
            curve_atoms.push_back(value);
        }
        curve_out.send(curve_atoms);
    }

      void publish_definition() {
          if (definition_dictionary_) {
              const consolidator::protocol::FilterDefineMessage typed_message{
                  contract_.slot, {}, definition_dictionary_name_
              };
            const auto message = typed_message.to_envelope();
            command_out.send("message", message.transport_atom());
            return;
        }

        debug_out.send("error", "filter_definition_requires_dictionary");
    }

    std::vector<double> response_curve() const {
        const auto frequencies = make_eq_curve_frequency_grid();
        std::vector<double> result;
        result.reserve(frequencies.size());

        for (double frequency_hz : frequencies) {
            result.push_back(filter_response_db(spec_, frequency_hz, sample_rate_));
        }

        return result;
    }

    double sample_rate_ = EqCurveGrid::default_sample_rate;
    FilterSpec spec_{};
    FilterContract contract_ = make_default_contract(0, FilterType::peak);
      std::unique_ptr<dict> definition_dictionary_;
      std::string definition_dictionary_name_;
    struct ControlState {
        FilterControl control;
        bool visible = true;
        bool enabled = true;
        bool output_value = false;
        bool active = false;
        bool always_enabled = false;
    };
    std::vector<ControlState> controls_;
    std::vector<std::string> parameter_control_ids_;
    std::vector<double> normalized_values_;
    std::array<double, 4> color_{ 1.0, 1.0, 1.0, 1.0 };
    std::optional<bool> pending_instance_recovered_;
    bool defined_ = false;
    bool bypassed_ = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorFilter, consolidator.filter);
