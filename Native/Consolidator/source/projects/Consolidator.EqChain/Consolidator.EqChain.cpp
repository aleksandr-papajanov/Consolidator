#include "c74_min.h"

#include "FilterContract.h"
#include "FilterContractDictionary.h"
#include "FilterChain.h"

#include <array>
#include <vector>

using namespace c74::min;

class ConsolidatorEqChain :
    public object<ConsolidatorEqChain>,
    public sample_operator<2, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ chain audio processor." };
    MIN_TAGS{ "audio, eq, chain" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> input_l{ this, "(signal) left input", "signal" };
    inlet<> input_r{ this, "(signal) right input", "signal" };
    inlet<> commands_in{
        this,
        "(anything) commands: define <slot> <type> <ranges...>, dictionary <dictionary>, undefine <slot>, filter <slot> <normalized values...>"
    };

    outlet<> output_l{ this, "(signal) left output", "signal" };
    outlet<> output_r{ this, "(signal) right output", "signal" };
    outlet<> curve_out{ this, "(list) reserved compatibility outlet; no graphics output" };
    outlet<> command_out{ this, "(anything) commands: add_filter, remove_filter" };
    outlet<> debug_out{ this, "(anything) diagnostics: error <code>" };

    message<> dspsetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                chain_.set_sample_rate(static_cast<double>(args[0]));
            }

            return {};
        }
    };

    message<> filter_message{
        this,
        "filter",
        "Set a normalized filter slot",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            if (!apply_filter_values(args)) {
                return {};
            }

            return {};
        }
    };

    message<> define_message{
        this,
        "define",
        "Define a slot dynamically",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            if (args.size() == 1 && dictionary_atom(args[0])) {
                if (!define_dictionary(args[0])) {
                    debug_out.send("error", "invalid_filter_configuration_dictionary");
                }
                return {};
            }

            if (args.size() < 2) {
                debug_out.send("error", "invalid_define_command");
                return {};
            }

            const size_t slot = static_cast<size_t>(args[0]);
            if (slot >= contracts_.size()) {
                debug_out.send("error", "invalid_filter_slot");
                return {};
            }
            FilterContract contract;
            contract.slot = static_cast<int>(slot);
            if (!parse_definition_arguments(contract, args)) {
                debug_out.send("error", "invalid_filter_contract");
                return {};
            }

            contracts_[slot] = contract;
            chain_.remove_filter(slot);
            publish_definition(slot);
            return {};
        }
    };

    message<> dictionary_message{
        this,
        "dictionary",
        "Define a slot from a configuration dictionary",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1 || !dictionary_atom(args[0])) {
                debug_out.send("error", "invalid_filter_configuration_dictionary");
                return {};
            }

            if (!define_dictionary(args[0])) {
                debug_out.send("error", "invalid_filter_configuration_dictionary");
            }
            return {};
        }
    };

    message<> undefine_message{
        this,
        "undefine",
        "Remove one dynamically defined filter",
        MIN_FUNCTION {
            if (inlet != 2) {
                debug_out.send("error", "commands_must_use_command_inlet");
                return {};
            }

            if (args.size() != 1) {
                debug_out.send("error", "undefine_requires_slot");
                return {};
            }

            const size_t slot = static_cast<size_t>(args[0]);
            if (slot >= contracts_.size()) {
                debug_out.send("error", "invalid_filter_slot");
                return {};
            }

            contracts_[slot].reset();
            chain_.remove_filter(slot);
            command_out.send("remove_filter", static_cast<int>(slot));
            return {};
        }
    };

    samples<2> operator()(sample in_l, sample in_r) {
        const auto [out_l, out_r] = chain_.process(in_l, in_r);
        return { out_l, out_r };
    }

private:
    bool define_dictionary(const atom& value) {
        FilterContract contract;
        if (!parse_filter_contract_dictionary(contract, value)) {
            return false;
        }

        const auto slot = static_cast<size_t>(contract.slot);
        if (slot >= contracts_.size()) {
            return false;
        }

        contracts_[slot] = contract;
        chain_.remove_filter(slot);
        command_out.send("add_filter", value);
        return true;
    }

    bool apply_filter_values(const atoms& args) {
        if (args.size() < 2) {
            debug_out.send("error", "invalid_filter_command");
            return false;
        }

        const size_t slot = static_cast<size_t>(args[0]);
        if (slot >= contracts_.size()) {
            debug_out.send("error", "invalid_filter_slot");
            return false;
        }
        if (!contracts_[slot]) {
            debug_out.send("error", "filter_slot_not_defined");
            return false;
        }

        const auto& contract = *contracts_[slot];
        const std::size_t expected = 1 + contract_parameter_count(contract);
        if (args.size() != expected) {
            debug_out.send("error", "invalid_filter_values");
            return false;
        }

        std::vector<double> normalized;
        normalized.reserve(expected - 1);
        for (std::size_t i = 1; i < args.size(); ++i) {
            normalized.push_back(static_cast<double>(args[i]));
        }

        chain_.set_filter(slot, contract_to_spec(contract, normalized));
        return true;
    }

    void publish_definition(size_t slot) {
        if (contracts_[slot]) {
            command_out.send(make_add_filter_atoms(*contracts_[slot]));
        }
    }

    FilterChain chain_;
    std::array<std::optional<FilterContract>, FilterChain::max_filters> contracts_{};
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqChain, consolidator.eqchain);
